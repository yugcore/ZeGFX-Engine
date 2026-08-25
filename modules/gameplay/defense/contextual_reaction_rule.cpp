/**************************************************************************/
/*  contextual_reaction_rule.cpp                                          */
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

#include "contextual_reaction_rule.h"

ContextualReactionEngine *ContextualReactionEngine::singleton = nullptr;

// -----------------------------------------------------------------------------
// ContextualReactionRule
// -----------------------------------------------------------------------------

bool ContextualReactionRule::matches(const Ref<GameplayTagContainer> &p_threat_tags, const Ref<GameplayTagContainer> &p_self_tags, const StringName &p_input) const {
	if (required_input_action != StringName() && required_input_action != p_input) {
		return false;
	}
	if (incoming_threat_query.is_valid()) {
		if (!incoming_threat_query->evaluate(p_threat_tags)) {
			return false;
		}
	}
	if (self_state_query.is_valid()) {
		if (!self_state_query->evaluate(p_self_tags)) {
			return false;
		}
	}
	return true;
}

void ContextualReactionRule::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rule_name", "name"), &ContextualReactionRule::set_rule_name);
	ClassDB::bind_method(D_METHOD("get_rule_name"), &ContextualReactionRule::get_rule_name);

	ClassDB::bind_method(D_METHOD("set_incoming_threat_query", "query"), &ContextualReactionRule::set_incoming_threat_query);
	ClassDB::bind_method(D_METHOD("get_incoming_threat_query"), &ContextualReactionRule::get_incoming_threat_query);

	ClassDB::bind_method(D_METHOD("set_self_state_query", "query"), &ContextualReactionRule::set_self_state_query);
	ClassDB::bind_method(D_METHOD("get_self_state_query"), &ContextualReactionRule::get_self_state_query);

	ClassDB::bind_method(D_METHOD("set_required_input_action", "input"), &ContextualReactionRule::set_required_input_action);
	ClassDB::bind_method(D_METHOD("get_required_input_action"), &ContextualReactionRule::get_required_input_action);

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &ContextualReactionRule::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &ContextualReactionRule::get_priority);

	ClassDB::bind_method(D_METHOD("set_resulting_action_name", "action"), &ContextualReactionRule::set_resulting_action_name);
	ClassDB::bind_method(D_METHOD("get_resulting_action_name"), &ContextualReactionRule::get_resulting_action_name);

	ClassDB::bind_method(D_METHOD("matches", "threat_tags", "self_tags", "input"), &ContextualReactionRule::matches);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "rule_name"), "set_rule_name", "get_rule_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "incoming_threat_query", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_incoming_threat_query", "get_incoming_threat_query");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "self_state_query", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_self_state_query", "get_self_state_query");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "required_input_action"), "set_required_input_action", "get_required_input_action");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "resulting_action_name"), "set_resulting_action_name", "get_resulting_action_name");
}

// -----------------------------------------------------------------------------
// ContextualReactionEngine
// -----------------------------------------------------------------------------

void ContextualReactionEngine::add_rule(const Ref<ContextualReactionRule> &p_rule) {
	if (p_rule.is_valid() && !rules.has(p_rule)) {
		rules.push_back(p_rule);
	}
}

void ContextualReactionEngine::remove_rule(const Ref<ContextualReactionRule> &p_rule) {
	if (rules.has(p_rule)) {
		rules.erase(p_rule);
	}
}

void ContextualReactionEngine::clear_rules() {
	rules.clear();
}

StringName ContextualReactionEngine::evaluate_reaction(const Ref<GameplayTagContainer> &p_threat_tags, const Ref<GameplayTagContainer> &p_self_tags, const StringName &p_input) const {
	Ref<ContextualReactionRule> best_rule;
	int highest_prio = -999999;

	for (int i = 0; i < rules.size(); i++) {
		Ref<ContextualReactionRule> rule = rules[i];
		if (rule.is_valid() && rule->matches(p_threat_tags, p_self_tags, p_input)) {
			if (rule->get_priority() > highest_prio) {
				highest_prio = rule->get_priority();
				best_rule = rule;
			}
		}
	}

	if (best_rule.is_valid()) {
		return best_rule->get_resulting_action_name();
	}
	return StringName();
}

void ContextualReactionEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rules", "rules"), &ContextualReactionEngine::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &ContextualReactionEngine::get_rules);

	ClassDB::bind_method(D_METHOD("add_rule", "rule"), &ContextualReactionEngine::add_rule);
	ClassDB::bind_method(D_METHOD("remove_rule", "rule"), &ContextualReactionEngine::remove_rule);
	ClassDB::bind_method(D_METHOD("clear_rules"), &ContextualReactionEngine::clear_rules);
	ClassDB::bind_method(D_METHOD("evaluate_reaction", "threat_tags", "self_tags", "input"), &ContextualReactionEngine::evaluate_reaction);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_RESOURCE_TYPE, "ContextualReactionRule"), "set_rules", "get_rules");
}

ContextualReactionEngine::ContextualReactionEngine() {
	singleton = this;
}

ContextualReactionEngine::~ContextualReactionEngine() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
