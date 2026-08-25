/**************************************************************************/
/*  combat_brain_3d.h                                                     */
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

#include "move_consideration.h"
#include "../actions/combat_action.h"
#include "core/templates/hash_map.h"
#include "core/variant/typed_array.h"
#include "scene/3d/node_3d.h"

class CombatBrain3D : public Node3D {
	GDCLASS(CombatBrain3D, Node3D);

private:
	TypedArray<CombatAction> available_actions;
	HashMap<StringName, TypedArray<MoveConsideration>> action_considerations;
	HashMap<StringName, real_t> cooldown_timers;

	NodePath target_path;
	Node3D *target_node = nullptr;

	real_t evaluation_interval = 0.1;
	real_t eval_timer = 0.0;
	real_t decision_noise = 0.05;

	Node3D *_get_target();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_available_actions(const TypedArray<CombatAction> &p_actions) { available_actions = p_actions; }
	TypedArray<CombatAction> get_available_actions() const { return available_actions; }

	void set_target_path(const NodePath &p_path) { target_path = p_path; target_node = nullptr; }
	NodePath get_target_path() const { return target_path; }

	void set_evaluation_interval(real_t p_interval) { evaluation_interval = MAX(0.01, p_interval); }
	real_t get_evaluation_interval() const { return evaluation_interval; }

	void set_decision_noise(real_t p_noise) { decision_noise = MAX(0.0, p_noise); }
	real_t get_decision_noise() const { return decision_noise; }

	void register_action_considerations(const StringName &p_action_name, const TypedArray<MoveConsideration> &p_considerations);
	TypedArray<MoveConsideration> get_action_considerations(const StringName &p_action_name) const;

	StringName evaluate_best_action();
	void trigger_action(const StringName &p_action_name);

	CombatBrain3D();
	~CombatBrain3D();
};
