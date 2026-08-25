/**************************************************************************/
/*  gameplay_effect.h                                                     */
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

#include "attribute_set.h"
#include "gameplay_effect_execution.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "core/variant/typed_array.h"

class GameplayEffect : public Resource {
	GDCLASS(GameplayEffect, Resource);

public:
	enum DurationType {
		DURATION_INSTANT,
		DURATION_HAS_DURATION,
		DURATION_INFINITE,
	};

	enum StackingPolicy {
		STACK_REFRESH_DURATION,
		STACK_ACCUMULATE,
		STACK_IGNORE_WHILE_ACTIVE,
	};

private:
	StringName effect_id;
	DurationType duration_type = DURATION_INSTANT;
	real_t duration = 0.0;
	real_t period = 0.0; // Periodic tick interval (0.0 = no periodic execution)
	StackingPolicy stacking_policy = STACK_REFRESH_DURATION;
	int max_stacks = 1;

	TypedArray<AttributeModifier> modifiers;
	Ref<GameplayEffectExecution> execution_calculation;
	Ref<GameplayTagContainer> granted_tags;
	Ref<GameplayTagQuery> application_requirements;
	Dictionary custom_parameters;

protected:
	static void _bind_methods();

public:
	void set_effect_id(const StringName &p_id) { effect_id = p_id; }
	StringName get_effect_id() const { return effect_id; }

	void set_duration_type(DurationType p_type) { duration_type = p_type; }
	DurationType get_duration_type() const { return duration_type; }

	void set_duration(real_t p_dur) { duration = p_dur; }
	real_t get_duration() const { return duration; }

	void set_period(real_t p_period) { period = p_period; }
	real_t get_period() const { return period; }

	void set_stacking_policy(StackingPolicy p_policy) { stacking_policy = p_policy; }
	StackingPolicy get_stacking_policy() const { return stacking_policy; }

	void set_max_stacks(int p_max) { max_stacks = p_max; }
	int get_max_stacks() const { return max_stacks; }

	void set_modifiers(const TypedArray<AttributeModifier> &p_mods) { modifiers = p_mods; }
	TypedArray<AttributeModifier> get_modifiers() const { return modifiers; }

	void set_execution_calculation(const Ref<GameplayEffectExecution> &p_exec) { execution_calculation = p_exec; }
	Ref<GameplayEffectExecution> get_execution_calculation() const { return execution_calculation; }

	void set_granted_tags(const Ref<GameplayTagContainer> &p_tags) { granted_tags = p_tags; }
	Ref<GameplayTagContainer> get_granted_tags() const { return granted_tags; }

	void set_application_requirements(const Ref<GameplayTagQuery> &p_query) { application_requirements = p_query; }
	Ref<GameplayTagQuery> get_application_requirements() const { return application_requirements; }

	void set_custom_parameters(const Dictionary &p_params) { custom_parameters = p_params; }
	Dictionary get_custom_parameters() const { return custom_parameters; }

	GameplayEffect() {}
	~GameplayEffect() {}
};

VARIANT_ENUM_CAST(GameplayEffect::DurationType);
VARIANT_ENUM_CAST(GameplayEffect::StackingPolicy);

class ActiveGameplayEffect : public RefCounted {
	GDCLASS(ActiveGameplayEffect, RefCounted);

private:
	Ref<GameplayEffect> effect;
	real_t time_remaining = 0.0;
	real_t period_timer = 0.0;
	int current_stacks = 1;
	StringName source_id;

protected:
	static void _bind_methods();

public:
	void init(const Ref<GameplayEffect> &p_effect, const StringName &p_source_id);
	bool tick(real_t p_delta, bool &r_execute_periodic);
	void refresh_duration();
	bool add_stack();

	Ref<GameplayEffect> get_effect() const { return effect; }
	real_t get_time_remaining() const { return time_remaining; }
	int get_current_stacks() const { return current_stacks; }
	StringName get_source_id() const { return source_id; }

	ActiveGameplayEffect() {}
	~ActiveGameplayEffect() {}
};
