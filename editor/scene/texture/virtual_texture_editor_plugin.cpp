/**************************************************************************/
/*  virtual_texture_editor_plugin.cpp                                     */
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

#include "virtual_texture_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"

void VirtualTextureDashboard::_notification(int p_what) {
}

void VirtualTextureDashboard::set_virtual_texture(const Ref<VirtualTexture2D> &p_vtex) {
	vtex = p_vtex;
	if (vtex.is_null() || !info_label) return;

	int64_t theo_mb = vtex->get_theoretical_vram_bytes() / (1024 * 1024);
	int64_t act_mb = vtex->get_actual_vram_bytes() / (1024 * 1024);
	float savings = vtex->get_vram_savings_percentage();
	Vector2i vsz = vtex->get_virtual_size();

	String text = vformat(
		TTR("Virtual Canvas: %d x %d (%d MB)\n"
			"Physical VRAM Cache: %d MB\n"
			"VRAM Savings: %.1f%%"),
		vsz.x, vsz.y, (int)theo_mb, (int)act_mb, savings);

	info_label->set_text(text);
}

void VirtualTextureDashboard::_show_diagnostics() {
	if (vtex.is_null()) return;

	if (!diag_dialog) {
		diag_dialog = memnew(AcceptDialog);
		diag_dialog->set_title(TTR("Virtual Texture (SVT) Diagnostics"));
		EditorInterface::get_singleton()->get_base_control()->add_child(diag_dialog);
	}

	Vector2i vsz = vtex->get_virtual_size();
	int tsz = vtex->get_tile_size();
	int csz = vtex->get_physical_cache_size();
	int tot_tiles = vtex->get_total_virtual_tiles();
	int slots = vtex->get_physical_slot_count();
	int resident = vtex->get_resident_tile_count();
	float hit_rate = vtex->get_cache_hit_rate();
	int64_t theo_mb = vtex->get_theoretical_vram_bytes() / (1024 * 1024);
	int64_t act_mb = vtex->get_actual_vram_bytes() / (1024 * 1024);
	float savings = vtex->get_vram_savings_percentage();

	String text = vformat(
		TTR("Virtual Dimensions: %d x %d pixels\n"
			"Tile Size: %d x %d pixels\n"
			"Total Virtual Tiles: %d\n"
			"Physical Cache Atlas: %d x %d pixels\n"
			"Total Physical Slots: %d\n"
			"Active Resident Tiles: %d\n"
			"Cache Hit Rate: %.1f%%\n\n"
			"Theoretical Uncompressed VRAM: %d MB\n"
			"Actual Physical VRAM Footprint: %d MB\n"
			"Hardware Memory Savings: %.2f%%"),
		vsz.x, vsz.y, tsz, tsz, tot_tiles,
		csz, csz, slots, resident, hit_rate,
		(int)theo_mb, (int)act_mb, savings);

	diag_dialog->set_text(text);
	diag_dialog->popup_centered();
}

void VirtualTextureDashboard::_rebuild_cache() {
	if (vtex.is_valid()) {
		vtex->rebuild_virtual_texture();
		set_virtual_texture(vtex);
	}
}

VirtualTextureDashboard::VirtualTextureDashboard() {
	set_custom_minimum_size(Size2(0, 100 * EDSCALE));

	info_label = memnew(Label);
	info_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	add_child(info_label);

	HBoxContainer *btn_hb = memnew(HBoxContainer);
	btn_hb->set_h_size_flags(SIZE_EXPAND_FILL);

	diag_button = memnew(Button);
	diag_button->set_text(TTR("Diagnostics..."));
	diag_button->set_h_size_flags(SIZE_EXPAND_FILL);
	diag_button->connect(SceneStringName(pressed), callable_mp(this, &VirtualTextureDashboard::_show_diagnostics));
	btn_hb->add_child(diag_button);

	rebuild_button = memnew(Button);
	rebuild_button->set_text(TTR("Rebuild Cache"));
	rebuild_button->set_h_size_flags(SIZE_EXPAND_FILL);
	rebuild_button->connect(SceneStringName(pressed), callable_mp(this, &VirtualTextureDashboard::_rebuild_cache));
	btn_hb->add_child(rebuild_button);

	add_child(btn_hb);
}

bool EditorInspectorPluginVirtualTexture::can_handle(Object *p_object) {
	return Object::cast_to<VirtualTexture2D>(p_object) != nullptr;
}

void EditorInspectorPluginVirtualTexture::parse_begin(Object *p_object) {
	Ref<VirtualTexture2D> vtex = Ref<VirtualTexture2D>(Object::cast_to<VirtualTexture2D>(p_object));
	if (vtex.is_valid()) {
		VirtualTextureDashboard *dash = memnew(VirtualTextureDashboard);
		dash->set_virtual_texture(vtex);
		add_custom_control(dash);
	}
}

VirtualTextureEditorPlugin::VirtualTextureEditorPlugin() {
	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);
}
