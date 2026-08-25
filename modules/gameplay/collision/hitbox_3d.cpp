/**************************************************************************/
/*  hitbox_3d.cpp                                                         */
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

#include "hitbox_3d.h"
#include "damage_resolver.h"
#include "hurtbox_3d.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

Skeleton3D *Hitbox3D::_find_skeleton() {
	if (skeleton_node) {
		return skeleton_node;
	}
	if (!skeleton_path.is_empty()) {
		skeleton_node = Object::cast_to<Skeleton3D>(get_node_or_null(skeleton_path));
		if (skeleton_node) {
			return skeleton_node;
		}
	}
	Node *parent = get_parent();
	while (parent) {
		Skeleton3D *skel = Object::cast_to<Skeleton3D>(parent);
		if (skel) {
			skeleton_node = skel;
			return skeleton_node;
		}
		parent = parent->get_parent();
	}
	return nullptr;
}

Vector<Vector3> Hitbox3D::get_socket_world_positions() {
	Vector<Vector3> positions;
	Skeleton3D *skel = _find_skeleton();

	if (socket_names.is_empty()) {
		positions.push_back(get_global_position());
		return positions;
	}

	for (int i = 0; i < socket_names.size(); i++) {
		StringName s_name = StringName(socket_names[i]);
		Vector3 pos = get_global_position();
		bool found = false;

		// 1. Check Skeleton Bone
		if (skel) {
			int bone_idx = skel->find_bone(s_name);
			if (bone_idx >= 0) {
				pos = skel->get_global_transform().xform(skel->get_bone_global_pose(bone_idx).origin);
				found = true;
			}
		}

		// 2. Check Child Marker
		if (!found) {
			Node *child = find_child(s_name, true, false);
			Node3D *child_3d = Object::cast_to<Node3D>(child);
			if (child_3d) {
				pos = child_3d->get_global_position();
				found = true;
			}
		}

		positions.push_back(pos);
	}

	return positions;
}

void Hitbox3D::set_socket_mode(SocketMode p_mode) {
	socket_mode = p_mode;
}

void Hitbox3D::set_socket_names(const PackedStringArray &p_names) {
	socket_names = p_names;
}

void Hitbox3D::set_radius(real_t p_radius) {
	radius = p_radius;
}

void Hitbox3D::set_skeleton_path(const NodePath &p_path) {
	skeleton_path = p_path;
	skeleton_node = nullptr;
}

void Hitbox3D::set_damage_channels(const Dictionary &p_channels) {
	damage_channels = p_channels;
}

void Hitbox3D::set_hit_effect(const Ref<GameplayEffect> &p_effect) {
	hit_effect = p_effect;
}

void Hitbox3D::set_attack_tags(const Ref<GameplayTagContainer> &p_tags) {
	attack_tags = p_tags;
}

void Hitbox3D::activate() {
	if (!active) {
		active = true;
		reset_hit_history();
		prev_socket_positions = get_socket_world_positions();
		set_physics_process(true);
	}
}

void Hitbox3D::deactivate() {
	if (active) {
		active = false;
		set_physics_process(false);
	}
}

void Hitbox3D::reset_hit_history() {
	already_hit_objects.clear();
}

bool Hitbox3D::has_hit_object(ObjectID p_id) const {
	return already_hit_objects.has(p_id);
}

void Hitbox3D::register_hit_object(ObjectID p_id) {
	already_hit_objects.insert(p_id);
}

void Hitbox3D::_perform_swept_trace() {
	if (!is_inside_tree() || !active) {
		return;
	}

	Vector<Vector3> curr_positions = get_socket_world_positions();
	if (prev_socket_positions.size() != curr_positions.size()) {
		prev_socket_positions = curr_positions;
		return;
	}

	PhysicsDirectSpaceState3D *space_state = get_world_3d()->get_direct_space_state();
	if (!space_state) {
		return;
	}

	// Trace between previous and current socket positions
	for (int i = 0; i < curr_positions.size(); i++) {
		Vector3 p_from = prev_socket_positions[i];
		Vector3 p_to = curr_positions[i];

		// Ray query
		PhysicsServer3DTypes::RayParameters ray_params;
		ray_params.from = p_from;
		ray_params.to = (p_to - p_from).length_squared() > 0.0001 ? p_to : p_from + Vector3(0, 0.001, 0);
		ray_params.collide_with_areas = true;
		ray_params.collide_with_bodies = false;

		PhysicsServer3DTypes::RayResult ray_res;
		if (space_state->intersect_ray(ray_params, ray_res)) {
			Hurtbox3D *hurtbox = Object::cast_to<Hurtbox3D>(ray_res.collider);
			if (hurtbox && hurtbox->can_receive_hit(this)) {
				hurtbox->receive_hit(this, ray_res.position, ray_res.normal);
				emit_signal(SNAME("hit_detected"), hurtbox, ray_res.position, ray_res.normal);
			}
		}

		// If 2 or more sockets, trace across blade span
		if (i + 1 < curr_positions.size()) {
			Vector3 span_from = curr_positions[i];
			Vector3 span_to = curr_positions[i + 1];

			PhysicsServer3DTypes::RayParameters span_params;
			span_params.from = span_from;
			span_params.to = span_to;
			span_params.collide_with_areas = true;
			span_params.collide_with_bodies = false;

			PhysicsServer3DTypes::RayResult span_res;
			if (space_state->intersect_ray(span_params, span_res)) {
				Hurtbox3D *hurtbox = Object::cast_to<Hurtbox3D>(span_res.collider);
				if (hurtbox && hurtbox->can_receive_hit(this)) {
					hurtbox->receive_hit(this, span_res.position, span_res.normal);
					emit_signal(SNAME("hit_detected"), hurtbox, span_res.position, span_res.normal);
				}
			}
		}
	}

	prev_socket_positions = curr_positions;
}

void Hitbox3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (active) {
				set_physics_process(true);
			} else {
				set_physics_process(false);
			}
		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {
			if (active) {
				_perform_swept_trace();
			}
		} break;
	}
}

void Hitbox3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_socket_mode", "mode"), &Hitbox3D::set_socket_mode);
	ClassDB::bind_method(D_METHOD("get_socket_mode"), &Hitbox3D::get_socket_mode);

	ClassDB::bind_method(D_METHOD("set_socket_names", "names"), &Hitbox3D::set_socket_names);
	ClassDB::bind_method(D_METHOD("get_socket_names"), &Hitbox3D::get_socket_names);

	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &Hitbox3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &Hitbox3D::get_radius);

	ClassDB::bind_method(D_METHOD("set_team_id", "id"), &Hitbox3D::set_team_id);
	ClassDB::bind_method(D_METHOD("get_team_id"), &Hitbox3D::get_team_id);

	ClassDB::bind_method(D_METHOD("set_deduplicate_hits", "deduplicate"), &Hitbox3D::set_deduplicate_hits);
	ClassDB::bind_method(D_METHOD("get_deduplicate_hits"), &Hitbox3D::get_deduplicate_hits);

	ClassDB::bind_method(D_METHOD("set_skeleton_path", "path"), &Hitbox3D::set_skeleton_path);
	ClassDB::bind_method(D_METHOD("get_skeleton_path"), &Hitbox3D::get_skeleton_path);

	ClassDB::bind_method(D_METHOD("set_damage_channels", "channels"), &Hitbox3D::set_damage_channels);
	ClassDB::bind_method(D_METHOD("get_damage_channels"), &Hitbox3D::get_damage_channels);

	ClassDB::bind_method(D_METHOD("set_hit_effect", "effect"), &Hitbox3D::set_hit_effect);
	ClassDB::bind_method(D_METHOD("get_hit_effect"), &Hitbox3D::get_hit_effect);

	ClassDB::bind_method(D_METHOD("set_attack_tags", "tags"), &Hitbox3D::set_attack_tags);
	ClassDB::bind_method(D_METHOD("get_attack_tags"), &Hitbox3D::get_attack_tags);

	ClassDB::bind_method(D_METHOD("set_instigator_attributes", "attributes"), &Hitbox3D::set_instigator_attributes);
	ClassDB::bind_method(D_METHOD("get_instigator_attributes"), &Hitbox3D::get_instigator_attributes);

	ClassDB::bind_method(D_METHOD("activate"), &Hitbox3D::activate);
	ClassDB::bind_method(D_METHOD("deactivate"), &Hitbox3D::deactivate);
	ClassDB::bind_method(D_METHOD("is_active"), &Hitbox3D::is_active);

	ClassDB::bind_method(D_METHOD("reset_hit_history"), &Hitbox3D::reset_hit_history);
	ClassDB::bind_method(D_METHOD("has_hit_object", "id"), &Hitbox3D::has_hit_object);
	ClassDB::bind_method(D_METHOD("register_hit_object", "id"), &Hitbox3D::register_hit_object);

	ClassDB::bind_method(D_METHOD("get_socket_world_positions"), &Hitbox3D::get_socket_world_positions);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "socket_mode", PROPERTY_HINT_ENUM, "Point,Capsule,Chain"), "set_socket_mode", "get_socket_mode");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "socket_names"), "set_socket_names", "get_socket_names");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "team_id"), "set_team_id", "get_team_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deduplicate_hits"), "set_deduplicate_hits", "get_deduplicate_hits");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_path"), "set_skeleton_path", "get_skeleton_path");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "damage_channels"), "set_damage_channels", "get_damage_channels");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "hit_effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), "set_hit_effect", "get_hit_effect");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attack_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_attack_tags", "get_attack_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "instigator_attributes", PROPERTY_HINT_RESOURCE_TYPE, "AttributeSet"), "set_instigator_attributes", "get_instigator_attributes");

	ADD_SIGNAL(MethodInfo("hit_detected", PropertyInfo(Variant::OBJECT, "hurtbox", PROPERTY_HINT_RESOURCE_TYPE, "Hurtbox3D"), PropertyInfo(Variant::VECTOR3, "hit_point"), PropertyInfo(Variant::VECTOR3, "hit_normal")));

	BIND_ENUM_CONSTANT(SOCKET_MODE_POINT);
	BIND_ENUM_CONSTANT(SOCKET_MODE_CAPSULE);
	BIND_ENUM_CONSTANT(SOCKET_MODE_CHAIN);
}

Hitbox3D::Hitbox3D() {}
Hitbox3D::~Hitbox3D() {}
