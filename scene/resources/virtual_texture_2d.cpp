/**************************************************************************/
/*  virtual_texture_2d.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "virtual_texture_2d.h"

#include "core/object/class_db.h"

VirtualTexture2D::VirtualTexture2D() {
	rebuild_virtual_texture();
}

VirtualTexture2D::~VirtualTexture2D() {
}

uint64_t VirtualTexture2D::_get_tile_key(int p_tile_x, int p_tile_y, int p_mip) const {
	return ((uint64_t)(p_mip & 0xFF) << 48) | ((uint64_t)(p_tile_y & 0xFFFFFF) << 24) | ((uint64_t)(p_tile_x & 0xFFFFFF));
}

int VirtualTexture2D::get_virtual_tile_count_x() const {
	return MAX(1, (virtual_size.x + tile_size - 1) / tile_size);
}

int VirtualTexture2D::get_virtual_tile_count_y() const {
	return MAX(1, (virtual_size.y + tile_size - 1) / tile_size);
}

int VirtualTexture2D::get_total_virtual_tiles() const {
	return get_virtual_tile_count_x() * get_virtual_tile_count_y();
}

int VirtualTexture2D::get_slots_per_axis() const {
	return MAX(1, physical_cache_size / tile_size);
}

int VirtualTexture2D::get_physical_slot_count() const {
	int spa = get_slots_per_axis();
	return spa * spa;
}

int VirtualTexture2D::get_resident_tile_count() const {
	int count = 0;
	for (int i = 0; i < physical_slots.size(); ++i) {
		if (physical_slots[i].is_resident) {
			count++;
		}
	}
	return count;
}

int64_t VirtualTexture2D::get_theoretical_vram_bytes() const {
	return (int64_t)virtual_size.x * (int64_t)virtual_size.y * 4; // RGBA8
}

int64_t VirtualTexture2D::get_actual_vram_bytes() const {
	return (int64_t)physical_cache_size * (int64_t)physical_cache_size * 4; // RGBA8
}

float VirtualTexture2D::get_vram_savings_percentage() const {
	int64_t theo = get_theoretical_vram_bytes();
	int64_t act = get_actual_vram_bytes();
	if (theo <= 0) return 0.0f;
	return (1.0f - ((float)act / (float)theo)) * 100.0f;
}

float VirtualTexture2D::get_cache_hit_rate() const {
	if (cache_requests == 0) return 100.0f;
	return ((float)cache_hits / (float)cache_requests) * 100.0f;
}

void VirtualTexture2D::rebuild_virtual_texture() {
	int total_slots = get_physical_slot_count();
	physical_slots.resize(total_slots);
	for (int i = 0; i < total_slots; ++i) {
		physical_slots.write[i].slot_idx = i;
		physical_slots.write[i].tile_x = -1;
		physical_slots.write[i].tile_y = -1;
		physical_slots.write[i].mip = -1;
		physical_slots.write[i].last_access_tick = 0;
		physical_slots.write[i].is_resident = false;
		physical_slots.write[i].is_loading = false;
	}
	tile_to_slot_map.clear();

	// 1. Create Physical Cache Atlas
	physical_cache_image = Image::create_empty(physical_cache_size, physical_cache_size, false, Image::FORMAT_RGBA8);
	physical_cache_image->fill(Color(0.2f, 0.22f, 0.25f, 1.0f));
	physical_cache_texture = ImageTexture::create_from_image(physical_cache_image);

	// 2. Create Page Table Indirection Texture
	int ptx = get_virtual_tile_count_x();
	int pty = get_virtual_tile_count_y();
	page_table_image = Image::create_empty(ptx, pty, false, Image::FORMAT_RGBA8);
	page_table_image->fill(Color(0.0f, 0.0f, 0.0f, 0.0f));
	page_table_texture = ImageTexture::create_from_image(page_table_image);
}

int VirtualTexture2D::_allocate_physical_slot(int p_tile_x, int p_tile_y, int p_mip) {
	int best_slot = -1;
	uint64_t oldest_tick = UINT64_MAX;

	for (int i = 0; i < physical_slots.size(); ++i) {
		if (!physical_slots[i].is_resident && !physical_slots[i].is_loading) {
			best_slot = i;
			break;
		}
		if (physical_slots[i].last_access_tick < oldest_tick) {
			oldest_tick = physical_slots[i].last_access_tick;
			best_slot = i;
		}
	}

	if (best_slot >= 0) {
		PhysicalSlot &slot = physical_slots.write[best_slot];
		if (slot.is_resident && slot.tile_x >= 0) {
			uint64_t old_key = _get_tile_key(slot.tile_x, slot.tile_y, slot.mip);
			tile_to_slot_map.erase(old_key);
			_update_page_table_entry(slot.tile_x, slot.tile_y, slot.mip, -1);
		}

		slot.tile_x = p_tile_x;
		slot.tile_y = p_tile_y;
		slot.mip = p_mip;
		slot.last_access_tick = ++current_access_tick;
		slot.is_resident = false;
		slot.is_loading = true;

		uint64_t new_key = _get_tile_key(p_tile_x, p_tile_y, p_mip);
		tile_to_slot_map[new_key] = best_slot;
	}

	return best_slot;
}

void VirtualTexture2D::_update_page_table_entry(int p_tile_x, int p_tile_y, int p_mip, int p_slot_idx) {
	if (page_table_image.is_null()) return;
	if (p_tile_x < 0 || p_tile_x >= page_table_image->get_width()) return;
	if (p_tile_y < 0 || p_tile_y >= page_table_image->get_height()) return;

	if (p_slot_idx < 0) {
		page_table_image->set_pixel(p_tile_x, p_tile_y, Color(0.0f, 0.0f, 0.0f, 0.0f));
	} else {
		int spa = get_slots_per_axis();
		int sx = p_slot_idx % spa;
		int sy = p_slot_idx / spa;
		float r = (float)(sx + 1) / (float)spa;
		float g = (float)(sy + 1) / (float)spa;
		float b = (float)p_mip / 16.0f;
		page_table_image->set_pixel(p_tile_x, p_tile_y, Color(r, g, b, 1.0f));
	}

	if (page_table_texture.is_valid()) {
		page_table_texture->update(page_table_image);
	}
}

void VirtualTexture2D::request_tile(int p_tile_x, int p_tile_y, int p_mip) {
	cache_requests++;
	uint64_t key = _get_tile_key(p_tile_x, p_tile_y, p_mip);
	if (tile_to_slot_map.has(key)) {
		int slot_idx = tile_to_slot_map[key];
		physical_slots.write[slot_idx].last_access_tick = ++current_access_tick;
		cache_hits++;
		return;
	}

	_allocate_physical_slot(p_tile_x, p_tile_y, p_mip);
	emit_signal(SNAME("tile_requested"), p_tile_x, p_tile_y, p_mip);
}

void VirtualTexture2D::request_region(const Rect2 &p_uv_rect, int p_mip) {
	int tiles_x = get_virtual_tile_count_x();
	int tiles_y = get_virtual_tile_count_y();
	if (tiles_x <= 0 || tiles_y <= 0) return;

	int min_tx = CLAMP((int)(p_uv_rect.position.x * tiles_x), 0, tiles_x - 1);
	int max_tx = CLAMP((int)((p_uv_rect.position.x + p_uv_rect.size.x) * tiles_x), 0, tiles_x - 1);
	int min_ty = CLAMP((int)(p_uv_rect.position.y * tiles_y), 0, tiles_y - 1);
	int max_ty = CLAMP((int)((p_uv_rect.position.y + p_uv_rect.size.y) * tiles_y), 0, tiles_y - 1);

	for (int ty = min_ty; ty <= max_ty; ++ty) {
		for (int tx = min_tx; tx <= max_tx; ++tx) {
			request_tile(tx, ty, p_mip);
		}
	}
}

void VirtualTexture2D::request_tiles_around_point(const Vector2 &p_uv_point, float p_radius_uv, int p_mip) {
	Rect2 uv_rect(p_uv_point.x - p_radius_uv, p_uv_point.y - p_radius_uv, p_radius_uv * 2.0f, p_radius_uv * 2.0f);
	request_region(uv_rect, p_mip);
}

bool VirtualTexture2D::is_tile_resident(int p_tile_x, int p_tile_y, int p_mip) const {
	uint64_t key = _get_tile_key(p_tile_x, p_tile_y, p_mip);
	if (!tile_to_slot_map.has(key)) return false;
	int slot_idx = tile_to_slot_map[key];
	return physical_slots[slot_idx].is_resident;
}

void VirtualTexture2D::upload_tile_data(int p_tile_x, int p_tile_y, int p_mip, const Ref<Image> &p_image) {
	uint64_t key = _get_tile_key(p_tile_x, p_tile_y, p_mip);
	int slot_idx = -1;
	if (tile_to_slot_map.has(key)) {
		slot_idx = tile_to_slot_map[key];
	} else {
		slot_idx = _allocate_physical_slot(p_tile_x, p_tile_y, p_mip);
	}

	if (slot_idx < 0 || p_image.is_null()) return;

	PhysicalSlot &slot = physical_slots.write[slot_idx];
	slot.is_resident = true;
	slot.is_loading = false;
	slot.last_access_tick = ++current_access_tick;

	int spa = get_slots_per_axis();
	int sx = slot_idx % spa;
	int sy = slot_idx / spa;

	if (physical_cache_image.is_valid()) {
		physical_cache_image->blit_rect(p_image, Rect2i(0, 0, MIN(tile_size, p_image->get_width()), MIN(tile_size, p_image->get_height())), Vector2i(sx * tile_size, sy * tile_size));
		if (physical_cache_texture.is_valid()) {
			physical_cache_texture->update(physical_cache_image);
		}
	}

	_update_page_table_entry(p_tile_x, p_tile_y, p_mip, slot_idx);
}

void VirtualTexture2D::clear_cache() {
	rebuild_virtual_texture();
}

Ref<Texture2D> VirtualTexture2D::get_page_table_texture() const {
	return page_table_texture;
}

Ref<Texture2D> VirtualTexture2D::get_physical_cache_texture() const {
	return physical_cache_texture;
}

RID VirtualTexture2D::get_rid() const {
	if (physical_cache_texture.is_valid()) {
		return physical_cache_texture->get_rid();
	}
	return RID();
}

Ref<Image> VirtualTexture2D::get_image() const {
	return physical_cache_image;
}

void VirtualTexture2D::set_virtual_size(const Vector2i &p_size) {
	virtual_size.x = CLAMP(p_size.x, 512, 65536);
	virtual_size.y = CLAMP(p_size.y, 512, 65536);
	rebuild_virtual_texture();
}

Vector2i VirtualTexture2D::get_virtual_size() const {
	return virtual_size;
}

void VirtualTexture2D::set_tile_size(int p_size) {
	tile_size = CLAMP(p_size, 32, 1024);
	rebuild_virtual_texture();
}

int VirtualTexture2D::get_tile_size() const {
	return tile_size;
}

void VirtualTexture2D::set_physical_cache_size(int p_size) {
	physical_cache_size = CLAMP(p_size, 512, 8192);
	rebuild_virtual_texture();
}

int VirtualTexture2D::get_physical_cache_size() const {
	return physical_cache_size;
}

void VirtualTexture2D::set_vram_budget_mb(int p_mb) {
	vram_budget_mb = CLAMP(p_mb, 16, 4096);
}

int VirtualTexture2D::get_vram_budget_mb() const {
	return vram_budget_mb;
}

void VirtualTexture2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_virtual_size", "size"), &VirtualTexture2D::set_virtual_size);
	ClassDB::bind_method(D_METHOD("get_virtual_size"), &VirtualTexture2D::get_virtual_size);

	ClassDB::bind_method(D_METHOD("set_tile_size", "size"), &VirtualTexture2D::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &VirtualTexture2D::get_tile_size);

	ClassDB::bind_method(D_METHOD("set_physical_cache_size", "size"), &VirtualTexture2D::set_physical_cache_size);
	ClassDB::bind_method(D_METHOD("get_physical_cache_size"), &VirtualTexture2D::get_physical_cache_size);

	ClassDB::bind_method(D_METHOD("set_vram_budget_mb", "megabytes"), &VirtualTexture2D::set_vram_budget_mb);
	ClassDB::bind_method(D_METHOD("get_vram_budget_mb"), &VirtualTexture2D::get_vram_budget_mb);

	ClassDB::bind_method(D_METHOD("get_virtual_tile_count_x"), &VirtualTexture2D::get_virtual_tile_count_x);
	ClassDB::bind_method(D_METHOD("get_virtual_tile_count_y"), &VirtualTexture2D::get_virtual_tile_count_y);
	ClassDB::bind_method(D_METHOD("get_total_virtual_tiles"), &VirtualTexture2D::get_total_virtual_tiles);
	ClassDB::bind_method(D_METHOD("get_physical_slot_count"), &VirtualTexture2D::get_physical_slot_count);
	ClassDB::bind_method(D_METHOD("get_resident_tile_count"), &VirtualTexture2D::get_resident_tile_count);

	ClassDB::bind_method(D_METHOD("get_theoretical_vram_bytes"), &VirtualTexture2D::get_theoretical_vram_bytes);
	ClassDB::bind_method(D_METHOD("get_actual_vram_bytes"), &VirtualTexture2D::get_actual_vram_bytes);
	ClassDB::bind_method(D_METHOD("get_vram_savings_percentage"), &VirtualTexture2D::get_vram_savings_percentage);
	ClassDB::bind_method(D_METHOD("get_cache_hit_rate"), &VirtualTexture2D::get_cache_hit_rate);

	ClassDB::bind_method(D_METHOD("get_page_table_texture"), &VirtualTexture2D::get_page_table_texture);
	ClassDB::bind_method(D_METHOD("get_physical_cache_texture"), &VirtualTexture2D::get_physical_cache_texture);

	ClassDB::bind_method(D_METHOD("request_tile", "tile_x", "tile_y", "mip_level"), &VirtualTexture2D::request_tile, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("request_region", "uv_rect", "mip_level"), &VirtualTexture2D::request_region, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("request_tiles_around_point", "uv_point", "radius_uv", "mip_level"), &VirtualTexture2D::request_tiles_around_point, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("is_tile_resident", "tile_x", "tile_y", "mip_level"), &VirtualTexture2D::is_tile_resident, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("upload_tile_data", "tile_x", "tile_y", "mip_level", "image"), &VirtualTexture2D::upload_tile_data);
	ClassDB::bind_method(D_METHOD("clear_cache"), &VirtualTexture2D::clear_cache);
	ClassDB::bind_method(D_METHOD("rebuild_virtual_texture"), &VirtualTexture2D::rebuild_virtual_texture);

	ADD_SIGNAL(MethodInfo("tile_requested", PropertyInfo(Variant::INT, "tile_x"), PropertyInfo(Variant::INT, "tile_y"), PropertyInfo(Variant::INT, "mip_level")));

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "virtual_size"), "set_virtual_size", "get_virtual_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size"), "set_tile_size", "get_tile_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "physical_cache_size"), "set_physical_cache_size", "get_physical_cache_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vram_budget_mb"), "set_vram_budget_mb", "get_vram_budget_mb");
}
