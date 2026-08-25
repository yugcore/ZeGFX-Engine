/**************************************************************************/
/*  combat_action.cpp                                                     */
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

#include "combat_action.h"

bool CombatAction::can_activate(const Ref<AttributeSet> &p_attrs, const Ref<GameplayTagContainer> &p_tags) const {
	// 1. Tag Requirements
	if (activation_required_tags.is_valid()) {
		if (!activation_required_tags->evaluate(p_tags)) {
			return false;
		}
	}

	// 2. Tag Blocks
	if (activation_blocked_tags.is_valid()) {
		if (activation_blocked_tags->evaluate(p_tags)) {
			return false;
		}
	}

	// 3. Resource Costs
	if (p_attrs.is_valid() && !resource_costs.is_empty()) {
		for (const Variant *key = resource_costs.next(nullptr); key != nullptr; key = resource_costs.next(key)) {
			StringName attr_name = *key;
			real_t req_cost = resource_costs[*key];
			if (p_attrs->get_attribute_current(attr_name) < req_cost) {
				return false;
			}
		}
	}

	return true;
}

void CombatAction::consume_costs(const Ref<AttributeSet> &p_attrs) const {
	if (p_attrs.is_valid() && !resource_costs.is_empty()) {
		for (const Variant *key = resource_costs.next(nullptr); key != nullptr; key = resource_costs.next(key)) {
			StringName attr_name = *key;
			real_t req_cost = resource_costs[*key];
			p_attrs->apply_damage(attr_name, req_cost);
		}
	}
}

void CombatAction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_action_name", "name"), &CombatAction::set_action_name);
	ClassDB::bind_method(D_METHOD("get_action_name"), &CombatAction::get_action_name);

	ClassDB::bind_method(D_METHOD("set_animation_state_name", "name"), &CombatAction::set_animation_state_name);
	ClassDB::bind_method(D_METHOD("get_animation_state_name"), &CombatAction::get_animation_state_name);

	ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &CombatAction::set_timeline);
	ClassDB::bind_method(D_METHOD("get_timeline"), &CombatAction::get_timeline);

	ClassDB::bind_method(D_METHOD("set_resource_costs", "costs"), &CombatAction::set_resource_costs);
	ClassDB::bind_method(D_METHOD("get_resource_costs"), &CombatAction::get_resource_costs);

	ClassDB::bind_method(D_METHOD("set_cooldown", "cooldown"), &CombatAction::set_cooldown);
	ClassDB::bind_method(D_METHOD("get_cooldown"), &CombatAction::get_cooldown);

	ClassDB::bind_method(D_METHOD("set_cooldown_tag_group", "group"), &CombatAction::set_cooldown_tag_group);
	ClassDB::bind_method(D_METHOD("get_cooldown_tag_group"), &CombatAction::get_cooldown_tag_group);

	ClassDB::bind_method(D_METHOD("set_activation_required_tags", "query"), &CombatAction::set_activation_required_tags);
	ClassDB::bind_method(D_METHOD("get_activation_required_tags"), &CombatAction::get_activation_required_tags);

	ClassDB::bind_method(D_METHOD("set_activation_blocked_tags", "query"), &CombatAction::set_activation_blocked_tags);
	ClassDB::bind_method(D_METHOD("get_activation_blocked_tags"), &CombatAction::get_activation_blocked_tags);

	ClassDB::bind_method(D_METHOD("set_granted_tags_while_active", "tags"), &CombatAction::set_granted_tags_while_active);
	ClassDB::bind_method(D_METHOD("get_granted_tags_while_active"), &CombatAction::get_granted_tags_while_active);

	ClassDB::bind_method(D_METHOD("can_activate", "attributes", "tags"), &CombatAction::can_activate);
	ClassDB::bind_method(D_METHOD("consume_costs", "attributes"), &CombatAction::consume_costs);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_name"), "set_action_name", "get_action_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "animation_state_name"), "set_animation_state_name", "get_animation_state_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "timeline", PROPERTY_HINT_RESOURCE_TYPE, "ActionTimeline"), "set_timeline", "get_timeline");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "resource_costs"), "set_resource_costs", "get_resource_costs");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cooldown"), "set_cooldown", "get_cooldown");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "cooldown_tag_group"), "set_cooldown_tag_group", "get_cooldown_tag_group");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "activation_required_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_activation_required_tags", "get_activation_required_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "activation_blocked_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_activation_blocked_tags", "get_activation_blocked_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "granted_tags_while_active", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_granted_tags_while_active", "get_granted_tags_while_active");
}
