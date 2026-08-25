/**************************************************************************/
/*  combo_graph.cpp                                                       */
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

#include "combo_graph.h"

// -----------------------------------------------------------------------------
// ComboEdge
// -----------------------------------------------------------------------------

void ComboEdge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_action_name", "name"), &ComboEdge::set_target_action_name);
	ClassDB::bind_method(D_METHOD("get_target_action_name"), &ComboEdge::get_target_action_name);

	ClassDB::bind_method(D_METHOD("set_input_action_name", "input"), &ComboEdge::set_input_action_name);
	ClassDB::bind_method(D_METHOD("get_input_action_name"), &ComboEdge::get_input_action_name);

	ClassDB::bind_method(D_METHOD("set_required_tags", "query"), &ComboEdge::set_required_tags);
	ClassDB::bind_method(D_METHOD("get_required_tags"), &ComboEdge::get_required_tags);

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &ComboEdge::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &ComboEdge::get_priority);

	ClassDB::bind_method(D_METHOD("set_resource_cost_overrides", "overrides"), &ComboEdge::set_resource_cost_overrides);
	ClassDB::bind_method(D_METHOD("get_resource_cost_overrides"), &ComboEdge::get_resource_cost_overrides);

	ClassDB::bind_method(D_METHOD("is_legal", "tags"), &ComboEdge::is_legal);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_action_name"), "set_target_action_name", "get_target_action_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "input_action_name"), "set_input_action_name", "get_input_action_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "required_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_required_tags", "get_required_tags");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "resource_cost_overrides"), "set_resource_cost_overrides", "get_resource_cost_overrides");
}

// -----------------------------------------------------------------------------
// ComboNode
// -----------------------------------------------------------------------------

void ComboNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_action_name", "name"), &ComboNode::set_action_name);
	ClassDB::bind_method(D_METHOD("get_action_name"), &ComboNode::get_action_name);

	ClassDB::bind_method(D_METHOD("set_combat_action", "action"), &ComboNode::set_combat_action);
	ClassDB::bind_method(D_METHOD("get_combat_action"), &ComboNode::get_combat_action);

	ClassDB::bind_method(D_METHOD("set_edges", "edges"), &ComboNode::set_edges);
	ClassDB::bind_method(D_METHOD("get_edges"), &ComboNode::get_edges);
	ClassDB::bind_method(D_METHOD("add_edge", "edge"), &ComboNode::add_edge);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "action_name"), "set_action_name", "get_action_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "combat_action", PROPERTY_HINT_RESOURCE_TYPE, "CombatAction"), "set_combat_action", "get_combat_action");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "edges", PROPERTY_HINT_RESOURCE_TYPE, "ComboEdge"), "set_edges", "get_edges");
}

// -----------------------------------------------------------------------------
// ComboGraph
// -----------------------------------------------------------------------------

void ComboGraph::add_node(const Ref<ComboNode> &p_node) {
	if (p_node.is_valid() && !nodes.has(p_node)) {
		nodes.push_back(p_node);
		emit_changed();
	}
}

Ref<ComboNode> ComboGraph::find_node(const StringName &p_action_name) const {
	for (int i = 0; i < nodes.size(); i++) {
		Ref<ComboNode> n = nodes[i];
		if (n.is_valid() && n->get_action_name() == p_action_name) {
			return n;
		}
	}
	return Ref<ComboNode>();
}

StringName ComboGraph::get_next_action(const StringName &p_current_action, const StringName &p_input, const Ref<GameplayTagContainer> &p_current_tags) const {
	StringName start_node_name = p_current_action.is_empty() ? initial_action_name : p_current_action;
	Ref<ComboNode> current_node = find_node(start_node_name);
	if (current_node.is_null()) {
		return StringName();
	}

	TypedArray<ComboEdge> edges = current_node->get_edges();
	Ref<ComboEdge> best_edge;
	int highest_prio = -999999;

	for (int i = 0; i < edges.size(); i++) {
		Ref<ComboEdge> edge = edges[i];
		if (edge.is_valid() && edge->get_input_action_name() == p_input) {
			if (edge->is_legal(p_current_tags)) {
				if (edge->get_priority() > highest_prio) {
					highest_prio = edge->get_priority();
					best_edge = edge;
				}
			}
		}
	}

	if (best_edge.is_valid()) {
		return best_edge->get_target_action_name();
	}
	return StringName();
}

void ComboGraph::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_initial_action_name", "name"), &ComboGraph::set_initial_action_name);
	ClassDB::bind_method(D_METHOD("get_initial_action_name"), &ComboGraph::get_initial_action_name);

	ClassDB::bind_method(D_METHOD("set_nodes", "nodes"), &ComboGraph::set_nodes);
	ClassDB::bind_method(D_METHOD("get_nodes"), &ComboGraph::get_nodes);

	ClassDB::bind_method(D_METHOD("add_node", "node"), &ComboGraph::add_node);
	ClassDB::bind_method(D_METHOD("find_node", "action_name"), &ComboGraph::find_node);
	ClassDB::bind_method(D_METHOD("get_next_action", "current_action", "input", "tags"), &ComboGraph::get_next_action);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "initial_action_name"), "set_initial_action_name", "get_initial_action_name");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_RESOURCE_TYPE, "ComboNode"), "set_nodes", "get_nodes");
}
