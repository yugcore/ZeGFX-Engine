/**************************************************************************/
/*  gameplay_effect.cpp                                                   */
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

#include "gameplay_effect.h"

// -----------------------------------------------------------------------------
// GameplayEffect
// -----------------------------------------------------------------------------

void GameplayEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_effect_id", "id"), &GameplayEffect::set_effect_id);
	ClassDB::bind_method(D_METHOD("get_effect_id"), &GameplayEffect::get_effect_id);

	ClassDB::bind_method(D_METHOD("set_duration_type", "type"), &GameplayEffect::set_duration_type);
	ClassDB::bind_method(D_METHOD("get_duration_type"), &GameplayEffect::get_duration_type);

	ClassDB::bind_method(D_METHOD("set_duration", "duration"), &GameplayEffect::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &GameplayEffect::get_duration);

	ClassDB::bind_method(D_METHOD("set_period", "period"), &GameplayEffect::set_period);
	ClassDB::bind_method(D_METHOD("get_period"), &GameplayEffect::get_period);

	ClassDB::bind_method(D_METHOD("set_stacking_policy", "policy"), &GameplayEffect::set_stacking_policy);
	ClassDB::bind_method(D_METHOD("get_stacking_policy"), &GameplayEffect::get_stacking_policy);

	ClassDB::bind_method(D_METHOD("set_max_stacks", "max_stacks"), &GameplayEffect::set_max_stacks);
	ClassDB::bind_method(D_METHOD("get_max_stacks"), &GameplayEffect::get_max_stacks);

	ClassDB::bind_method(D_METHOD("set_modifiers", "modifiers"), &GameplayEffect::set_modifiers);
	ClassDB::bind_method(D_METHOD("get_modifiers"), &GameplayEffect::get_modifiers);

	ClassDB::bind_method(D_METHOD("set_execution_calculation", "calculation"), &GameplayEffect::set_execution_calculation);
	ClassDB::bind_method(D_METHOD("get_execution_calculation"), &GameplayEffect::get_execution_calculation);

	ClassDB::bind_method(D_METHOD("set_granted_tags", "tags"), &GameplayEffect::set_granted_tags);
	ClassDB::bind_method(D_METHOD("get_granted_tags"), &GameplayEffect::get_granted_tags);

	ClassDB::bind_method(D_METHOD("set_application_requirements", "query"), &GameplayEffect::set_application_requirements);
	ClassDB::bind_method(D_METHOD("get_application_requirements"), &GameplayEffect::get_application_requirements);

	ClassDB::bind_method(D_METHOD("set_custom_parameters", "params"), &GameplayEffect::set_custom_parameters);
	ClassDB::bind_method(D_METHOD("get_custom_parameters"), &GameplayEffect::get_custom_parameters);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "effect_id"), "set_effect_id", "get_effect_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_type", PROPERTY_HINT_ENUM, "Instant,Has Duration,Infinite"), "set_duration_type", "get_duration_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "period"), "set_period", "get_period");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stacking_policy", PROPERTY_HINT_ENUM, "Refresh Duration,Accumulate Stacks,Ignore While Active"), "set_stacking_policy", "get_stacking_policy");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_stacks"), "set_max_stacks", "get_max_stacks");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "modifiers", PROPERTY_HINT_RESOURCE_TYPE, "AttributeModifier"), "set_modifiers", "get_modifiers");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "execution_calculation", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffectExecution"), "set_execution_calculation", "get_execution_calculation");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "granted_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_granted_tags", "get_granted_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "application_requirements", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_application_requirements", "get_application_requirements");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "custom_parameters"), "set_custom_parameters", "get_custom_parameters");

	BIND_ENUM_CONSTANT(DURATION_INSTANT);
	BIND_ENUM_CONSTANT(DURATION_HAS_DURATION);
	BIND_ENUM_CONSTANT(DURATION_INFINITE);

	BIND_ENUM_CONSTANT(STACK_REFRESH_DURATION);
	BIND_ENUM_CONSTANT(STACK_ACCUMULATE);
	BIND_ENUM_CONSTANT(STACK_IGNORE_WHILE_ACTIVE);
}

// -----------------------------------------------------------------------------
// ActiveGameplayEffect
// -----------------------------------------------------------------------------

void ActiveGameplayEffect::init(const Ref<GameplayEffect> &p_effect, const StringName &p_source_id) {
	effect = p_effect;
	source_id = p_source_id;
	current_stacks = 1;
	if (effect.is_valid()) {
		time_remaining = effect->get_duration();
		period_timer = effect->get_period();
	}
}

bool ActiveGameplayEffect::tick(real_t p_delta, bool &r_execute_periodic) {
	r_execute_periodic = false;
	if (effect.is_null()) {
		return false; // Expired
	}

	if (effect->get_period() > 0.0) {
		period_timer -= p_delta;
		if (period_timer <= 0.0) {
			period_timer += effect->get_period();
			r_execute_periodic = true;
		}
	}

	if (effect->get_duration_type() == GameplayEffect::DURATION_HAS_DURATION) {
		time_remaining -= p_delta;
		if (time_remaining <= 0.0) {
			return false; // Expired
		}
	}

	return true; // Still active
}

void ActiveGameplayEffect::refresh_duration() {
	if (effect.is_valid()) {
		time_remaining = effect->get_duration();
	}
}

bool ActiveGameplayEffect::add_stack() {
	if (effect.is_valid() && current_stacks < effect->get_max_stacks()) {
		current_stacks++;
		refresh_duration();
		return true;
	}
	return false;
}

void ActiveGameplayEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_effect"), &ActiveGameplayEffect::get_effect);
	ClassDB::bind_method(D_METHOD("get_time_remaining"), &ActiveGameplayEffect::get_time_remaining);
	ClassDB::bind_method(D_METHOD("get_current_stacks"), &ActiveGameplayEffect::get_current_stacks);
	ClassDB::bind_method(D_METHOD("get_source_id"), &ActiveGameplayEffect::get_source_id);
}
