/**************************************************************************/
/*  combat_brain_3d.cpp                                                   */
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

#include "combat_brain_3d.h"
#include "core/math/random_pcg.h"

Node3D *CombatBrain3D::_get_target() {
	if (target_node) {
		return target_node;
	}
	if (!target_path.is_empty()) {
		target_node = Object::cast_to<Node3D>(get_node_or_null(target_path));
		return target_node;
	}
	return nullptr;
}

void CombatBrain3D::register_action_considerations(const StringName &p_action_name, const TypedArray<MoveConsideration> &p_considerations) {
	action_considerations[p_action_name] = p_considerations;
}

TypedArray<MoveConsideration> CombatBrain3D::get_action_considerations(const StringName &p_action_name) const {
	if (action_considerations.has(p_action_name)) {
		return action_considerations[p_action_name];
	}
	return TypedArray<MoveConsideration>();
}

StringName CombatBrain3D::evaluate_best_action() {
	Node3D *target = _get_target();
	Node3D *self_node = this;

	Variant v_self_attrs = get("attribute_set");
	Ref<AttributeSet> self_attrs = v_self_attrs;

	Variant v_self_tags = get("state_tags");
	Ref<GameplayTagContainer> self_tags = v_self_tags;

	Ref<AttributeSet> target_attrs;
	Ref<GameplayTagContainer> target_tags;
	if (target) {
		Variant v_target_attrs = target->get("attribute_set");
		target_attrs = v_target_attrs;
		Variant v_target_tags = target->get("state_tags");
		target_tags = v_target_tags;
	}

	StringName best_action;
	real_t highest_score = -1.0;

	for (int i = 0; i < available_actions.size(); i++) {
		Ref<CombatAction> action = available_actions[i];
		if (action.is_null()) {
			continue;
		}

		StringName a_name = action->get_action_name();

		// Check cooldown
		if (cooldown_timers.has(a_name) && cooldown_timers[a_name] > 0.0) {
			continue;
		}

		// Check legal activation
		if (!action->can_activate(self_attrs, self_tags)) {
			continue;
		}

		real_t score_product = 1.0;
		if (action_considerations.has(a_name)) {
			TypedArray<MoveConsideration> considerations = action_considerations[a_name];
			if (!considerations.is_empty()) {
				for (int c = 0; c < considerations.size(); c++) {
					Ref<MoveConsideration> cons = considerations[c];
					if (cons.is_valid()) {
						real_t s = cons->score(self_node, target, self_attrs, target_attrs, target_tags);
						score_product *= s;
					}
				}
			}
		}

		// Add subtle noise
		if (decision_noise > 0.0) {
			score_product += Math::random(-decision_noise, decision_noise);
		}

		if (score_product > highest_score) {
			highest_score = score_product;
			best_action = a_name;
		}
	}

	if (highest_score > 0.0) {
		return best_action;
	}
	return StringName();
}

void CombatBrain3D::trigger_action(const StringName &p_action_name) {
	for (int i = 0; i < available_actions.size(); i++) {
		Ref<CombatAction> action = available_actions[i];
		if (action.is_valid() && action->get_action_name() == p_action_name) {
			cooldown_timers[p_action_name] = action->get_cooldown();
			emit_signal(SNAME("action_chosen"), p_action_name);
			break;
		}
	}
}

void CombatBrain3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_PROCESS: {
			real_t delta = get_process_delta_time();

			// Tick cooldowns
			for (KeyValue<StringName, real_t> &E : cooldown_timers) {
				if (E.value > 0.0) {
					E.value = MAX(0.0, E.value - delta);
				}
			}

			eval_timer += delta;
			if (eval_timer >= evaluation_interval) {
				eval_timer = 0.0;
				StringName chosen = evaluate_best_action();
				if (chosen != StringName()) {
					trigger_action(chosen);
				}
			}
		} break;
	}
}

void CombatBrain3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_available_actions", "actions"), &CombatBrain3D::set_available_actions);
	ClassDB::bind_method(D_METHOD("get_available_actions"), &CombatBrain3D::get_available_actions);

	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &CombatBrain3D::set_target_path);
	ClassDB::bind_method(D_METHOD("get_target_path"), &CombatBrain3D::get_target_path);

	ClassDB::bind_method(D_METHOD("set_evaluation_interval", "interval"), &CombatBrain3D::set_evaluation_interval);
	ClassDB::bind_method(D_METHOD("get_evaluation_interval"), &CombatBrain3D::get_evaluation_interval);

	ClassDB::bind_method(D_METHOD("set_decision_noise", "noise"), &CombatBrain3D::set_decision_noise);
	ClassDB::bind_method(D_METHOD("get_decision_noise"), &CombatBrain3D::get_decision_noise);

	ClassDB::bind_method(D_METHOD("register_action_considerations", "action_name", "considerations"), &CombatBrain3D::register_action_considerations);
	ClassDB::bind_method(D_METHOD("get_action_considerations", "action_name"), &CombatBrain3D::get_action_considerations);

	ClassDB::bind_method(D_METHOD("evaluate_best_action"), &CombatBrain3D::evaluate_best_action);
	ClassDB::bind_method(D_METHOD("trigger_action", "action_name"), &CombatBrain3D::trigger_action);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "available_actions", PROPERTY_HINT_RESOURCE_TYPE, "CombatAction"), "set_available_actions", "get_available_actions");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"), "set_target_path", "get_target_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "evaluation_interval"), "set_evaluation_interval", "get_evaluation_interval");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "decision_noise"), "set_decision_noise", "get_decision_noise");

	ADD_SIGNAL(MethodInfo("action_chosen", PropertyInfo(Variant::STRING_NAME, "action_name")));
}

CombatBrain3D::CombatBrain3D() {
	set_process(true);
}

CombatBrain3D::~CombatBrain3D() {}
