/**************************************************************************/
/*  foliage_3d_editor_plugin.cpp                                          */
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

#include "foliage_3d_editor_plugin.h"

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
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

void Foliage3DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			options_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &Foliage3DEditorPlugin::_menu_option));
		} break;
	}
}

void Foliage3DEditorPlugin::_menu_option(int p_option) {
	if (!foliage) return;

	switch (p_option) {
		case MENU_CONFORM_TO_TERRAIN: {
			EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
			PackedFloat32Array before_data = foliage->get_foliage_data();
			foliage->conform_to_terrain();
			PackedFloat32Array after_data = foliage->get_foliage_data();

			ur->create_action(TTR("Conform Foliage to Terrain"));
			ur->add_do_method(foliage, "set_foliage_data", after_data);
			ur->add_undo_method(foliage, "set_foliage_data", before_data);
			ur->commit_action(false);
			_update_palette_ui();
		} break;
		case MENU_PROCEDURAL_SCATTER: {
			EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
			PackedFloat32Array before_data = foliage->get_foliage_data();
			Vector<int> active_types = _get_active_type_filter();
			if (active_types.is_empty()) {
				foliage->scatter_all();
			} else {
				for (int t : active_types) {
					foliage->scatter_type(t);
				}
			}
			PackedFloat32Array after_data = foliage->get_foliage_data();

			ur->create_action(TTR("Procedural Scatter Foliage"));
			ur->add_do_method(foliage, "set_foliage_data", after_data);
			ur->add_undo_method(foliage, "set_foliage_data", before_data);
			ur->commit_action(false);
			_update_palette_ui();
		} break;
		case MENU_CLEAR_ALL: {
			EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
			PackedFloat32Array before_data = foliage->get_foliage_data();
			foliage->clear_all();
			PackedFloat32Array after_data = foliage->get_foliage_data();

			ur->create_action(TTR("Clear All Foliage"));
			ur->add_do_method(foliage, "set_foliage_data", after_data);
			ur->add_undo_method(foliage, "set_foliage_data", before_data);
			ur->commit_action(false);
			_update_palette_ui();
		} break;
		case MENU_FOLIAGE_STATS: {
			if (!stats_dialog) {
				stats_dialog = memnew(AcceptDialog);
				stats_dialog->set_title(TTR("Foliage3D Statistics"));
				EditorInterface::get_singleton()->get_base_control()->add_child(stats_dialog);
			}
			int total_instances = foliage->get_total_instance_count();
			int type_count = foliage->get_foliage_type_count();
			int chunks = foliage->get_chunk_count();
			AABB aabb = foliage->get_total_aabb();

			String text = vformat(
				TTR("Total Foliage Instances: %d\n"
					"Active Foliage Types: %d\n"
					"Spatial Chunks: %d\n"
					"Chunk Grid Size: %.1f m\n"
					"AABB: Position (%.1f, %.1f, %.1f), Size (%.1f, %.1f, %.1f)\n\n"
					"--- Instance Breakdown ---"),
				total_instances, type_count, chunks, foliage->get_chunk_size(),
				aabb.position.x, aabb.position.y, aabb.position.z,
				aabb.size.x, aabb.size.y, aabb.size.z);

			for (int i = 0; i < type_count; ++i) {
				Ref<FoliageType3D> ft = foliage->get_foliage_type(i);
				String name = ft.is_valid() ? ft->get_type_name() : vformat("Type %d", i);
				int count = foliage->get_type_instance_count(i);
				text += vformat("\n • %s: %d instances", name, count);
			}

			stats_dialog->set_text(text);
			stats_dialog->popup_centered();
		} break;
	}
}

void Foliage3DEditorPlugin::_on_paint_toggled(bool p_pressed) {
	brush_active = p_pressed;
	brush_mode_opt->set_visible(p_pressed);
	size_hb->set_visible(p_pressed);
	density_hb->set_visible(p_pressed);
	erase_filter_hb->set_visible(p_pressed && brush_mode_opt->get_selected() == MODE_ERASE);
	palette_panel->set_visible(p_pressed);
	_update_palette_ui();
	update_overlays();
}

void Foliage3DEditorPlugin::_on_brush_mode_changed(int p_mode) {
	erase_filter_hb->set_visible(brush_active && p_mode == MODE_ERASE);
	density_hb->set_visible(brush_active && p_mode == MODE_PAINT);
	update_overlays();
}

void Foliage3DEditorPlugin::_update_palette_ui() {
	if (!foliage || !palette_list) return;

	palette_list->clear();
	int count = foliage->get_foliage_type_count();

	for (int i = 0; i < count; ++i) {
		Ref<FoliageType3D> ft = foliage->get_foliage_type(i);
		if (!ft.is_valid()) continue;

		String label = vformat("[%s] %s (%d)", ft->is_enabled() ? "X" : " ", ft->get_type_name(), foliage->get_type_instance_count(i));
		palette_list->add_item(label);
		if (ft->is_enabled()) {
			palette_list->select(palette_list->get_item_count() - 1, false);
		}
	}

	if (palette_summary_lbl) {
		palette_summary_lbl->set_text(vformat(TTR("Total: %d instances in %d chunks"), foliage->get_total_instance_count(), foliage->get_chunk_count()));
	}
}

void Foliage3DEditorPlugin::_on_palette_item_selected(int p_index) {
	if (!foliage || p_index < 0 || p_index >= foliage->get_foliage_type_count()) return;
	Ref<FoliageType3D> ft = foliage->get_foliage_type(p_index);
	if (ft.is_valid()) {
		ft->set_enabled(!ft->is_enabled());
		_update_palette_ui();
	}
}

void Foliage3DEditorPlugin::_on_palette_add_pressed() {
	if (!foliage) return;
	Ref<FoliageType3D> new_type = memnew(FoliageType3D);
	new_type->set_type_name(vformat("Foliage_%d", foliage->get_foliage_type_count() + 1));
	foliage->add_foliage_type(new_type);
	_update_palette_ui();
}

void Foliage3DEditorPlugin::_on_palette_remove_pressed() {
	if (!foliage) return;
	Vector<int> sel = palette_list->get_selected_items();
	if (!sel.is_empty()) {
		foliage->remove_foliage_type(sel[0]);
		_update_palette_ui();
	}
}

void Foliage3DEditorPlugin::_on_palette_select_all_pressed(bool p_select) {
	if (!foliage) return;
	for (int i = 0; i < foliage->get_foliage_type_count(); ++i) {
		Ref<FoliageType3D> ft = foliage->get_foliage_type(i);
		if (ft.is_valid()) {
			ft->set_enabled(p_select);
		}
	}
	_update_palette_ui();
}

Vector<int> Foliage3DEditorPlugin::_get_active_type_filter() const {
	Vector<int> active;
	if (!foliage) return active;
	for (int i = 0; i < foliage->get_foliage_type_count(); ++i) {
		Ref<FoliageType3D> ft = foliage->get_foliage_type(i);
		if (ft.is_valid() && ft->is_enabled()) {
			active.push_back(i);
		}
	}
	return active;
}

bool Foliage3DEditorPlugin::_raycast_surface(Camera3D *p_camera, const Vector2 &p_mouse_pos, Vector3 &r_hit_pos) {
	if (!foliage || !p_camera) return false;

	Vector3 ray_origin = p_camera->project_ray_origin(p_mouse_pos);
	Vector3 ray_dir = p_camera->project_ray_normal(p_mouse_pos);

	// Try raycasting against Terrain3D if present
	NodePath tpath = foliage->get_terrain_path();
	Terrain3D *terrain = nullptr;
	if (!tpath.is_empty()) {
		terrain = Object::cast_to<Terrain3D>(foliage->get_node_or_null(tpath));
	} else if (foliage->get_parent()) {
		for (int i = 0; i < foliage->get_parent()->get_child_count(); ++i) {
			Terrain3D *t = Object::cast_to<Terrain3D>(foliage->get_parent()->get_child(i));
			if (t) {
				terrain = t;
				break;
			}
		}
	}

	if (terrain) {
		Transform3D xform = terrain->get_global_transform();
		Transform3D inv_xform = xform.affine_inverse();
		Vector3 local_origin = inv_xform.xform(ray_origin);
		Vector3 local_dir = inv_xform.basis.xform(ray_dir).normalized();

		float cell_sz = terrain->get_cell_size();
		float step = MAX(0.5f, cell_sz * 0.5f);

		for (float t = 0.5f; t < 5000.0f; t += step) {
			Vector3 p_local = local_origin + local_dir * t;
			Vector3 p_world = xform.xform(p_local);
			float terrain_h = terrain->sample_height(p_world);

			if (p_world.y <= terrain_h + 0.1f) {
				r_hit_pos = Vector3(p_world.x, terrain_h, p_world.z);
				return true;
			}
		}
	}

	// Fallback to ground plane Y = 0
	if (Math::abs(ray_dir.y) > 1e-4f) {
		float t = -ray_origin.y / ray_dir.y;
		if (t >= 0.0f) {
			r_hit_pos = ray_origin + ray_dir * t;
			return true;
		}
	}

	return false;
}

EditorPlugin::AfterGUIInput Foliage3DEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!brush_active || !foliage) {
		return AFTER_GUI_INPUT_PASS;
	}

	last_camera = p_camera;

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		mouse_screen_pos = mm->get_position();
		has_mouse_hit = _raycast_surface(p_camera, mouse_screen_pos, brush_pos);
		update_overlays();

		if (is_stroking && has_mouse_hit) {
			float radius = (float)brush_size_sb->get_value();
			float density_mult = (float)brush_density_sb->get_value();
			BrushMode mode = (BrushMode)brush_mode_opt->get_selected();
			Vector<int> active_types = _get_active_type_filter();

			if (mode == MODE_PAINT) {
				foliage->paint_instances(brush_pos, radius, density_mult * 0.2f, active_types);
			} else if (mode == MODE_ERASE) {
				bool erase_all = erase_filter_opt->get_selected() == 1;
				Vector<int> filter = erase_all ? Vector<int>() : active_types;
				foliage->erase_instances(brush_pos, radius, filter);
			}
			return AFTER_GUI_INPUT_STOP;
		}
		return AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::RIGHT || mb->get_button_index() == MouseButton::MIDDLE) {
			return AFTER_GUI_INPUT_PASS;
		}

		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				has_mouse_hit = _raycast_surface(p_camera, mb->get_position(), brush_pos);
				mouse_screen_pos = mb->get_position();

				if (has_mouse_hit) {
					is_stroking = true;
					initial_foliage_data = foliage->get_foliage_data();

					float radius = (float)brush_size_sb->get_value();
					float density_mult = (float)brush_density_sb->get_value();
					BrushMode mode = (BrushMode)brush_mode_opt->get_selected();
					Vector<int> active_types = _get_active_type_filter();

					if (mode == MODE_PAINT) {
						foliage->paint_instances(brush_pos, radius, density_mult, active_types);
					} else if (mode == MODE_ERASE) {
						bool erase_all = erase_filter_opt->get_selected() == 1;
						Vector<int> filter = erase_all ? Vector<int>() : active_types;
						foliage->erase_instances(brush_pos, radius, filter);
					}
					update_overlays();
				}
				return AFTER_GUI_INPUT_STOP;
			} else {
				if (is_stroking) {
					is_stroking = false;
					PackedFloat32Array final_data = foliage->get_foliage_data();

					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					ur->create_action(TTR("Foliage Paint Stroke"));
					ur->add_do_method(foliage, "set_foliage_data", final_data);
					ur->add_undo_method(foliage, "set_foliage_data", initial_foliage_data);
					ur->commit_action(false);

					_update_palette_ui();
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
				brush_density_sb->set_value(MAX(0.1, brush_density_sb->get_value() - 0.2));
			} else {
				brush_size_sb->set_value(MAX(1.0, brush_size_sb->get_value() - 2.0));
			}
			update_overlays();
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::BRACKETRIGHT) {
			if (k->is_shift_pressed()) {
				brush_density_sb->set_value(MIN(10.0, brush_density_sb->get_value() + 0.2));
			} else {
				brush_size_sb->set_value(MIN(150.0, brush_size_sb->get_value() + 2.0));
			}
			update_overlays();
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_1) {
			brush_mode_opt->select(MODE_PAINT);
			_on_brush_mode_changed(MODE_PAINT);
			return AFTER_GUI_INPUT_STOP;
		} else if (k->get_keycode() == Key::KEY_2) {
			brush_mode_opt->select(MODE_ERASE);
			_on_brush_mode_changed(MODE_ERASE);
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

void Foliage3DEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	if (!brush_active || !foliage || !has_mouse_hit) return;

	Camera3D *cam = last_camera;
	if (!cam) {
		Viewport *vp = foliage->get_viewport();
		if (vp) cam = vp->get_camera_3d();
	}
	if (!cam) return;

	float r = (float)brush_size_sb->get_value();
	BrushMode mode = (BrushMode)brush_mode_opt->get_selected();

	Color circle_color = (mode == MODE_PAINT) ? Color(0.2f, 0.95f, 0.45f, 0.95f) : Color(1.0f, 0.35f, 0.25f, 0.95f);
	Color glow_color = (mode == MODE_PAINT) ? Color(0.1f, 0.75f, 0.35f, 0.40f) : Color(0.9f, 0.2f, 0.15f, 0.40f);

	Vector<Vector2> screen_points;
	int segments = 48;

	NodePath tpath = foliage->get_terrain_path();
	Terrain3D *terrain = Object::cast_to<Terrain3D>(foliage->get_node_or_null(tpath));

	for (int i = 0; i <= segments; ++i) {
		float angle = (float)i * (float)Math::TAU / (float)segments;
		Vector3 pt = brush_pos + Vector3(Math::cos(angle) * r, 0, Math::sin(angle) * r);
		if (terrain) {
			pt.y = terrain->sample_height(pt) + 0.15f;
		} else {
			pt.y = brush_pos.y + 0.15f;
		}

		if (!cam->is_position_behind(pt)) {
			screen_points.push_back(cam->unproject_position(pt));
		}
	}

	if (screen_points.size() > 2) {
		p_overlay->draw_polyline(screen_points, glow_color, 5.0f);
		p_overlay->draw_polyline(screen_points, circle_color, 2.5f);
	}

	if (!cam->is_position_behind(brush_pos)) {
		Vector2 center_2d = cam->unproject_position(brush_pos);
		p_overlay->draw_circle(center_2d, 3.5f, circle_color);

		Ref<Font> font = p_overlay->get_theme_font(SNAME("font"), SNAME("Label"));
		int font_size = p_overlay->get_theme_font_size(SNAME("font_size"), SNAME("Label"));

		String hud_text = vformat("%s | R: %.1fm | Types: %d | Total: %d",
			(mode == MODE_PAINT) ? "PAINT" : "ERASE",
			r, _get_active_type_filter().size(), foliage->get_total_instance_count());

		if (font.is_valid()) {
			Vector2 text_size = font->get_string_size(hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
			Vector2 badge_pos = center_2d + Vector2(15, -15);
			Rect2 bg_rect(badge_pos.x - 6, badge_pos.y - text_size.y - 2, text_size.x + 12, text_size.y + 8);

			p_overlay->draw_rect(bg_rect, Color(0.08f, 0.10f, 0.14f, 0.85f), true);
			p_overlay->draw_rect(bg_rect, circle_color, false, 1.5f);
			p_overlay->draw_string(font, badge_pos + Vector2(0, -2), hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.95f, 0.95f, 0.95f));
		}
	}
}

void Foliage3DEditorPlugin::edit(Object *p_object) {
	foliage = Object::cast_to<Foliage3D>(p_object);
	if (!foliage && brush_active) {
		paint_mode_btn->set_pressed(false);
		_on_paint_toggled(false);
	}
	_update_palette_ui();
}

bool Foliage3DEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<Foliage3D>(p_object) != nullptr;
}

void Foliage3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
		if (!p_visible && brush_active) {
			paint_mode_btn->set_pressed(false);
			_on_paint_toggled(false);
		}
	}
	if (palette_panel) {
		palette_panel->set_visible(p_visible && brush_active);
	}
}

Foliage3DEditorPlugin::Foliage3DEditorPlugin() {
	toolbar_hb = memnew(HBoxContainer);
	toolbar_hb->hide();

	options_menu = memnew(MenuButton);
	options_menu->set_text(TTR("Foliage3D"));
	options_menu->set_switch_on_hover(true);

	PopupMenu *popup = options_menu->get_popup();
	popup->add_item(TTR("Conform to Terrain"), MENU_CONFORM_TO_TERRAIN);
	popup->add_item(TTR("Procedural Scatter (Active Types)"), MENU_PROCEDURAL_SCATTER);
	popup->add_separator();
	popup->add_item(TTR("Clear All Foliage"), MENU_CLEAR_ALL);
	popup->add_separator();
	popup->add_item(TTR("Foliage Statistics"), MENU_FOLIAGE_STATS);

	toolbar_hb->add_child(options_menu);
	toolbar_hb->add_child(memnew(VSeparator));

	// Paint Toggle Button
	paint_mode_btn = memnew(Button);
	paint_mode_btn->set_text(TTR("Paint Foliage"));
	paint_mode_btn->set_toggle_mode(true);
	paint_mode_btn->connect(SceneStringName(toggled), callable_mp(this, &Foliage3DEditorPlugin::_on_paint_toggled));
	toolbar_hb->add_child(paint_mode_btn);

	// Brush Mode (Paint / Erase)
	brush_mode_opt = memnew(OptionButton);
	brush_mode_opt->add_item(TTR("Paint Brush"), MODE_PAINT);
	brush_mode_opt->add_item(TTR("Erase Brush"), MODE_ERASE);
	brush_mode_opt->connect(SceneStringName(item_selected), callable_mp(this, &Foliage3DEditorPlugin::_on_brush_mode_changed));
	brush_mode_opt->hide();
	toolbar_hb->add_child(brush_mode_opt);

	// Brush Size
	size_hb = memnew(HBoxContainer);
	size_hb->hide();
	Label *size_lbl = memnew(Label);
	size_lbl->set_text(TTR("Radius:"));
	size_hb->add_child(size_lbl);

	brush_size_sb = memnew(SpinBox);
	brush_size_sb->set_min(1.0);
	brush_size_sb->set_max(150.0);
	brush_size_sb->set_step(1.0);
	brush_size_sb->set_value(10.0);
	brush_size_sb->set_suffix("m");
	size_hb->add_child(brush_size_sb);
	toolbar_hb->add_child(size_hb);

	// Brush Density
	density_hb = memnew(HBoxContainer);
	density_hb->hide();
	Label *density_lbl = memnew(Label);
	density_lbl->set_text(TTR("Density:"));
	density_hb->add_child(density_lbl);

	brush_density_sb = memnew(SpinBox);
	brush_density_sb->set_min(0.1);
	brush_density_sb->set_max(10.0);
	brush_density_sb->set_step(0.1);
	brush_density_sb->set_value(1.0);
	brush_density_sb->set_suffix("x");
	density_hb->add_child(brush_density_sb);
	toolbar_hb->add_child(density_hb);

	// Erase Filter
	erase_filter_hb = memnew(HBoxContainer);
	erase_filter_hb->hide();
	erase_filter_opt = memnew(OptionButton);
	erase_filter_opt->add_item(TTR("Erase Selected Types"), 0);
	erase_filter_opt->add_item(TTR("Erase All Types"), 1);
	erase_filter_hb->add_child(erase_filter_opt);
	toolbar_hb->add_child(erase_filter_hb);

	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);

	// Palette Dock Panel (Bottom)
	palette_panel = memnew(PanelContainer);
	palette_panel->set_custom_minimum_size(Size2(0, 180));
	palette_panel->hide();

	VBoxContainer *palette_vb = memnew(VBoxContainer);
	palette_panel->add_child(palette_vb);

	HBoxContainer *palette_header = memnew(HBoxContainer);
	Label *pal_title = memnew(Label);
	pal_title->set_text(TTR("Foliage Palette"));
	palette_header->add_child(pal_title);
	palette_header->add_child(memnew(VSeparator));

	palette_add_btn = memnew(Button);
	palette_add_btn->set_text(TTR("+ Add Type"));
	palette_add_btn->connect(SceneStringName(pressed), callable_mp(this, &Foliage3DEditorPlugin::_on_palette_add_pressed));
	palette_header->add_child(palette_add_btn);

	palette_remove_btn = memnew(Button);
	palette_remove_btn->set_text(TTR("- Remove"));
	palette_remove_btn->connect(SceneStringName(pressed), callable_mp(this, &Foliage3DEditorPlugin::_on_palette_remove_pressed));
	palette_header->add_child(palette_remove_btn);

	palette_select_all_btn = memnew(Button);
	palette_select_all_btn->set_text(TTR("Select All"));
	palette_select_all_btn->connect(SceneStringName(pressed), callable_mp(this, &Foliage3DEditorPlugin::_on_palette_select_all_pressed).bind(true));
	palette_header->add_child(palette_select_all_btn);

	palette_deselect_all_btn = memnew(Button);
	palette_deselect_all_btn->set_text(TTR("Deselect All"));
	palette_deselect_all_btn->connect(SceneStringName(pressed), callable_mp(this, &Foliage3DEditorPlugin::_on_palette_select_all_pressed).bind(false));
	palette_header->add_child(palette_deselect_all_btn);

	palette_summary_lbl = memnew(Label);
	palette_summary_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	palette_summary_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	palette_header->add_child(palette_summary_lbl);

	palette_vb->add_child(palette_header);

	palette_list = memnew(ItemList);
	palette_list->set_select_mode(ItemList::SELECT_MULTI);
	palette_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	palette_list->connect(SceneStringName(item_selected), callable_mp(this, &Foliage3DEditorPlugin::_on_palette_item_selected));
	palette_vb->add_child(palette_list);

	add_control_to_bottom_panel(palette_panel, TTR("Foliage Palette"));
}

Foliage3DEditorPlugin::~Foliage3DEditorPlugin() {
}
