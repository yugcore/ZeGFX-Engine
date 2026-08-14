/**************************************************************************/
/*  terrain_3d_editor_plugin.cpp                                          */
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

#include "terrain_3d_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

void Terrain3DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			options_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &Terrain3DEditorPlugin::_menu_option));
		} break;
	}
}

void Terrain3DEditorPlugin::_menu_option(int p_option) {
	if (!terrain) return;

	switch (p_option) {
		case MENU_REBUILD_TERRAIN: {
			terrain->rebuild_terrain();
		} break;
		case MENU_BAKE_COLLISION: {
			terrain->bake_collision();
		} break;
		case MENU_TOGGLE_CHUNK_BOUNDS: {
			terrain->set_show_chunk_bounds(!terrain->is_show_chunk_bounds());
		} break;
		case MENU_TOGGLE_LOD_COLORS: {
			terrain->set_debug_lod_colors(!terrain->is_debug_lod_colors());
		} break;
		case MENU_TERRAIN_STATS: {
			if (!stats_dialog) {
				stats_dialog = memnew(AcceptDialog);
				stats_dialog->set_title(TTR("Terrain3D Statistics"));
				EditorInterface::get_singleton()->get_base_control()->add_child(stats_dialog);
			}
			int w = terrain->get_map_width();
			int h = terrain->get_map_height();
			int chunks = terrain->get_chunk_count();
			int quads = (w > 1 && h > 1) ? (w - 1) * (h - 1) : 0;
			int triangles = quads * 2;
			int vertices = w * h;
			Vector2 size = terrain->get_terrain_size();
			AABB aabb = terrain->get_total_aabb();

			String text = vformat(
				TTR("Resolution: %d x %d vertices\n"
					"Dimensions: %.1f m x %.1f m\n"
					"Total Chunks: %d\n"
					"Total Triangles (LOD 0): %d\n"
					"Total Vertices (LOD 0): %d\n"
					"Cell Size: %.2f m\n"
					"Height Scale: %.2f m\n"
					"LOD Levels: %d\n"
					"LOD Distance Step: %.1f m\n"
					"Max View Distance: %.1f m\n"
					"Skirt Height: %.1f m\n"
					"AABB: Position (%.1f, %.1f, %.1f), Size (%.1f, %.1f, %.1f)"),
				w, h, size.x, size.y, chunks, triangles, vertices,
				terrain->get_cell_size(), terrain->get_height_scale(),
				terrain->get_lod_count(), terrain->get_lod_distance_step(),
				terrain->get_max_view_distance(), terrain->get_skirt_height(),
				aabb.position.x, aabb.position.y, aabb.position.z,
				aabb.size.x, aabb.size.y, aabb.size.z);

			stats_dialog->set_text(text);
			stats_dialog->popup_centered();
		} break;
	}
}

void Terrain3DEditorPlugin::_on_sculpt_toggled(bool p_pressed) {
	sculpt_active = p_pressed;
	brush_mode_opt->set_visible(p_pressed);
	size_hb->set_visible(p_pressed);
	strength_hb->set_visible(p_pressed);
	target_h_hb->set_visible(p_pressed && brush_mode_opt->get_selected() == Terrain3D::BRUSH_FLATTEN);
	update_overlays();
}

void Terrain3DEditorPlugin::_on_brush_mode_changed(int p_mode) {
	target_h_hb->set_visible(sculpt_active && p_mode == Terrain3D::BRUSH_FLATTEN);
	update_overlays();
}

void Terrain3DEditorPlugin::_on_pick_height_pressed() {
	if (has_mouse_hit) {
		brush_height_sb->set_value(brush_pos.y);
	}
}

bool Terrain3DEditorPlugin::_raycast_terrain(Camera3D *p_camera, const Vector2 &p_mouse_pos, Vector3 &r_hit_pos) {
	if (!terrain || !p_camera) return false;

	Vector3 ray_origin = p_camera->project_ray_origin(p_mouse_pos);
	Vector3 ray_dir = p_camera->project_ray_normal(p_mouse_pos);

	Transform3D xform = terrain->get_global_transform();
	Transform3D inv_xform = xform.affine_inverse();
	Vector3 local_origin = inv_xform.xform(ray_origin);
	Vector3 local_dir = inv_xform.basis.xform(ray_dir).normalized();

	float cell_sz = terrain->get_cell_size();
	float step = MAX(0.5f, cell_sz * 0.5f);

	for (float t = 0.5f; t < 5000.0f; t += step) {
		Vector3 p_local = local_origin + local_dir * t;
		Vector3 p_world = xform.xform(p_local);
		float h_world = terrain->sample_height(p_world);

		if (p_world.y <= h_world) {
			float t0 = t - step;
			float t1 = t;
			for (int k = 0; k < 6; ++k) {
				float mid = (t0 + t1) * 0.5f;
				Vector3 mid_w = xform.xform(local_origin + local_dir * mid);
				if (mid_w.y <= terrain->sample_height(mid_w)) {
					t1 = mid;
				} else {
					t0 = mid;
				}
			}
			r_hit_pos = xform.xform(local_origin + local_dir * t1);
			r_hit_pos.y = terrain->sample_height(r_hit_pos);
			return true;
		}
	}

	Plane ground_plane(Vector3(0, 1, 0), xform.origin.y);
	if (ground_plane.intersects_ray(ray_origin, ray_dir, &r_hit_pos)) {
		r_hit_pos.y = terrain->sample_height(r_hit_pos);
		return true;
	}

	return false;
}

EditorPlugin::AfterGUIInput Terrain3DEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!terrain || !sculpt_active) return AFTER_GUI_INPUT_PASS;

	last_camera = p_camera;

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		has_mouse_hit = _raycast_terrain(p_camera, mm->get_position(), brush_pos);
		mouse_screen_pos = mm->get_position();
		update_overlays();

		// Allow 6-DOF freecam / orbit / pan when Right-Click or Middle-Click is pressed
		if (mm->get_button_mask().has_flag(MouseButtonMask::RIGHT) || mm->get_button_mask().has_flag(MouseButtonMask::MIDDLE)) {
			return AFTER_GUI_INPUT_PASS;
		}

		if (is_sculpting && has_mouse_hit) {
			Terrain3D::BrushMode mode = (Terrain3D::BrushMode)brush_mode_opt->get_selected();
			float radius = (float)brush_size_sb->get_value();
			float strength = (float)brush_strength_sb->get_value();
			float target_h = (float)brush_height_sb->get_value();

			terrain->sculpt(brush_pos, radius, strength, mode, target_h);
			return AFTER_GUI_INPUT_STOP;
		}

		return AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Pass through Right and Middle clicks to camera controller for freecam, orbit, and pan
		if (mb->get_button_index() == MouseButton::RIGHT || mb->get_button_index() == MouseButton::MIDDLE) {
			return AFTER_GUI_INPUT_PASS;
		}

		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				has_mouse_hit = _raycast_terrain(p_camera, mb->get_position(), brush_pos);
				mouse_screen_pos = mb->get_position();
				if (has_mouse_hit) {
					Terrain3D::BrushMode mode = (Terrain3D::BrushMode)brush_mode_opt->get_selected();

					// Ctrl + Left Click samples terrain height directly under cursor for Flatten brush
					if (mode == Terrain3D::BRUSH_FLATTEN && (mb->is_ctrl_pressed() || brush_height_sb->get_value() == 0.0)) {
						brush_height_sb->set_value(brush_pos.y);
					}

					is_sculpting = true;
					initial_heights = terrain->get_heights_raw();

					float radius = (float)brush_size_sb->get_value();
					float strength = (float)brush_strength_sb->get_value();
					float target_h = (float)brush_height_sb->get_value();

					terrain->sculpt(brush_pos, radius, strength, mode, target_h);
					update_overlays();
				}
				return AFTER_GUI_INPUT_STOP;
			} else {
				if (is_sculpting) {
					is_sculpting = false;
					terrain->bake_collision();
					Vector<float> final_heights = terrain->get_heights_raw();

					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					ur->create_action(TTR("Sculpt Terrain"));
					ur->add_do_method(terrain, "set_heights_raw", final_heights);
					ur->add_undo_method(terrain, "set_heights_raw", initial_heights);
					ur->commit_action(false);

					update_overlays();
				}
				return AFTER_GUI_INPUT_STOP;
			}
		}
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::BRACKETLEFT) {
			if (k->is_shift_pressed()) {
				brush_strength_sb->set_value(MAX(0.5, brush_strength_sb->get_value() - 1.0));
			} else {
				brush_size_sb->set_value(MAX(1.0, brush_size_sb->get_value() - 2.0));
			}
			update_overlays();
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::BRACKETRIGHT) {
			if (k->is_shift_pressed()) {
				brush_strength_sb->set_value(MIN(100.0, brush_strength_sb->get_value() + 1.0));
			} else {
				brush_size_sb->set_value(MIN(200.0, brush_size_sb->get_value() + 2.0));
			}
			update_overlays();
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_1) {
			brush_mode_opt->select(Terrain3D::BRUSH_RAISE);
			_on_brush_mode_changed(Terrain3D::BRUSH_RAISE);
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_2) {
			brush_mode_opt->select(Terrain3D::BRUSH_LOWER);
			_on_brush_mode_changed(Terrain3D::BRUSH_LOWER);
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_3) {
			brush_mode_opt->select(Terrain3D::BRUSH_SMOOTH);
			_on_brush_mode_changed(Terrain3D::BRUSH_SMOOTH);
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_4) {
			brush_mode_opt->select(Terrain3D::BRUSH_FLATTEN);
			_on_brush_mode_changed(Terrain3D::BRUSH_FLATTEN);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

void Terrain3DEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	if (!sculpt_active || !terrain || !has_mouse_hit) return;

	Camera3D *cam = last_camera;
	if (!cam) {
		Viewport *vp = terrain->get_viewport();
		if (vp) cam = vp->get_camera_3d();
	}
	if (!cam) return;

	float r = (float)brush_size_sb->get_value();
	float strength = (float)brush_strength_sb->get_value();
	String mode_str = brush_mode_opt->get_item_text(brush_mode_opt->get_selected());

	Vector<Vector2> screen_points;
	int segments = 48;
	for (int i = 0; i <= segments; ++i) {
		float angle = (float)i * (float)Math::TAU / (float)segments;
		Vector3 pt = brush_pos + Vector3(Math::cos(angle) * r, 0, Math::sin(angle) * r);
		pt.y = terrain->sample_height(pt) + 0.15f;

		if (!cam->is_position_behind(pt)) {
			screen_points.push_back(cam->unproject_position(pt));
		}
	}

	if (screen_points.size() > 2) {
		// Glow outer line
		p_overlay->draw_polyline(screen_points, Color(0.1f, 0.7f, 1.0f, 0.45f), 5.0f);
		// Inner sharp line
		p_overlay->draw_polyline(screen_points, Color(0.2f, 0.95f, 1.0f, 0.95f), 2.5f);
	}

	if (!cam->is_position_behind(brush_pos)) {
		Vector2 center_2d = cam->unproject_position(brush_pos);
		p_overlay->draw_circle(center_2d, 3.5f, Color(1.0f, 0.85f, 0.2f, 0.95f));

		Ref<Font> font = p_overlay->get_theme_font(SNAME("font"), SNAME("Label"));
		int font_size = p_overlay->get_theme_font_size(SNAME("font_size"), SNAME("Label"));

		String hud_text = vformat("%s | R: %.1fm | Str: %.1fm", mode_str, r, strength);
		if (font.is_valid()) {
			Vector2 text_size = font->get_string_size(hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
			Vector2 badge_pos = center_2d + Vector2(15, -15);
			Rect2 bg_rect(badge_pos.x - 6, badge_pos.y - text_size.y - 2, text_size.x + 12, text_size.y + 8);

			p_overlay->draw_rect(bg_rect, Color(0.08f, 0.10f, 0.14f, 0.85f), true);
			p_overlay->draw_rect(bg_rect, Color(0.25f, 0.85f, 1.0f, 0.8f), false, 1.5f);
			p_overlay->draw_string(font, badge_pos + Vector2(0, -2), hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.95f, 0.95f, 0.95f));
		}
	}
}

void Terrain3DEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<Terrain3D>(p_object);
	if (!terrain && sculpt_active) {
		sculpt_btn->set_pressed(false);
		_on_sculpt_toggled(false);
	}
}

bool Terrain3DEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<Terrain3D>(p_object) != nullptr;
}

void Terrain3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
		if (!p_visible && sculpt_active) {
			sculpt_btn->set_pressed(false);
			_on_sculpt_toggled(false);
		}
	}
}

Terrain3DEditorPlugin::Terrain3DEditorPlugin() {
	toolbar_hb = memnew(HBoxContainer);
	toolbar_hb->hide();

	options_menu = memnew(MenuButton);
	options_menu->set_text(TTR("Terrain3D"));
	options_menu->set_switch_on_hover(true);

	PopupMenu *popup = options_menu->get_popup();
	popup->add_item(TTR("Rebuild Terrain"), MENU_REBUILD_TERRAIN);
	popup->add_item(TTR("Bake Collision"), MENU_BAKE_COLLISION);
	popup->add_separator();
	popup->add_item(TTR("Toggle Chunk Wireframes"), MENU_TOGGLE_CHUNK_BOUNDS);
	popup->add_item(TTR("Toggle LOD Color Debugger"), MENU_TOGGLE_LOD_COLORS);
	popup->add_separator();
	popup->add_item(TTR("Terrain Statistics..."), MENU_TERRAIN_STATS);

	toolbar_hb->add_child(options_menu);
	toolbar_hb->add_child(memnew(VSeparator));

	// 1. Sculpt Mode Toggle Button
	sculpt_btn = memnew(Button);
	sculpt_btn->set_text(TTR("Sculpt Mode"));
	sculpt_btn->set_toggle_mode(true);
	sculpt_btn->set_tooltip_text(TTR("Toggle Terrain Sculpting Mode (Hotkeys: 1=Raise, 2=Lower, 3=Smooth, 4=Flatten)"));
	sculpt_btn->connect(SceneStringName(toggled), callable_mp(this, &Terrain3DEditorPlugin::_on_sculpt_toggled));
	toolbar_hb->add_child(sculpt_btn);

	// 2. Brush Mode Options
	brush_mode_opt = memnew(OptionButton);
	brush_mode_opt->add_item(TTR("Raise (Up)"), Terrain3D::BRUSH_RAISE);
	brush_mode_opt->add_item(TTR("Lower (Down)"), Terrain3D::BRUSH_LOWER);
	brush_mode_opt->add_item(TTR("Smooth"), Terrain3D::BRUSH_SMOOTH);
	brush_mode_opt->add_item(TTR("Flatten"), Terrain3D::BRUSH_FLATTEN);
	brush_mode_opt->set_tooltip_text(TTR("Active Brush Mode (Hotkeys: 1, 2, 3, 4)"));
	brush_mode_opt->connect(SceneStringName(item_selected), callable_mp(this, &Terrain3DEditorPlugin::_on_brush_mode_changed));
	brush_mode_opt->hide();
	toolbar_hb->add_child(brush_mode_opt);

	// 3. Size / Radius Box
	size_hb = memnew(HBoxContainer);
	Label *size_lbl = memnew(Label(TTR("Size:")));
	size_lbl->set_theme_type_variation("HeaderSmall");
	size_hb->add_child(size_lbl);

	brush_size_sb = memnew(SpinBox);
	brush_size_sb->set_min(1.0);
	brush_size_sb->set_max(200.0);
	brush_size_sb->set_value(15.0);
	brush_size_sb->set_step(1.0);
	brush_size_sb->set_suffix(" m");
	brush_size_sb->set_custom_minimum_size(Vector2(75 * EDSCALE, 0));
	brush_size_sb->set_tooltip_text(TTR("Brush Radius in meters (Hotkeys: [ and ])"));
	size_hb->add_child(brush_size_sb);
	size_hb->hide();
	toolbar_hb->add_child(size_hb);

	// 4. Strength Box
	strength_hb = memnew(HBoxContainer);
	Label *strength_lbl = memnew(Label(TTR("Strength:")));
	strength_lbl->set_theme_type_variation("HeaderSmall");
	strength_hb->add_child(strength_lbl);

	brush_strength_sb = memnew(SpinBox);
	brush_strength_sb->set_min(0.1);
	brush_strength_sb->set_max(100.0);
	brush_strength_sb->set_value(5.0);
	brush_strength_sb->set_step(0.5);
	brush_strength_sb->set_suffix(" m");
	brush_strength_sb->set_custom_minimum_size(Vector2(75 * EDSCALE, 0));
	brush_strength_sb->set_tooltip_text(TTR("Brush Strength in meters (Hotkeys: Shift+[ and Shift+])"));
	strength_hb->add_child(brush_strength_sb);
	strength_hb->hide();
	toolbar_hb->add_child(strength_hb);

	// 5. Target Height Box (Flatten Mode Only)
	target_h_hb = memnew(HBoxContainer);
	Label *target_h_lbl = memnew(Label(TTR("Target H:")));
	target_h_lbl->set_theme_type_variation("HeaderSmall");
	target_h_hb->add_child(target_h_lbl);

	brush_height_sb = memnew(SpinBox);
	brush_height_sb->set_min(-500.0);
	brush_height_sb->set_max(2000.0);
	brush_height_sb->set_value(0.0);
	brush_height_sb->set_step(0.5);
	brush_height_sb->set_suffix(" m");
	brush_height_sb->set_custom_minimum_size(Vector2(85 * EDSCALE, 0));
	brush_height_sb->set_tooltip_text(TTR("Flatten Target Elevation (Ctrl+Click on terrain to sample)"));
	target_h_hb->add_child(brush_height_sb);

	pick_h_btn = memnew(Button(TTR("🎯 Pick")));
	pick_h_btn->set_tooltip_text(TTR("Sample elevation under cursor into Target H"));
	pick_h_btn->connect(SceneStringName(pressed), callable_mp(this, &Terrain3DEditorPlugin::_on_pick_height_pressed));
	target_h_hb->add_child(pick_h_btn);

	target_h_hb->hide();
	toolbar_hb->add_child(target_h_hb);

	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);
}

Terrain3DEditorPlugin::~Terrain3DEditorPlugin() {
}
