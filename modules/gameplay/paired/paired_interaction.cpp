/**************************************************************************/
/*  paired_interaction.cpp                                                */
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

#include "paired_interaction.h"

void PairedInteraction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_interaction_id", "id"), &PairedInteraction::set_interaction_id);
	ClassDB::bind_method(D_METHOD("get_interaction_id"), &PairedInteraction::get_interaction_id);

	ClassDB::bind_method(D_METHOD("set_relative_victim_transform", "xform"), &PairedInteraction::set_relative_victim_transform);
	ClassDB::bind_method(D_METHOD("get_relative_victim_transform"), &PairedInteraction::get_relative_victim_transform);

	ClassDB::bind_method(D_METHOD("set_attacker_animation", "anim"), &PairedInteraction::set_attacker_animation);
	ClassDB::bind_method(D_METHOD("get_attacker_animation"), &PairedInteraction::get_attacker_animation);

	ClassDB::bind_method(D_METHOD("set_victim_animation", "anim"), &PairedInteraction::set_victim_animation);
	ClassDB::bind_method(D_METHOD("get_victim_animation"), &PairedInteraction::get_victim_animation);

	ClassDB::bind_method(D_METHOD("set_duration", "duration"), &PairedInteraction::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &PairedInteraction::get_duration);

	ClassDB::bind_method(D_METHOD("set_alignment_duration", "duration"), &PairedInteraction::set_alignment_duration);
	ClassDB::bind_method(D_METHOD("get_alignment_duration"), &PairedInteraction::get_alignment_duration);

	ClassDB::bind_method(D_METHOD("set_damage_payload", "payload"), &PairedInteraction::set_damage_payload);
	ClassDB::bind_method(D_METHOD("get_damage_payload"), &PairedInteraction::get_damage_payload);

	ClassDB::bind_method(D_METHOD("set_attacker_effect", "effect"), &PairedInteraction::set_attacker_effect);
	ClassDB::bind_method(D_METHOD("get_attacker_effect"), &PairedInteraction::get_attacker_effect);

	ClassDB::bind_method(D_METHOD("set_victim_effect", "effect"), &PairedInteraction::set_victim_effect);
	ClassDB::bind_method(D_METHOD("get_victim_effect"), &PairedInteraction::get_victim_effect);

	ClassDB::bind_method(D_METHOD("set_attacker_granted_tags", "tags"), &PairedInteraction::set_attacker_granted_tags);
	ClassDB::bind_method(D_METHOD("get_attacker_granted_tags"), &PairedInteraction::get_attacker_granted_tags);

	ClassDB::bind_method(D_METHOD("set_victim_granted_tags", "tags"), &PairedInteraction::set_victim_granted_tags);
	ClassDB::bind_method(D_METHOD("get_victim_granted_tags"), &PairedInteraction::get_victim_granted_tags);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "interaction_id"), "set_interaction_id", "get_interaction_id");
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "relative_victim_transform"), "set_relative_victim_transform", "get_relative_victim_transform");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "attacker_animation"), "set_attacker_animation", "get_attacker_animation");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "victim_animation"), "set_victim_animation", "get_victim_animation");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alignment_duration"), "set_alignment_duration", "get_alignment_duration");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "damage_payload"), "set_damage_payload", "get_damage_payload");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attacker_effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), "set_attacker_effect", "get_attacker_effect");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "victim_effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), "set_victim_effect", "get_victim_effect");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attacker_granted_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_attacker_granted_tags", "get_attacker_granted_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "victim_granted_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_victim_granted_tags", "get_victim_granted_tags");
}
