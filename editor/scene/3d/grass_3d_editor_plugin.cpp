/**************************************************************************/
/*  grass_3d_editor_plugin.cpp                                            */
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

#include "grass_3d_editor_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"

void Grass3DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process(true);
			options_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &Grass3DEditorPlugin::_menu_option));
		} break;

		case NOTIFICATION_PROCESS: {
			Node3DEditorViewport *evp = Node3DEditor::get_singleton() ? Node3DEditor::get_singleton()->get_editor_viewport(0) : nullptr;
			if (evp && evp->get_camera_3d()) {
				Vector3 cam_pos = evp->get_camera_3d()->get_global_position();
				for (Grass3D *g : Grass3D::active_grass_nodes) {
					if (g && g->is_inside_tree()) {
						g->update_culling_from_camera(cam_pos);
					}
				}
			}
		} break;
	}
}

EditorPlugin::AfterGUIInput Grass3DEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (p_camera) {
		Vector3 cam_pos = p_camera->get_global_position();
		for (Grass3D *g : Grass3D::active_grass_nodes) {
			if (g && g->is_inside_tree()) {
				g->update_culling_from_camera(cam_pos);
			}
		}
	}
	return AFTER_GUI_INPUT_PASS;
}

void Grass3DEditorPlugin::_menu_option(int p_option) {
	if (!grass) {
		return;
	}

	switch (p_option) {
		case MENU_REBUILD_GRASS: {
			grass->rebuild_grass();
		} break;

		case MENU_CLEAR_GRASS: {
			grass->clear_grass();
		} break;

		case MENU_TOGGLE_CHUNK_BOUNDS: {
			grass->set_show_chunk_bounds(!grass->is_show_chunk_bounds());
		} break;

		case MENU_GRASS_STATS: {
			if (!stats_dialog) {
				stats_dialog = memnew(AcceptDialog);
				stats_dialog->set_title(TTR("Grass3D Foliage Diagnostics"));
				add_child(stats_dialog);
			}

			int chunk_count = grass->get_chunk_count();
			int total_instances = grass->get_total_instance_count();
			int visible_instances = grass->get_visible_instance_count();

			String txt = vformat(
					"=== Grass3D Foliage Diagnostics ===\n\n"
					"• Total Chunks: %d\n"
					"• Total Instances: %d\n"
					"• Currently Visible Instances: %d\n"
					"• Density: %.2f / m²\n"
					"• Cull Distance: %.1f m\n"
					"• Blade Dimensions: %.2f m width x %.2f m height\n"
					"• Estimated Memory: ~%.2f KB\n",
					chunk_count,
					total_instances,
					visible_instances,
					grass->get_density(),
					grass->get_cull_distance(),
					grass->get_blade_width(),
					grass->get_blade_height(),
					(float)(total_instances * 64) / 1024.0f);

			stats_dialog->set_text(txt);
			stats_dialog->popup_centered(Vector2i(380, 260) * EDSCALE);
		} break;
	}
}

void Grass3DEditorPlugin::_on_rebuild_pressed() {
	if (grass) {
		grass->rebuild_grass();
	}
}

void Grass3DEditorPlugin::_on_clear_pressed() {
	if (grass) {
		grass->clear_grass();
	}
}

void Grass3DEditorPlugin::edit(Object *p_object) {
	grass = Object::cast_to<Grass3D>(p_object);
}

bool Grass3DEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<Grass3D>(p_object) != nullptr;
}

void Grass3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
	}
}

Grass3DEditorPlugin::Grass3DEditorPlugin() {
	toolbar_hb = memnew(HBoxContainer);
	toolbar_hb->hide();

	VSeparator *sep = memnew(VSeparator);
	toolbar_hb->add_child(sep);

	// Options Dropdown Menu
	options_menu = memnew(MenuButton);
	options_menu->set_text(TTR("🌱 Grass3D"));
	options_menu->set_flat(false);
	options_menu->set_theme_type_variation("FlatMenuButton");

	PopupMenu *popup = options_menu->get_popup();
	popup->add_item(TTR("Rebuild Grass Instances"), MENU_REBUILD_GRASS);
	popup->add_item(TTR("Clear Grass Instances"), MENU_CLEAR_GRASS);
	popup->add_separator();
	popup->add_item(TTR("Toggle Chunk Bounds Wireframe"), MENU_TOGGLE_CHUNK_BOUNDS);
	popup->add_separator();
	popup->add_item(TTR("Grass & VRAM Statistics..."), MENU_GRASS_STATS);

	toolbar_hb->add_child(options_menu);

	// Quick Rebuild Button
	rebuild_btn = memnew(Button(TTR("Rebuild")));
	rebuild_btn->set_tooltip_text(TTR("Rebuild procedural grass instances"));
	rebuild_btn->connect(SceneStringName(pressed), callable_mp(this, &Grass3DEditorPlugin::_on_rebuild_pressed));
	toolbar_hb->add_child(rebuild_btn);

	// Quick Clear Button
	clear_btn = memnew(Button(TTR("Clear")));
	clear_btn->set_tooltip_text(TTR("Clear all active grass instances"));
	clear_btn->connect(SceneStringName(pressed), callable_mp(this, &Grass3DEditorPlugin::_on_clear_pressed));
	toolbar_hb->add_child(clear_btn);

	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);
}

Grass3DEditorPlugin::~Grass3DEditorPlugin() {
}
