/**************************************************************************/
/*  virtual_texture_2d.h                                                  */
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

#pragma once

#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"

class VirtualTexture2D : public Texture2D {
	GDCLASS(VirtualTexture2D, Texture2D);

public:
	struct PhysicalSlot {
		int slot_idx = 0;
		int tile_x = -1;
		int tile_y = -1;
		int mip = -1;
		uint64_t last_access_tick = 0;
		bool is_resident = false;
		bool is_loading = false;
	};

private:
	Vector2i virtual_size = Vector2i(16384, 16384);
	int tile_size = 128;
	int physical_cache_size = 2048;
	int vram_budget_mb = 128;

	Ref<ImageTexture> page_table_texture;
	Ref<ImageTexture> physical_cache_texture;
	Ref<Image> page_table_image;
	Ref<Image> physical_cache_image;

	Vector<PhysicalSlot> physical_slots;
	HashMap<uint64_t, int> tile_to_slot_map;

	uint64_t current_access_tick = 0;
	int cache_requests = 0;
	int cache_hits = 0;

	uint64_t _get_tile_key(int p_tile_x, int p_tile_y, int p_mip) const;
	int _allocate_physical_slot(int p_tile_x, int p_tile_y, int p_mip);
	void _update_page_table_entry(int p_tile_x, int p_tile_y, int p_mip, int p_slot_idx);

protected:
	static void _bind_methods();

public:
	VirtualTexture2D();
	~VirtualTexture2D();

	void set_virtual_size(const Vector2i &p_size);
	Vector2i get_virtual_size() const;

	void set_tile_size(int p_size);
	int get_tile_size() const;

	void set_physical_cache_size(int p_size);
	int get_physical_cache_size() const;

	void set_vram_budget_mb(int p_mb);
	int get_vram_budget_mb() const;

	int get_virtual_tile_count_x() const;
	int get_virtual_tile_count_y() const;
	int get_total_virtual_tiles() const;
	int get_physical_slot_count() const;
	int get_slots_per_axis() const;
	int get_resident_tile_count() const;

	int64_t get_theoretical_vram_bytes() const;
	int64_t get_actual_vram_bytes() const;
	float get_vram_savings_percentage() const;
	float get_cache_hit_rate() const;

	Ref<Texture2D> get_page_table_texture() const;
	Ref<Texture2D> get_physical_cache_texture() const;

	void request_tile(int p_tile_x, int p_tile_y, int p_mip = 0);
	void request_region(const Rect2 &p_uv_rect, int p_mip = 0);
	void request_tiles_around_point(const Vector2 &p_uv_point, float p_radius_uv, int p_mip = 0);
	bool is_tile_resident(int p_tile_x, int p_tile_y, int p_mip = 0) const;
	void upload_tile_data(int p_tile_x, int p_tile_y, int p_mip, const Ref<Image> &p_image);
	void clear_cache();
	void rebuild_virtual_texture();

	// Texture2D overrides
	virtual int get_width() const override { return virtual_size.x; }
	virtual int get_height() const override { return virtual_size.y; }
	virtual RID get_rid() const override;
	virtual bool has_alpha() const override { return false; }
	virtual Ref<Image> get_image() const override;
	virtual bool is_pixel_opaque(int p_x, int p_y) const override { return true; }
};
