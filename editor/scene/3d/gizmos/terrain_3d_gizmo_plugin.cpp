/**************************************************************************/
/*  terrain_3d_gizmo_plugin.cpp                                           */
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

#include "terrain_3d_gizmo_plugin.h"

#include "scene/3d/terrain_3d.h"

Terrain3DGizmoPlugin::Terrain3DGizmoPlugin() {
	create_material("terrain_bounds", Color(0.95, 0.75, 0.15, 0.8));
	create_material("terrain_chunk_bounds", Color(0.2, 0.85, 0.95, 0.4));
}

bool Terrain3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return Object::cast_to<Terrain3D>(p_spatial) != nullptr;
}

String Terrain3DGizmoPlugin::get_gizmo_name() const {
	return "Terrain3D";
}

int Terrain3DGizmoPlugin::get_priority() const {
	return -1;
}

void Terrain3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	p_gizmo->clear();

	Terrain3D *terrain = Object::cast_to<Terrain3D>(p_gizmo->get_node_3d());
	if (!terrain) {
		return;
	}

	Ref<Material> bounds_mat = get_material("terrain_bounds", p_gizmo);
	Ref<Material> chunk_mat = get_material("terrain_chunk_bounds", p_gizmo);

	// Draw outer total AABB
	AABB total_aabb = terrain->get_total_aabb();
	if (total_aabb.size != Vector3()) {
		Vector<Vector3> lines;
		for (int i = 0; i < 12; i++) {
			Vector3 a, b;
			total_aabb.get_edge(i, a, b);
			lines.push_back(a);
			lines.push_back(b);
		}
		p_gizmo->add_lines(lines, bounds_mat);
	}

	// Draw sub-chunk bounds if enabled
	if (terrain->is_show_chunk_bounds()) {
		TypedArray<AABB> chunk_aabbs = terrain->get_chunk_aabbs();
		Vector<Vector3> chunk_lines;
		for (int c = 0; c < chunk_aabbs.size(); ++c) {
			AABB ca = chunk_aabbs[c];
			for (int i = 0; i < 12; i++) {
				Vector3 a, b;
				ca.get_edge(i, a, b);
				chunk_lines.push_back(a);
				chunk_lines.push_back(b);
			}
		}
		if (!chunk_lines.is_empty()) {
			p_gizmo->add_lines(chunk_lines, chunk_mat);
		}
	}
}
