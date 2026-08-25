/**************************************************************************/
/*  defense_component.cpp                                                 */
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

#include "defense_component.h"

void DefenseComponent::_update_state_tags() {
	if (state_tags.is_null()) {
		return;
	}

	state_tags->remove_tag("Combat.State.Parrying");
	state_tags->remove_tag("Combat.State.Guarding");
	state_tags->remove_tag("Combat.State.Staggered");
	state_tags->remove_tag("Combat.State.GuardBroken");

	switch (current_state) {
		case DEFENSE_STATE_PARRY_START:
			state_tags->add_tag("Combat.State.Parrying");
			break;
		case DEFENSE_STATE_GUARDING:
			state_tags->add_tag("Combat.State.Guarding");
			break;
		case DEFENSE_STATE_GUARD_BROKEN:
			state_tags->add_tag("Combat.State.Staggered");
			state_tags->add_tag("Combat.State.GuardBroken");
			break;
		default:
			break;
	}
}

void DefenseComponent::press_defense() {
	if (current_state == DEFENSE_STATE_GUARD_BROKEN) {
		return;
	}

	is_button_held = true;
	fatigue_idle_timer = 0.0;

	real_t effective_window = MAX(parry_window_min, parry_window_base - current_fatigue);
	current_fatigue += spam_fatigue_penalty_per_press;

	current_state = DEFENSE_STATE_PARRY_START;
	state_timer = effective_window;
	_update_state_tags();

	emit_signal(SNAME("defense_pressed"), effective_window);
}

void DefenseComponent::release_defense() {
	is_button_held = false;
	if (current_state == DEFENSE_STATE_GUARDING || current_state == DEFENSE_STATE_PARRY_START) {
		current_state = DEFENSE_STATE_NEUTRAL;
		consecutive_parries = 0;
		_update_state_tags();
		emit_signal(SNAME("defense_released"));
	}
}

void DefenseComponent::on_successful_parry() {
	consecutive_parries++;
	current_fatigue = 0.0; // Reward successful parry by clearing spam fatigue
	emit_signal(SNAME("parry_succeeded"), consecutive_parries);
}

void DefenseComponent::on_guard_broken() {
	current_state = DEFENSE_STATE_GUARD_BROKEN;
	state_timer = guard_break_stagger_duration;
	consecutive_parries = 0;
	is_button_held = false;
	_update_state_tags();
	emit_signal(SNAME("guard_broken"));
}

void DefenseComponent::reset_defense() {
	current_state = DEFENSE_STATE_NEUTRAL;
	state_timer = 0.0;
	current_fatigue = 0.0;
	fatigue_idle_timer = 0.0;
	consecutive_parries = 0;
	is_button_held = false;
	_update_state_tags();
}

void DefenseComponent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_PROCESS: {
			real_t delta = get_process_delta_time();

			// Fatigue recovery
			fatigue_idle_timer += delta;
			if (fatigue_idle_timer >= spam_fatigue_recovery_time && current_fatigue > 0.0) {
				current_fatigue = MAX(0.0, current_fatigue - (delta * 0.5));
			}

			// State timer
			if (state_timer > 0.0) {
				state_timer -= delta;
				if (state_timer <= 0.0) {
					if (current_state == DEFENSE_STATE_PARRY_START) {
						if (is_button_held) {
							current_state = DEFENSE_STATE_GUARDING;
							_update_state_tags();
						} else {
							current_state = DEFENSE_STATE_NEUTRAL;
							_update_state_tags();
						}
					} else if (current_state == DEFENSE_STATE_GUARD_BROKEN) {
						current_state = DEFENSE_STATE_NEUTRAL;
						_update_state_tags();
						emit_signal(SNAME("guard_break_recovered"));
					}
				}
			}
		} break;
	}
}

void DefenseComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_parry_window_base", "val"), &DefenseComponent::set_parry_window_base);
	ClassDB::bind_method(D_METHOD("get_parry_window_base"), &DefenseComponent::get_parry_window_base);

	ClassDB::bind_method(D_METHOD("set_parry_window_min", "val"), &DefenseComponent::set_parry_window_min);
	ClassDB::bind_method(D_METHOD("get_parry_window_min"), &DefenseComponent::get_parry_window_min);

	ClassDB::bind_method(D_METHOD("set_spam_fatigue_penalty", "val"), &DefenseComponent::set_spam_fatigue_penalty);
	ClassDB::bind_method(D_METHOD("get_spam_fatigue_penalty"), &DefenseComponent::get_spam_fatigue_penalty);

	ClassDB::bind_method(D_METHOD("set_spam_fatigue_recovery_time", "val"), &DefenseComponent::set_spam_fatigue_recovery_time);
	ClassDB::bind_method(D_METHOD("get_spam_fatigue_recovery_time"), &DefenseComponent::get_spam_fatigue_recovery_time);

	ClassDB::bind_method(D_METHOD("set_guard_break_stagger_duration", "val"), &DefenseComponent::set_guard_break_stagger_duration);
	ClassDB::bind_method(D_METHOD("get_guard_break_stagger_duration"), &DefenseComponent::get_guard_break_stagger_duration);

	ClassDB::bind_method(D_METHOD("set_attribute_set", "set"), &DefenseComponent::set_attribute_set);
	ClassDB::bind_method(D_METHOD("get_attribute_set"), &DefenseComponent::get_attribute_set);

	ClassDB::bind_method(D_METHOD("set_state_tags", "tags"), &DefenseComponent::set_state_tags);
	ClassDB::bind_method(D_METHOD("get_state_tags"), &DefenseComponent::get_state_tags);

	ClassDB::bind_method(D_METHOD("get_current_state"), &DefenseComponent::get_current_state);
	ClassDB::bind_method(D_METHOD("get_consecutive_parries"), &DefenseComponent::get_consecutive_parries);
	ClassDB::bind_method(D_METHOD("get_current_fatigue"), &DefenseComponent::get_current_fatigue);

	ClassDB::bind_method(D_METHOD("press_defense"), &DefenseComponent::press_defense);
	ClassDB::bind_method(D_METHOD("release_defense"), &DefenseComponent::release_defense);
	ClassDB::bind_method(D_METHOD("on_successful_parry"), &DefenseComponent::on_successful_parry);
	ClassDB::bind_method(D_METHOD("on_guard_broken"), &DefenseComponent::on_guard_broken);
	ClassDB::bind_method(D_METHOD("reset_defense"), &DefenseComponent::reset_defense);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parry_window_base"), "set_parry_window_base", "get_parry_window_base");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parry_window_min"), "set_parry_window_min", "get_parry_window_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spam_fatigue_penalty"), "set_spam_fatigue_penalty", "get_spam_fatigue_penalty");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spam_fatigue_recovery_time"), "set_spam_fatigue_recovery_time", "get_spam_fatigue_recovery_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "guard_break_stagger_duration"), "set_guard_break_stagger_duration", "get_guard_break_stagger_duration");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attribute_set", PROPERTY_HINT_RESOURCE_TYPE, "AttributeSet"), "set_attribute_set", "get_attribute_set");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "state_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_state_tags", "get_state_tags");

	ADD_SIGNAL(MethodInfo("defense_pressed", PropertyInfo(Variant::FLOAT, "active_parry_window")));
	ADD_SIGNAL(MethodInfo("defense_released"));
	ADD_SIGNAL(MethodInfo("parry_succeeded", PropertyInfo(Variant::INT, "consecutive_count")));
	ADD_SIGNAL(MethodInfo("guard_broken"));
	ADD_SIGNAL(MethodInfo("guard_break_recovered"));

	BIND_ENUM_CONSTANT(DEFENSE_STATE_NEUTRAL);
	BIND_ENUM_CONSTANT(DEFENSE_STATE_PARRY_START);
	BIND_ENUM_CONSTANT(DEFENSE_STATE_GUARDING);
	BIND_ENUM_CONSTANT(DEFENSE_STATE_RECOVERY);
	BIND_ENUM_CONSTANT(DEFENSE_STATE_GUARD_BROKEN);
}

DefenseComponent::DefenseComponent() {
	set_process(true);
}

DefenseComponent::~DefenseComponent() {}
