/**************************************************************************/
/*  phase_transition_node.h                                               */
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

#include "../actions/combat_action.h"
#include "../attributes/attribute_set.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "core/variant/typed_array.h"
#include "scene/main/node.h"

class PhaseTransitionRule : public Resource {
	GDCLASS(PhaseTransitionRule, Resource);

private:
	int phase_index = 1;
	real_t health_threshold_pct = 0.5;
	StringName transition_animation;
	Ref<AttributeSet> new_attribute_set;
	TypedArray<CombatAction> new_available_actions;
	Ref<GameplayTagContainer> granted_phase_tags;

protected:
	static void _bind_methods();

public:
	void set_phase_index(int p_idx) { phase_index = p_idx; emit_changed(); }
	int get_phase_index() const { return phase_index; }

	void set_health_threshold_pct(real_t p_pct) { health_threshold_pct = CLAMP(p_pct, 0.0, 1.0); emit_changed(); }
	real_t get_health_threshold_pct() const { return health_threshold_pct; }

	void set_transition_animation(const StringName &p_anim) { transition_animation = p_anim; emit_changed(); }
	StringName get_transition_animation() const { return transition_animation; }

	void set_new_attribute_set(const Ref<AttributeSet> &p_attrs) { new_attribute_set = p_attrs; emit_changed(); }
	Ref<AttributeSet> get_new_attribute_set() const { return new_attribute_set; }

	void set_new_available_actions(const TypedArray<CombatAction> &p_actions) { new_available_actions = p_actions; emit_changed(); }
	TypedArray<CombatAction> get_new_available_actions() const { return new_available_actions; }

	void set_granted_phase_tags(const Ref<GameplayTagContainer> &p_tags) { granted_phase_tags = p_tags; emit_changed(); }
	Ref<GameplayTagContainer> get_granted_phase_tags() const { return granted_phase_tags; }

	PhaseTransitionRule() {}
	~PhaseTransitionRule() {}
};

class PhaseTransitionController : public Node {
	GDCLASS(PhaseTransitionController, Node);

private:
	TypedArray<PhaseTransitionRule> phase_rules;
	int current_phase = 0;
	bool is_transitioning = false;

	Ref<AttributeSet> attribute_set;
	Ref<GameplayTagContainer> state_tags;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_phase_rules(const TypedArray<PhaseTransitionRule> &p_rules) { phase_rules = p_rules; }
	TypedArray<PhaseTransitionRule> get_phase_rules() const { return phase_rules; }

	void set_attribute_set(const Ref<AttributeSet> &p_set) { attribute_set = p_set; }
	Ref<AttributeSet> get_attribute_set() const { return attribute_set; }

	void set_state_tags(const Ref<GameplayTagContainer> &p_tags) { state_tags = p_tags; }
	Ref<GameplayTagContainer> get_state_tags() const { return state_tags; }

	int get_current_phase() const { return current_phase; }
	bool get_is_transitioning() const { return is_transitioning; }

	void check_phase_transition();
	void transition_to_phase(int p_phase);
	void complete_transition();

	PhaseTransitionController();
	~PhaseTransitionController();
};
