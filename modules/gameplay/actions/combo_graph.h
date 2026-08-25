/**************************************************************************/
/*  combo_graph.h                                                         */
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

#include "combat_action.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "core/variant/typed_array.h"

class ComboEdge : public Resource {
	GDCLASS(ComboEdge, Resource);

private:
	StringName target_action_name;
	StringName input_action_name;
	Ref<GameplayTagQuery> required_tags;
	int priority = 0;
	Dictionary resource_cost_overrides;

protected:
	static void _bind_methods();

public:
	void set_target_action_name(const StringName &p_name) { target_action_name = p_name; emit_changed(); }
	StringName get_target_action_name() const { return target_action_name; }

	void set_input_action_name(const StringName &p_input) { input_action_name = p_input; emit_changed(); }
	StringName get_input_action_name() const { return input_action_name; }

	void set_required_tags(const Ref<GameplayTagQuery> &p_query) { required_tags = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_required_tags() const { return required_tags; }

	void set_priority(int p_prio) { priority = p_prio; emit_changed(); }
	int get_priority() const { return priority; }

	void set_resource_cost_overrides(const Dictionary &p_costs) { resource_cost_overrides = p_costs; emit_changed(); }
	Dictionary get_resource_cost_overrides() const { return resource_cost_overrides; }

	bool is_legal(const Ref<GameplayTagContainer> &p_tags) const {
		if (required_tags.is_valid()) {
			return required_tags->evaluate(p_tags);
		}
		return true;
	}

	ComboEdge() {}
	~ComboEdge() {}
};

class ComboNode : public Resource {
	GDCLASS(ComboNode, Resource);

private:
	StringName action_name;
	Ref<CombatAction> combat_action;
	TypedArray<ComboEdge> edges;

protected:
	static void _bind_methods();

public:
	void set_action_name(const StringName &p_name) { action_name = p_name; emit_changed(); }
	StringName get_action_name() const { return action_name; }

	void set_combat_action(const Ref<CombatAction> &p_action) { combat_action = p_action; emit_changed(); }
	Ref<CombatAction> get_combat_action() const { return combat_action; }

	void set_edges(const TypedArray<ComboEdge> &p_edges) { edges = p_edges; emit_changed(); }
	TypedArray<ComboEdge> get_edges() const { return edges; }

	void add_edge(const Ref<ComboEdge> &p_edge) {
		if (p_edge.is_valid() && !edges.has(p_edge)) {
			edges.push_back(p_edge);
			emit_changed();
		}
	}

	ComboNode() {}
	~ComboNode() {}
};

class ComboGraph : public Resource {
	GDCLASS(ComboGraph, Resource);

private:
	StringName initial_action_name;
	TypedArray<ComboNode> nodes;

protected:
	static void _bind_methods();

public:
	void set_initial_action_name(const StringName &p_name) { initial_action_name = p_name; emit_changed(); }
	StringName get_initial_action_name() const { return initial_action_name; }

	void set_nodes(const TypedArray<ComboNode> &p_nodes) { nodes = p_nodes; emit_changed(); }
	TypedArray<ComboNode> get_nodes() const { return nodes; }

	void add_node(const Ref<ComboNode> &p_node);
	Ref<ComboNode> find_node(const StringName &p_action_name) const;

	StringName get_next_action(const StringName &p_current_action, const StringName &p_input, const Ref<GameplayTagContainer> &p_current_tags) const;

	ComboGraph() {}
	~ComboGraph() {}
};
