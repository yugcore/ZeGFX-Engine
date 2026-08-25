/**************************************************************************/
/*  phase_transition_node.cpp                                             */
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

#include "phase_transition_node.h"

// -----------------------------------------------------------------------------
// PhaseTransitionRule
// -----------------------------------------------------------------------------

void PhaseTransitionRule::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_phase_index", "index"), &PhaseTransitionRule::set_phase_index);
	ClassDB::bind_method(D_METHOD("get_phase_index"), &PhaseTransitionRule::get_phase_index);

	ClassDB::bind_method(D_METHOD("set_health_threshold_pct", "pct"), &PhaseTransitionRule::set_health_threshold_pct);
	ClassDB::bind_method(D_METHOD("get_health_threshold_pct"), &PhaseTransitionRule::get_health_threshold_pct);

	ClassDB::bind_method(D_METHOD("set_transition_animation", "anim"), &PhaseTransitionRule::set_transition_animation);
	ClassDB::bind_method(D_METHOD("get_transition_animation"), &PhaseTransitionRule::get_transition_animation);

	ClassDB::bind_method(D_METHOD("set_new_attribute_set", "attrs"), &PhaseTransitionRule::set_new_attribute_set);
	ClassDB::bind_method(D_METHOD("get_new_attribute_set"), &PhaseTransitionRule::get_new_attribute_set);

	ClassDB::bind_method(D_METHOD("set_new_available_actions", "actions"), &PhaseTransitionRule::set_new_available_actions);
	ClassDB::bind_method(D_METHOD("get_new_available_actions"), &PhaseTransitionRule::get_new_available_actions);

	ClassDB::bind_method(D_METHOD("set_granted_phase_tags", "tags"), &PhaseTransitionRule::set_granted_phase_tags);
	ClassDB::bind_method(D_METHOD("get_granted_phase_tags"), &PhaseTransitionRule::get_granted_phase_tags);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "phase_index"), "set_phase_index", "get_phase_index");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "health_threshold_pct"), "set_health_threshold_pct", "get_health_threshold_pct");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "transition_animation"), "set_transition_animation", "get_transition_animation");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "new_attribute_set", PROPERTY_HINT_RESOURCE_TYPE, "AttributeSet"), "set_new_attribute_set", "get_new_attribute_set");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "new_available_actions", PROPERTY_HINT_RESOURCE_TYPE, "CombatAction"), "set_new_available_actions", "get_new_available_actions");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "granted_phase_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_granted_phase_tags", "get_granted_phase_tags");
}

// -----------------------------------------------------------------------------
// PhaseTransitionController
// -----------------------------------------------------------------------------

void PhaseTransitionController::check_phase_transition() {
	if (is_transitioning || attribute_set.is_null()) {
		return;
	}

	real_t current_hp_pct = attribute_set->get_attribute_percent("Health");

	for (int i = 0; i < phase_rules.size(); i++) {
		Ref<PhaseTransitionRule> rule = phase_rules[i];
		if (rule.is_valid() && rule->get_phase_index() > current_phase) {
			if (current_hp_pct <= rule->get_health_threshold_pct()) {
				transition_to_phase(rule->get_phase_index());
				break;
			}
		}
	}
}

void PhaseTransitionController::transition_to_phase(int p_phase) {
	if (is_transitioning || p_phase <= current_phase) {
		return;
	}

	is_transitioning = true;
	int old_phase = current_phase;
	current_phase = p_phase;

	Ref<PhaseTransitionRule> active_rule;
	for (int i = 0; i < phase_rules.size(); i++) {
		Ref<PhaseTransitionRule> rule = phase_rules[i];
		if (rule.is_valid() && rule->get_phase_index() == current_phase) {
			active_rule = rule;
			break;
		}
	}

	if (active_rule.is_valid()) {
		// Apply new attribute set or refill stats
		if (active_rule->get_new_attribute_set().is_valid()) {
			attribute_set = active_rule->get_new_attribute_set();
			if (get_parent()) {
				get_parent()->set("attribute_set", attribute_set);
			}
		}

		// Apply granted tags
		if (state_tags.is_valid() && active_rule->get_granted_phase_tags().is_valid()) {
			PackedStringArray tags = active_rule->get_granted_phase_tags()->get_tags();
			for (int t = 0; t < tags.size(); t++) {
				state_tags->add_tag(StringName(tags[t]));
			}
		}
	}

	emit_signal(SNAME("phase_transition_started"), old_phase, current_phase);
}

void PhaseTransitionController::complete_transition() {
	if (is_transitioning) {
		is_transitioning = false;
		emit_signal(SNAME("phase_transition_completed"), current_phase);
	}
}

void PhaseTransitionController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (attribute_set.is_null() && get_parent()) {
				Variant v = get_parent()->get("attribute_set");
				attribute_set = v;
			}
			if (state_tags.is_null() && get_parent()) {
				Variant v = get_parent()->get("state_tags");
				state_tags = v;
			}
			set_process(true);
		} break;
		case NOTIFICATION_PROCESS: {
			check_phase_transition();
		} break;
	}
}

void PhaseTransitionController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_phase_rules", "rules"), &PhaseTransitionController::set_phase_rules);
	ClassDB::bind_method(D_METHOD("get_phase_rules"), &PhaseTransitionController::get_phase_rules);

	ClassDB::bind_method(D_METHOD("set_attribute_set", "set"), &PhaseTransitionController::set_attribute_set);
	ClassDB::bind_method(D_METHOD("get_attribute_set"), &PhaseTransitionController::get_attribute_set);

	ClassDB::bind_method(D_METHOD("set_state_tags", "tags"), &PhaseTransitionController::set_state_tags);
	ClassDB::bind_method(D_METHOD("get_state_tags"), &PhaseTransitionController::get_state_tags);

	ClassDB::bind_method(D_METHOD("get_current_phase"), &PhaseTransitionController::get_current_phase);
	ClassDB::bind_method(D_METHOD("get_is_transitioning"), &PhaseTransitionController::get_is_transitioning);

	ClassDB::bind_method(D_METHOD("check_phase_transition"), &PhaseTransitionController::check_phase_transition);
	ClassDB::bind_method(D_METHOD("transition_to_phase", "phase"), &PhaseTransitionController::transition_to_phase);
	ClassDB::bind_method(D_METHOD("complete_transition"), &PhaseTransitionController::complete_transition);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "phase_rules", PROPERTY_HINT_RESOURCE_TYPE, "PhaseTransitionRule"), "set_phase_rules", "get_phase_rules");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attribute_set", PROPERTY_HINT_RESOURCE_TYPE, "AttributeSet"), "set_attribute_set", "get_attribute_set");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "state_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_state_tags", "get_state_tags");

	ADD_SIGNAL(MethodInfo("phase_transition_started", PropertyInfo(Variant::INT, "old_phase"), PropertyInfo(Variant::INT, "new_phase")));
	ADD_SIGNAL(MethodInfo("phase_transition_completed", PropertyInfo(Variant::INT, "new_phase")));
}

PhaseTransitionController::PhaseTransitionController() {}
PhaseTransitionController::~PhaseTransitionController() {}
