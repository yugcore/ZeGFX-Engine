/**************************************************************************/
/*  floating_origin_3d.cpp                                                */
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

#include "floating_origin_3d.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

FloatingOrigin3D::FloatingOrigin3D() {
}

FloatingOrigin3D::~FloatingOrigin3D() {
}

void FloatingOrigin3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_internal(true);
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!auto_rebase_enabled) return;

			if (Engine::get_singleton()->is_editor_hint() && !rebase_in_editor) {
				return;
			}

			Vector3 target_pos;
			bool has_pos = false;

			if (!target_node_path.is_empty()) {
				Node3D *target = Object::cast_to<Node3D>(get_node_or_null(target_node_path));
				if (target) {
					target_pos = target->get_global_position();
					has_pos = true;
				}
			}

			if (!has_pos) {
				Viewport *vp = get_viewport();
				if (vp) {
					Camera3D *cam = vp->get_camera_3d();
					if (cam) {
						target_pos = cam->get_global_position();
						has_pos = true;
					}
				}
			}

			if (has_pos && threshold > 10.0f) {
				if (target_pos.length() >= threshold) {
					rebase_to_position(target_pos);
				}
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			set_process_internal(false);
		} break;
	}
}

void FloatingOrigin3D::_perform_rebase(const Vector3 &p_shift_delta) {
	if (p_shift_delta.is_zero_approx()) return;

	Node *scene_root = get_tree() ? get_tree()->get_current_scene() : nullptr;
	if (!scene_root) {
		scene_root = get_parent();
	}

	if (scene_root) {
		for (int i = 0; i < scene_root->get_child_count(); ++i) {
			Node3D *child_3d = Object::cast_to<Node3D>(scene_root->get_child(i));
			if (child_3d) {
				Transform3D xform = child_3d->get_global_transform();
				xform.origin -= p_shift_delta;
				child_3d->set_global_transform(xform);
			}
		}
	}

	total_world_offset += p_shift_delta;
	shift_count++;

	emit_signal("world_origin_shifted", p_shift_delta, total_world_offset);
}

void FloatingOrigin3D::shift_world_origin(const Vector3 &p_delta) {
	_perform_rebase(p_delta);
}

void FloatingOrigin3D::rebase_to_position(const Vector3 &p_target_pos) {
	_perform_rebase(p_target_pos);
}

void FloatingOrigin3D::reset_universe_origin() {
	_perform_rebase(-total_world_offset);
	total_world_offset = Vector3();
	shift_count = 0;
}

Vector3 FloatingOrigin3D::local_to_universe(const Vector3 &p_local_pos) const {
	return p_local_pos + total_world_offset;
}

Vector3 FloatingOrigin3D::universe_to_local(const Vector3 &p_universe_pos) const {
	return p_universe_pos - total_world_offset;
}

void FloatingOrigin3D::set_threshold(float p_dist) {
	threshold = MAX(10.0f, p_dist);
}

float FloatingOrigin3D::get_threshold() const {
	return threshold;
}

void FloatingOrigin3D::set_auto_rebase_enabled(bool p_enabled) {
	auto_rebase_enabled = p_enabled;
}

bool FloatingOrigin3D::is_auto_rebase_enabled() const {
	return auto_rebase_enabled;
}

void FloatingOrigin3D::set_rebase_physics_bodies(bool p_enabled) {
	rebase_physics_bodies = p_enabled;
}

bool FloatingOrigin3D::is_rebase_physics_bodies() const {
	return rebase_physics_bodies;
}

void FloatingOrigin3D::set_rebase_in_editor(bool p_enabled) {
	rebase_in_editor = p_enabled;
}

bool FloatingOrigin3D::is_rebase_in_editor() const {
	return rebase_in_editor;
}

void FloatingOrigin3D::set_target_node_path(const NodePath &p_path) {
	target_node_path = p_path;
}

NodePath FloatingOrigin3D::get_target_node_path() const {
	return target_node_path;
}

Vector3 FloatingOrigin3D::get_total_world_offset() const {
	return total_world_offset;
}

int FloatingOrigin3D::get_shift_count() const {
	return shift_count;
}

void FloatingOrigin3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_threshold", "distance"), &FloatingOrigin3D::set_threshold);
	ClassDB::bind_method(D_METHOD("get_threshold"), &FloatingOrigin3D::get_threshold);

	ClassDB::bind_method(D_METHOD("set_auto_rebase_enabled", "enabled"), &FloatingOrigin3D::set_auto_rebase_enabled);
	ClassDB::bind_method(D_METHOD("is_auto_rebase_enabled"), &FloatingOrigin3D::is_auto_rebase_enabled);

	ClassDB::bind_method(D_METHOD("set_rebase_physics_bodies", "enabled"), &FloatingOrigin3D::set_rebase_physics_bodies);
	ClassDB::bind_method(D_METHOD("is_rebase_physics_bodies"), &FloatingOrigin3D::is_rebase_physics_bodies);

	ClassDB::bind_method(D_METHOD("set_rebase_in_editor", "enabled"), &FloatingOrigin3D::set_rebase_in_editor);
	ClassDB::bind_method(D_METHOD("is_rebase_in_editor"), &FloatingOrigin3D::is_rebase_in_editor);

	ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &FloatingOrigin3D::set_target_node_path);
	ClassDB::bind_method(D_METHOD("get_target_node_path"), &FloatingOrigin3D::get_target_node_path);

	ClassDB::bind_method(D_METHOD("get_total_world_offset"), &FloatingOrigin3D::get_total_world_offset);
	ClassDB::bind_method(D_METHOD("get_shift_count"), &FloatingOrigin3D::get_shift_count);

	ClassDB::bind_method(D_METHOD("local_to_universe", "local_position"), &FloatingOrigin3D::local_to_universe);
	ClassDB::bind_method(D_METHOD("universe_to_local", "universe_position"), &FloatingOrigin3D::universe_to_local);

	ClassDB::bind_method(D_METHOD("shift_world_origin", "delta"), &FloatingOrigin3D::shift_world_origin);
	ClassDB::bind_method(D_METHOD("rebase_to_position", "target_position"), &FloatingOrigin3D::rebase_to_position);
	ClassDB::bind_method(D_METHOD("reset_universe_origin"), &FloatingOrigin3D::reset_universe_origin);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "threshold", PROPERTY_HINT_RANGE, "10.0,50000.0,10.0,suffix:m"), "set_threshold", "get_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebase_enabled"), "set_auto_rebase_enabled", "is_auto_rebase_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rebase_physics_bodies"), "set_rebase_physics_bodies", "is_rebase_physics_bodies");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rebase_in_editor"), "set_rebase_in_editor", "is_rebase_in_editor");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");

	ADD_SIGNAL(MethodInfo("world_origin_shifted", PropertyInfo(Variant::VECTOR3, "shift_delta"), PropertyInfo(Variant::VECTOR3, "total_universe_offset")));
}
