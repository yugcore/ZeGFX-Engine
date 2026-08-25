/**************************************************************************/
/*  hurtbox_3d.cpp                                                        */
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

#include "hurtbox_3d.h"
#include "damage_resolver.h"
#include "hitbox_3d.h"

bool Hurtbox3D::can_receive_hit(Hitbox3D *p_hitbox) const {
	if (!p_hitbox || !is_inside_tree()) {
		return false;
	}
	if (team_id == p_hitbox->get_team_id()) {
		return false;
	}
	if (p_hitbox->get_deduplicate_hits() && p_hitbox->has_hit_object(get_instance_id())) {
		return false;
	}
	if (invulnerability_query.is_valid() && state_tags.is_valid()) {
		if (invulnerability_query->evaluate(state_tags)) {
			return false;
		}
	}
	return true;
}

Dictionary Hurtbox3D::receive_hit(Hitbox3D *p_hitbox, const Vector3 &p_hit_point, const Vector3 &p_hit_normal) {
	Dictionary result;
	if (!can_receive_hit(p_hitbox)) {
		return result;
	}

	if (p_hitbox->get_deduplicate_hits()) {
		p_hitbox->register_hit_object(get_instance_id());
	}

	if (DamageResolver::get_singleton()) {
		result = DamageResolver::get_singleton()->resolve_hit(p_hitbox, this, p_hit_point, p_hit_normal);
	}

	emit_signal(SNAME("hit_received"), p_hitbox, result, p_hit_point, p_hit_normal);
	return result;
}

void Hurtbox3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_team_id", "id"), &Hurtbox3D::set_team_id);
	ClassDB::bind_method(D_METHOD("get_team_id"), &Hurtbox3D::get_team_id);

	ClassDB::bind_method(D_METHOD("set_attribute_set", "set"), &Hurtbox3D::set_attribute_set);
	ClassDB::bind_method(D_METHOD("get_attribute_set"), &Hurtbox3D::get_attribute_set);

	ClassDB::bind_method(D_METHOD("set_state_tags", "tags"), &Hurtbox3D::set_state_tags);
	ClassDB::bind_method(D_METHOD("get_state_tags"), &Hurtbox3D::get_state_tags);

	ClassDB::bind_method(D_METHOD("set_invulnerability_query", "query"), &Hurtbox3D::set_invulnerability_query);
	ClassDB::bind_method(D_METHOD("get_invulnerability_query"), &Hurtbox3D::get_invulnerability_query);

	ClassDB::bind_method(D_METHOD("can_receive_hit", "hitbox"), &Hurtbox3D::can_receive_hit);
	ClassDB::bind_method(D_METHOD("receive_hit", "hitbox", "hit_point", "hit_normal"), &Hurtbox3D::receive_hit);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "team_id"), "set_team_id", "get_team_id");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attribute_set", PROPERTY_HINT_RESOURCE_TYPE, "AttributeSet"), "set_attribute_set", "get_attribute_set");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "state_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_state_tags", "get_state_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "invulnerability_query", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_invulnerability_query", "get_invulnerability_query");

	ADD_SIGNAL(MethodInfo("hit_received", PropertyInfo(Variant::OBJECT, "hitbox", PROPERTY_HINT_RESOURCE_TYPE, "Hitbox3D"), PropertyInfo(Variant::DICTIONARY, "damage_result"), PropertyInfo(Variant::VECTOR3, "hit_point"), PropertyInfo(Variant::VECTOR3, "hit_normal")));
}

Hurtbox3D::Hurtbox3D() {
	set_monitoring(false);
	set_monitorable(true);
}

Hurtbox3D::~Hurtbox3D() {}
