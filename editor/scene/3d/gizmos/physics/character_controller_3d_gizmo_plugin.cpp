/**************************************************************************/
/*  character_controller_3d_gizmo_plugin.cpp                             */
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

#include "character_controller_3d_gizmo_plugin.h"

#include "scene/3d/physics/character_controller_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"

CharacterController3DGizmoPlugin::CharacterController3DGizmoPlugin() {
	create_material("controller_eye_material", Color(0.2, 0.8, 1.0, 0.85));
	create_material("controller_step_material", Color(1.0, 0.75, 0.2, 0.7));
	create_material("controller_spring_material", Color(0.9, 0.3, 0.9, 0.7));
}

bool CharacterController3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return Object::cast_to<CharacterController3D>(p_spatial) != nullptr;
}

String CharacterController3DGizmoPlugin::get_gizmo_name() const {
	return "CharacterController3D";
}

int CharacterController3DGizmoPlugin::get_priority() const {
	return -1;
}

void CharacterController3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	CharacterController3D *controller = Object::cast_to<CharacterController3D>(p_gizmo->get_node_3d());
	if (!controller) {
		return;
	}

	p_gizmo->clear();

	real_t eye_h = controller->get_eye_height();
	real_t step_h = controller->get_max_step_height();
	real_t radius = controller->get_capsule_radius();

	// 1. Eye Height Ring and Forward Arrow
	Vector<Vector3> eye_lines;
	const int segments = 24;
	for (int i = 0; i < segments; i++) {
		real_t a1 = (real_t)i / segments * Math::TAU;
		real_t a2 = (real_t)(i + 1) / segments * Math::TAU;
		eye_lines.push_back(Vector3(Math::cos(a1) * (radius + 0.05), eye_h, Math::sin(a1) * (radius + 0.05)));
		eye_lines.push_back(Vector3(Math::cos(a2) * (radius + 0.05), eye_h, Math::sin(a2) * (radius + 0.05)));
	}
	// Forward pointer at eye height
	eye_lines.push_back(Vector3(0, eye_h, -(radius + 0.05)));
	eye_lines.push_back(Vector3(0, eye_h, -(radius + 0.45)));
	eye_lines.push_back(Vector3(0, eye_h, -(radius + 0.45)));
	eye_lines.push_back(Vector3(-0.1, eye_h, -(radius + 0.3)));
	eye_lines.push_back(Vector3(0, eye_h, -(radius + 0.45)));
	eye_lines.push_back(Vector3(0.1, eye_h, -(radius + 0.3)));

	Ref<StandardMaterial3D> eye_mat = get_material("controller_eye_material", p_gizmo);
	p_gizmo->add_lines(eye_lines, eye_mat);

	// 2. Stair Step Threshold Gizmo
	if (controller->is_stair_stepping_enabled()) {
		Vector<Vector3> step_lines;
		real_t step_fwd = controller->get_step_forward_distance();
		// Forward step bracket
		step_lines.push_back(Vector3(-radius * 0.6, 0.02, -step_fwd));
		step_lines.push_back(Vector3(radius * 0.6, 0.02, -step_fwd));
		step_lines.push_back(Vector3(-radius * 0.6, step_h, -step_fwd));
		step_lines.push_back(Vector3(radius * 0.6, step_h, -step_fwd));
		step_lines.push_back(Vector3(0, 0.02, -step_fwd));
		step_lines.push_back(Vector3(0, step_h, -step_fwd));

		Ref<StandardMaterial3D> step_mat = get_material("controller_step_material", p_gizmo);
		p_gizmo->add_lines(step_lines, step_mat);
	}

	// 3. Third Person Camera Ray Preview
	if (controller->get_view_mode() != CharacterController3D::VIEW_FIRST_PERSON) {
		Vector<Vector3> spring_lines;
		Vector3 start = Vector3(0, eye_h, 0) + controller->get_third_person_offset();
		Vector3 end = start + Vector3(0, 0, controller->get_third_person_distance());

		spring_lines.push_back(start);
		spring_lines.push_back(end);

		// Camera preview box at end
		real_t box_sz = 0.15;
		spring_lines.push_back(end + Vector3(-box_sz, -box_sz, 0));
		spring_lines.push_back(end + Vector3(box_sz, -box_sz, 0));
		spring_lines.push_back(end + Vector3(box_sz, -box_sz, 0));
		spring_lines.push_back(end + Vector3(box_sz, box_sz, 0));
		spring_lines.push_back(end + Vector3(box_sz, box_sz, 0));
		spring_lines.push_back(end + Vector3(-box_sz, box_sz, 0));
		spring_lines.push_back(end + Vector3(-box_sz, box_sz, 0));
		spring_lines.push_back(end + Vector3(-box_sz, -box_sz, 0));

		Ref<StandardMaterial3D> spring_mat = get_material("controller_spring_material", p_gizmo);
		p_gizmo->add_lines(spring_lines, spring_mat);
	}
}
