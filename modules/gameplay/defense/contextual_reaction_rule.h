/**************************************************************************/
/*  contextual_reaction_rule.h                                            */
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

#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/typed_array.h"

class ContextualReactionRule : public Resource {
	GDCLASS(ContextualReactionRule, Resource);

private:
	StringName rule_name;
	Ref<GameplayTagQuery> incoming_threat_query;
	Ref<GameplayTagQuery> self_state_query;
	StringName required_input_action;
	int priority = 0;
	StringName resulting_action_name;

protected:
	static void _bind_methods();

public:
	void set_rule_name(const StringName &p_name) { rule_name = p_name; emit_changed(); }
	StringName get_rule_name() const { return rule_name; }

	void set_incoming_threat_query(const Ref<GameplayTagQuery> &p_query) { incoming_threat_query = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_incoming_threat_query() const { return incoming_threat_query; }

	void set_self_state_query(const Ref<GameplayTagQuery> &p_query) { self_state_query = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_self_state_query() const { return self_state_query; }

	void set_required_input_action(const StringName &p_input) { required_input_action = p_input; emit_changed(); }
	StringName get_required_input_action() const { return required_input_action; }

	void set_priority(int p_prio) { priority = p_prio; emit_changed(); }
	int get_priority() const { return priority; }

	void set_resulting_action_name(const StringName &p_action) { resulting_action_name = p_action; emit_changed(); }
	StringName get_resulting_action_name() const { return resulting_action_name; }

	bool matches(const Ref<GameplayTagContainer> &p_threat_tags, const Ref<GameplayTagContainer> &p_self_tags, const StringName &p_input) const;

	ContextualReactionRule() {}
	~ContextualReactionRule() {}
};

class ContextualReactionEngine : public Object {
	GDCLASS(ContextualReactionEngine, Object);

private:
	static ContextualReactionEngine *singleton;
	TypedArray<ContextualReactionRule> rules;

protected:
	static void _bind_methods();

public:
	static ContextualReactionEngine *get_singleton() { return singleton; }

	void set_rules(const TypedArray<ContextualReactionRule> &p_rules) { rules = p_rules; }
	TypedArray<ContextualReactionRule> get_rules() const { return rules; }

	void add_rule(const Ref<ContextualReactionRule> &p_rule);
	void remove_rule(const Ref<ContextualReactionRule> &p_rule);
	void clear_rules();

	StringName evaluate_reaction(const Ref<GameplayTagContainer> &p_threat_tags, const Ref<GameplayTagContainer> &p_self_tags, const StringName &p_input) const;

	ContextualReactionEngine();
	~ContextualReactionEngine();
};
