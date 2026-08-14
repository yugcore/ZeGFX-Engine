/**************************************************************************/
/*  world_partition_3d_editor_plugin.cpp                                  */
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
/* EXPRESS OR IMPLIED/ INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "world_partition_3d_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/control.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/main/viewport.h"

void WorldPartition3DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			options_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &WorldPartition3DEditorPlugin::_menu_option));
		} break;
	}
}

void WorldPartition3DEditorPlugin::_menu_option(int p_option) {
	if (!partition) return;

	switch (p_option) {
		case MENU_LOAD_ALL: {
			partition->load_all_cells();
			update_overlays();
		} break;
		case MENU_UNLOAD_ALL: {
			partition->unload_all_cells();
			update_overlays();
		} break;
		case MENU_FORCE_REFRESH: {
			partition->force_refresh();
			update_overlays();
		} break;
		case MENU_TOGGLE_DEBUG_DRAW: {
			partition->set_debug_draw_cells(!partition->is_debug_draw_cells());
			update_overlays();
		} break;
		case MENU_PARTITION_STATS: {
			if (!stats_dialog) {
				stats_dialog = memnew(AcceptDialog);
				stats_dialog->set_title(TTR("World Partition Statistics"));
				EditorInterface::get_singleton()->get_base_control()->add_child(stats_dialog);
			}

			int loaded = partition->get_loaded_cell_count();
			int loading = partition->get_loading_cell_count();
			float c_sz = partition->get_cell_size();
			float l_dist = partition->get_loading_distance();
			float u_dist = partition->get_unloading_distance();

			String text = vformat(
				TTR("Cell Size: %.1f m\n"
					"Loading Radius: %.1f m\n"
					"Unloading Radius: %.1f m\n"
					"Active Loaded Cells: %d\n"
					"In-Flight Background Loads: %d\n"
					"Cell Pattern: %s\n"
					"Max Concurrent Threads: %d"),
				c_sz, l_dist, u_dist, loaded, loading,
				partition->get_cell_path_pattern(),
				partition->get_max_concurrent_loads());

			stats_dialog->set_text(text);
			stats_dialog->popup_centered();
		} break;
	}
}

void WorldPartition3DEditorPlugin::edit(Object *p_object) {
	partition = Object::cast_to<WorldPartition3D>(p_object);
	update_overlays();
}

bool WorldPartition3DEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<WorldPartition3D>(p_object) != nullptr;
}

void WorldPartition3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
		update_overlays();
	}
}

EditorPlugin::AfterGUIInput WorldPartition3DEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!partition) return AFTER_GUI_INPUT_PASS;
	last_camera = p_camera;
	update_overlays();
	return AFTER_GUI_INPUT_PASS;
}

void WorldPartition3DEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	if (!partition || !partition->is_debug_draw_cells()) return;

	Camera3D *cam = last_camera;
	if (!cam) {
		Viewport *vp = partition->get_viewport();
		if (vp) cam = vp->get_camera_3d();
	}
	if (!cam) return;

	Vector3 cam_pos = cam->get_global_position();
	float cell_sz = partition->get_cell_size();
	float load_dist = partition->get_loading_distance();
	float unload_dist = partition->get_unloading_distance();
	Vector2i center_coord = partition->world_to_cell_coord(cam_pos);
	int r_cells = (int)Math::ceil(unload_dist / cell_sz) + 1;

	float load_dist_sq = load_dist * load_dist;
	float unload_dist_sq = unload_dist * unload_dist;

	// 1. Draw Visible Cell Grid Squares
	for (int cz = center_coord.y - r_cells; cz <= center_coord.y + r_cells; ++cz) {
		for (int cx = center_coord.x - r_cells; cx <= center_coord.x + r_cells; ++cx) {
			Vector3 p0 = partition->cell_coord_to_world(cx, cz);
			Vector3 p1 = p0 + Vector3(cell_sz, 0, 0);
			Vector3 p2 = p0 + Vector3(cell_sz, 0, cell_sz);
			Vector3 p3 = p0 + Vector3(0, 0, cell_sz);

			Vector3 cell_center = p0 + Vector3(cell_sz * 0.5f, 0.0f, cell_sz * 0.5f);
			float d2 = Vector2(cell_center.x - cam_pos.x, cell_center.z - cam_pos.z).length_squared();

			if (d2 > unload_dist_sq * 1.5f) continue;

			if (cam->is_position_behind(p0) && cam->is_position_behind(p1) &&
				cam->is_position_behind(p2) && cam->is_position_behind(p3)) {
				continue;
			}

			Vector2 v0 = cam->unproject_position(p0);
			Vector2 v1 = cam->unproject_position(p1);
			Vector2 v2 = cam->unproject_position(p2);
			Vector2 v3 = cam->unproject_position(p3);

			bool is_loaded = partition->is_cell_loaded(cx, cz);
			bool is_loading = partition->is_cell_loading(cx, cz);

			Color line_col;
			float line_w = 1.0f;

			if (is_loaded) {
				line_col = Color(0.25f, 0.95f, 0.35f, 0.85f);
				line_w = 2.0f;
			} else if (is_loading) {
				line_col = Color(1.0f, 0.85f, 0.2f, 0.9f);
				line_w = 2.0f;
			} else if (d2 <= load_dist_sq) {
				line_col = Color(0.2f, 0.75f, 1.0f, 0.6f);
			} else {
				line_col = Color(0.5f, 0.55f, 0.65f, 0.25f);
			}

			p_overlay->draw_line(v0, v1, line_col, line_w);
			p_overlay->draw_line(v1, v2, line_col, line_w);
			p_overlay->draw_line(v2, v3, line_col, line_w);
			p_overlay->draw_line(v3, v0, line_col, line_w);
		}
	}

	// 2. Draw Loading Distance Circle around camera
	Vector3 center_ground = cam_pos;
	center_ground.y = partition->get_global_position().y;

	Vector<Vector2> load_ring_pts;
	int segments = 48;
	for (int i = 0; i <= segments; ++i) {
		float ang = (float)i * (float)Math::TAU / (float)segments;
		Vector3 pt = center_ground + Vector3(Math::cos(ang) * load_dist, 0, Math::sin(ang) * load_dist);
		if (!cam->is_position_behind(pt)) {
			load_ring_pts.push_back(cam->unproject_position(pt));
		}
	}
	if (load_ring_pts.size() > 2) {
		p_overlay->draw_polyline(load_ring_pts, Color(0.2f, 0.8f, 1.0f, 0.75f), 2.5f);
	}

	// 3. Draw Unloading Hysteresis Circle
	Vector<Vector2> unload_ring_pts;
	for (int i = 0; i <= segments; ++i) {
		float ang = (float)i * (float)Math::TAU / (float)segments;
		Vector3 pt = center_ground + Vector3(Math::cos(ang) * unload_dist, 0, Math::sin(ang) * unload_dist);
		if (!cam->is_position_behind(pt)) {
			unload_ring_pts.push_back(cam->unproject_position(pt));
		}
	}
	if (unload_ring_pts.size() > 2) {
		p_overlay->draw_polyline(unload_ring_pts, Color(1.0f, 0.5f, 0.2f, 0.45f), 1.5f);
	}

	// 4. Floating HUD Badge
	Ref<Font> font = p_overlay->get_theme_font(SNAME("font"), SNAME("Label"));
	int font_size = p_overlay->get_theme_font_size(SNAME("font_size"), SNAME("Label"));

	int loaded_cnt = partition->get_loaded_cell_count();
	int loading_cnt = partition->get_loading_cell_count();
	String hud_text = vformat("WorldPartition3D | Cell: (%d, %d) | Active: %d | Loading: %d | Cell: %.0fm",
		center_coord.x, center_coord.y, loaded_cnt, loading_cnt, cell_sz);

	if (font.is_valid()) {
		Vector2 text_sz = font->get_string_size(hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		Vector2 pos(20, p_overlay->get_size().y - 35);
		Rect2 bg(pos.x - 8, pos.y - text_sz.y - 2, text_sz.x + 16, text_sz.y + 8);

		p_overlay->draw_rect(bg, Color(0.08f, 0.10f, 0.14f, 0.85f), true);
		p_overlay->draw_rect(bg, Color(0.2f, 0.8f, 1.0f, 0.75f), false, 1.5f);
		p_overlay->draw_string(font, pos, hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.95f, 0.95f, 0.95f));
	}
}

WorldPartition3DEditorPlugin::WorldPartition3DEditorPlugin() {
	toolbar_hb = memnew(HBoxContainer);
	toolbar_hb->hide();

	options_menu = memnew(MenuButton);
	options_menu->set_text(TTR("WorldPartition3D"));
	options_menu->set_switch_on_hover(true);

	PopupMenu *popup = options_menu->get_popup();
	popup->add_item(TTR("Load All Cells (Editor)"), MENU_LOAD_ALL);
	popup->add_item(TTR("Unload All Cells"), MENU_UNLOAD_ALL);
	popup->add_item(TTR("Force Refresh Streaming"), MENU_FORCE_REFRESH);
	popup->add_separator();
	popup->add_item(TTR("Toggle Cell Grid Gizmo"), MENU_TOGGLE_DEBUG_DRAW);
	popup->add_separator();
	popup->add_item(TTR("Partition Statistics..."), MENU_PARTITION_STATS);

	toolbar_hb->add_child(options_menu);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);
}

WorldPartition3DEditorPlugin::~WorldPartition3DEditorPlugin() {
}
