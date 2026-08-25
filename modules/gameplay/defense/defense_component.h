/**************************************************************************/
/*  defense_component.h                                                   */
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
#include "../core/gameplay_tags.h"
#include "scene/main/node.h"

class DefenseComponent : public Node {
	GDCLASS(DefenseComponent, Node);

public:
	enum DefenseState {
		DEFENSE_STATE_NEUTRAL,
		DEFENSE_STATE_PARRY_START,
		DEFENSE_STATE_GUARDING,
		DEFENSE_STATE_RECOVERY,
		DEFENSE_STATE_GUARD_BROKEN,
	};

private:
	DefenseState current_state = DEFENSE_STATE_NEUTRAL;
	real_t state_timer = 0.0;
	bool is_button_held = false;

	real_t parry_window_base = 0.20; // 200ms default (approx 12 frames @ 60fps)
	real_t parry_window_min = 0.04; // 40ms floor
	real_t spam_fatigue_penalty_per_press = 0.04;
	real_t spam_fatigue_recovery_time = 0.5;
	real_t current_fatigue = 0.0;
	real_t fatigue_idle_timer = 0.0;

	int consecutive_parries = 0;
	real_t guard_break_stagger_duration = 2.0;

	Ref<AttributeSet> attribute_set;
	Ref<GameplayTagContainer> state_tags;

	void _update_state_tags();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_parry_window_base(real_t p_val) { parry_window_base = MAX(0.01, p_val); }
	real_t get_parry_window_base() const { return parry_window_base; }

	void set_parry_window_min(real_t p_val) { parry_window_min = MAX(0.01, p_val); }
	real_t get_parry_window_min() const { return parry_window_min; }

	void set_spam_fatigue_penalty(real_t p_val) { spam_fatigue_penalty_per_press = MAX(0.0, p_val); }
	real_t get_spam_fatigue_penalty() const { return spam_fatigue_penalty_per_press; }

	void set_spam_fatigue_recovery_time(real_t p_val) { spam_fatigue_recovery_time = MAX(0.01, p_val); }
	real_t get_spam_fatigue_recovery_time() const { return spam_fatigue_recovery_time; }

	void set_guard_break_stagger_duration(real_t p_val) { guard_break_stagger_duration = MAX(0.1, p_val); }
	real_t get_guard_break_stagger_duration() const { return guard_break_stagger_duration; }

	void set_attribute_set(const Ref<AttributeSet> &p_set) { attribute_set = p_set; }
	Ref<AttributeSet> get_attribute_set() const { return attribute_set; }

	void set_state_tags(const Ref<GameplayTagContainer> &p_tags) { state_tags = p_tags; }
	Ref<GameplayTagContainer> get_state_tags() const { return state_tags; }

	DefenseState get_current_state() const { return current_state; }
	int get_consecutive_parries() const { return consecutive_parries; }
	real_t get_current_fatigue() const { return current_fatigue; }

	void press_defense();
	void release_defense();
	void on_successful_parry();
	void on_guard_broken();
	void reset_defense();

	DefenseComponent();
	~DefenseComponent();
};

VARIANT_ENUM_CAST(DefenseComponent::DefenseState);
