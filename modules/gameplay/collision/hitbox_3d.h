/**************************************************************************/
/*  hitbox_3d.h                                                           */
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

#pragma once

#include "../attributes/attribute_set.h"
#include "../attributes/gameplay_effect.h"
#include "../core/gameplay_tags.h"
#include "core/templates/hash_set.h"
#include "core/templates/vector.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"

class Hurtbox3D;

class Hitbox3D : public Node3D {
	GDCLASS(Hitbox3D, Node3D);

public:
	enum SocketMode {
		SOCKET_MODE_POINT,
		SOCKET_MODE_CAPSULE,
		SOCKET_MODE_CHAIN,
	};

private:
	SocketMode socket_mode = SOCKET_MODE_CAPSULE;
	PackedStringArray socket_names;
	real_t radius = 0.15;
	uint32_t team_id = 0;
	bool deduplicate_hits = true;
	bool active = false;

	NodePath skeleton_path;
	Skeleton3D *skeleton_node = nullptr;

	Dictionary damage_channels;
	Ref<GameplayEffect> hit_effect;
	Ref<GameplayTagContainer> attack_tags;
	Ref<AttributeSet> instigator_attributes;

	Vector<Vector3> prev_socket_positions;
	HashSet<ObjectID> already_hit_objects;

	Skeleton3D *_find_skeleton();
	void _perform_swept_trace();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_socket_mode(SocketMode p_mode);
	SocketMode get_socket_mode() const { return socket_mode; }

	void set_socket_names(const PackedStringArray &p_names);
	PackedStringArray get_socket_names() const { return socket_names; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_team_id(uint32_t p_id) { team_id = p_id; }
	uint32_t get_team_id() const { return team_id; }

	void set_deduplicate_hits(bool p_dedup) { deduplicate_hits = p_dedup; }
	bool get_deduplicate_hits() const { return deduplicate_hits; }

	void set_skeleton_path(const NodePath &p_path);
	NodePath get_skeleton_path() const { return skeleton_path; }

	void set_damage_channels(const Dictionary &p_channels);
	Dictionary get_damage_channels() const { return damage_channels; }

	void set_hit_effect(const Ref<GameplayEffect> &p_effect);
	Ref<GameplayEffect> get_hit_effect() const { return hit_effect; }

	void set_attack_tags(const Ref<GameplayTagContainer> &p_tags);
	Ref<GameplayTagContainer> get_attack_tags() const { return attack_tags; }

	void set_instigator_attributes(const Ref<AttributeSet> &p_attrs) { instigator_attributes = p_attrs; }
	Ref<AttributeSet> get_instigator_attributes() const { return instigator_attributes; }

	void activate();
	void deactivate();
	bool is_active() const { return active; }

	void reset_hit_history();
	bool has_hit_object(ObjectID p_id) const;
	void register_hit_object(ObjectID p_id);

	Vector<Vector3> get_socket_world_positions();

	Hitbox3D();
	~Hitbox3D();
};

VARIANT_ENUM_CAST(Hitbox3D::SocketMode);
