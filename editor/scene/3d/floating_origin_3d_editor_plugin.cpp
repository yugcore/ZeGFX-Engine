/**************************************************************************/
/*  floating_origin_3d_editor_plugin.cpp                                  */
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

#include "floating_origin_3d_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/control.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/main/viewport.h"

void FloatingOrigin3DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			options_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &FloatingOrigin3DEditorPlugin::_menu_option));
		} break;
	}
}

void FloatingOrigin3DEditorPlugin::_menu_option(int p_option) {
	if (!origin_node) return;

	switch (p_option) {
		case MENU_REBASE_NOW: {
			Camera3D *cam = last_camera;
			if (!cam) {
				Viewport *vp = origin_node->get_viewport();
				if (vp) cam = vp->get_camera_3d();
			}
			if (cam) {
				origin_node->rebase_to_position(cam->get_global_position());
				update_overlays();
			}
		} break;
		case MENU_RESET_UNIVERSE_OFFSET: {
			origin_node->reset_universe_origin();
			update_overlays();
		} break;
		case MENU_ORIGIN_DIAGNOSTICS: {
			if (!diag_dialog) {
				diag_dialog = memnew(AcceptDialog);
				diag_dialog->set_title(TTR("Floating Origin Diagnostics"));
				EditorInterface::get_singleton()->get_base_control()->add_child(diag_dialog);
			}

			Camera3D *cam = last_camera;
			if (!cam) {
				Viewport *vp = origin_node->get_viewport();
				if (vp) cam = vp->get_camera_3d();
			}

			Vector3 cam_local = cam ? cam->get_global_position() : Vector3();
			Vector3 cam_universe = origin_node->local_to_universe(cam_local);
			Vector3 offset = origin_node->get_total_world_offset();
			float thresh = origin_node->get_threshold();
			int shifts = origin_node->get_shift_count();

			String text = vformat(
				TTR("Rebasing Threshold: %.1f m\n"
					"Auto Rebase: %s\n"
					"Total Shifts Executed: %d\n"
					"Accumulated World Offset: (%.1f, %.1f, %.1f) m\n\n"
					"Active Camera Local (32-bit GPU): (%.1f, %.1f, %.1f) m\n"
					"Active Camera Universe (64-bit Real): (%.1f, %.1f, %.1f) m"),
				thresh, origin_node->is_auto_rebase_enabled() ? "Enabled" : "Disabled",
				shifts, offset.x, offset.y, offset.z,
				cam_local.x, cam_local.y, cam_local.z,
				cam_universe.x, cam_universe.y, cam_universe.z);

			diag_dialog->set_text(text);
			diag_dialog->popup_centered();
		} break;
	}
}

void FloatingOrigin3DEditorPlugin::edit(Object *p_object) {
	origin_node = Object::cast_to<FloatingOrigin3D>(p_object);
	update_overlays();
}

bool FloatingOrigin3DEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<FloatingOrigin3D>(p_object) != nullptr;
}

void FloatingOrigin3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
		update_overlays();
	}
}

EditorPlugin::AfterGUIInput FloatingOrigin3DEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!origin_node) return AFTER_GUI_INPUT_PASS;
	last_camera = p_camera;
	update_overlays();
	return AFTER_GUI_INPUT_PASS;
}

void FloatingOrigin3DEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	if (!origin_node) return;

	Camera3D *cam = last_camera;
	if (!cam) {
		Viewport *vp = origin_node->get_viewport();
		if (vp) cam = vp->get_camera_3d();
	}
	if (!cam) return;

	Vector3 cam_pos = cam->get_global_position();
	Vector3 local_origin = Vector3(0, 0, 0);
	float thresh = origin_node->get_threshold();

	// 1. Draw threshold circle around (0,0,0)
	Vector<Vector2> thresh_ring;
	int segments = 48;
	for (int i = 0; i <= segments; ++i) {
		float ang = (float)i * (float)Math::TAU / (float)segments;
		Vector3 pt = local_origin + Vector3(Math::cos(ang) * thresh, 0, Math::sin(ang) * thresh);
		if (!cam->is_position_behind(pt)) {
			thresh_ring.push_back(cam->unproject_position(pt));
		}
	}
	if (thresh_ring.size() > 2) {
		p_overlay->draw_polyline(thresh_ring, Color(1.0f, 0.4f, 0.2f, 0.65f), 2.0f);
	}

	// 2. Floating Viewport HUD Badge
	Ref<Font> font = p_overlay->get_theme_font(SNAME("font"), SNAME("Label"));
	int font_size = p_overlay->get_theme_font_size(SNAME("font_size"), SNAME("Label"));

	Vector3 universe_pos = origin_node->local_to_universe(cam_pos);
	int shifts = origin_node->get_shift_count();

	String hud_text = vformat("FloatingOrigin3D | Local: (%.0f, %.0f, %.0f) | Universe: (%.0f, %.0f, %.0f) | Shifts: %d",
		cam_pos.x, cam_pos.y, cam_pos.z,
		universe_pos.x, universe_pos.y, universe_pos.z,
		shifts);

	if (font.is_valid()) {
		Vector2 text_sz = font->get_string_size(hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		Vector2 pos(20, p_overlay->get_size().y - 65);
		Rect2 bg(pos.x - 8, pos.y - text_sz.y - 2, text_sz.x + 16, text_sz.y + 8);

		p_overlay->draw_rect(bg, Color(0.08f, 0.10f, 0.14f, 0.85f), true);
		p_overlay->draw_rect(bg, Color(1.0f, 0.5f, 0.2f, 0.8f), false, 1.5f);
		p_overlay->draw_string(font, pos, hud_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.95f, 0.95f, 0.95f));
	}
}

FloatingOrigin3DEditorPlugin::FloatingOrigin3DEditorPlugin() {
	toolbar_hb = memnew(HBoxContainer);
	toolbar_hb->hide();

	options_menu = memnew(MenuButton);
	options_menu->set_text(TTR("FloatingOrigin3D"));
	options_menu->set_switch_on_hover(true);

	PopupMenu *popup = options_menu->get_popup();
	popup->add_item(TTR("Rebase Origin Now (Camera Position)"), MENU_REBASE_NOW);
	popup->add_item(TTR("Reset Universe Offset"), MENU_RESET_UNIVERSE_OFFSET);
	popup->add_separator();
	popup->add_item(TTR("Floating Origin Diagnostics..."), MENU_ORIGIN_DIAGNOSTICS);

	toolbar_hb->add_child(options_menu);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);
}

FloatingOrigin3DEditorPlugin::~FloatingOrigin3DEditorPlugin() {
}
