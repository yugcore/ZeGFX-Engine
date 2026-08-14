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
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "scene/gui/popup_menu.h"

void Terrain3DEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		if (options_menu) {
			options_menu->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(SNAME("Mesh"), EditorStringName(EditorIcons)));
		}
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

void Terrain3DEditorPlugin::edit(Object *p_object) {
	terrain = Object::cast_to<Terrain3D>(p_object);
}

bool Terrain3DEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("Terrain3D");
}

void Terrain3DEditorPlugin::make_visible(bool p_visible) {
	if (toolbar_hb) {
		toolbar_hb->set_visible(p_visible);
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

	popup->connect(SceneStringName(id_pressed), callable_mp(this, &Terrain3DEditorPlugin::_menu_option));

	toolbar_hb->add_child(options_menu);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_hb);
}

Terrain3DEditorPlugin::~Terrain3DEditorPlugin() {
}
