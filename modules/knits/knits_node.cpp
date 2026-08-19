/**************************************************************************/
/*  knits_node.cpp                                                        */
/**************************************************************************/

#include "knits_node.h"

void KnitNode::_bind_methods() {
	// Bindings for reflection if needed
}

KnitPin *KnitNode::find_pin(KnitPinID p_pin_id) {
	for (int i = 0; i < input_pins.size(); i++) {
		if (input_pins[i].id == p_pin_id) {
			return &input_pins.write[i];
		}
	}
	for (int i = 0; i < output_pins.size(); i++) {
		if (output_pins[i].id == p_pin_id) {
			return &output_pins.write[i];
		}
	}
	return nullptr;
}

const KnitPin *KnitNode::find_pin(KnitPinID p_pin_id) const {
	for (int i = 0; i < input_pins.size(); i++) {
		if (input_pins[i].id == p_pin_id) {
			return &input_pins[i];
		}
	}
	for (int i = 0; i < output_pins.size(); i++) {
		if (output_pins[i].id == p_pin_id) {
			return &output_pins[i];
		}
	}
	return nullptr;
}

KnitPinID KnitNode::add_input_pin(const StringName &p_name, KnitPinKind p_kind, const KnitTypeSignature &p_type, const Variant &p_default_val) {
	KnitPin pin;
	pin.id = KnitIDGenerator::generate();
	pin.owner_node = id;
	pin.name = p_name;
	pin.display_label = String(p_name);
	pin.kind = p_kind;
	pin.direction = KnitPinDirection::Input;
	pin.type = p_type;
	pin.default_value = p_default_val;
	pin.is_connected = false;
	pin.is_hidden = false;
	pin.is_orphaned = false;
	input_pins.push_back(pin);
	return pin.id;
}

KnitPinID KnitNode::add_output_pin(const StringName &p_name, KnitPinKind p_kind, const KnitTypeSignature &p_type) {
	KnitPin pin;
	pin.id = KnitIDGenerator::generate();
	pin.owner_node = id;
	pin.name = p_name;
	pin.display_label = String(p_name);
	pin.kind = p_kind;
	pin.direction = KnitPinDirection::Output;
	pin.type = p_type;
	pin.is_connected = false;
	pin.is_hidden = false;
	pin.is_orphaned = false;
	output_pins.push_back(pin);
	return pin.id;
}

void KnitNode::remove_pin(KnitPinID p_pin_id) {
	for (int i = 0; i < input_pins.size(); i++) {
		if (input_pins[i].id == p_pin_id) {
			input_pins.remove_at(i);
			return;
		}
	}
	for (int i = 0; i < output_pins.size(); i++) {
		if (output_pins[i].id == p_pin_id) {
			output_pins.remove_at(i);
			return;
		}
	}
}

KnitNode::KnitNode() {
	id = KnitIDGenerator::generate();
}

KnitNode::~KnitNode() {
}

///////////////////////////////////////////////////////////////////////////////
// KnitsGraph

void KnitsGraph::_bind_methods() {
}

Ref<KnitNode> KnitsGraph::create_node(KnitNodeCategory p_category, const String &p_title, const Vector2 &p_pos) {
	Ref<KnitNode> node;
	node.instantiate();
	node->category = p_category;
	node->title = p_title;
	node->position = p_pos;
	nodes[node->id] = node;
	return node;
}

bool KnitsGraph::remove_node(KnitNodeID p_node_id) {
	if (!nodes.has(p_node_id)) {
		return false;
	}

	// Remove all connections associated with pins on this node
	for (int i = connections.size() - 1; i >= 0; i--) {
		if (connections[i].from_node == p_node_id || connections[i].to_node == p_node_id) {
			connections.remove_at(i);
		}
	}

	nodes.erase(p_node_id);
	return true;
}

Ref<KnitNode> KnitsGraph::get_node(KnitNodeID p_node_id) const {
	if (nodes.has(p_node_id)) {
		return nodes[p_node_id];
	}
	return Ref<KnitNode>();
}

KnitConnectionID KnitsGraph::connect_pins(KnitNodeID p_from_node, KnitPinID p_from_pin, KnitNodeID p_to_node, KnitPinID p_to_pin) {
	Ref<KnitNode> from_n = get_node(p_from_node);
	Ref<KnitNode> to_n = get_node(p_to_node);
	if (from_n.is_null() || to_n.is_null()) {
		return 0;
	}

	KnitPin *from_p = from_n->find_pin(p_from_pin);
	KnitPin *to_p = to_n->find_pin(p_to_pin);
	if (!from_p || !to_p) {
		return 0;
	}

	// Must be Output -> Input
	if (from_p->direction != KnitPinDirection::Output || to_p->direction != KnitPinDirection::Input) {
		return 0;
	}

	// Kinds must match (Exec-Exec or Data-Data)
	if (from_p->kind != to_p->kind) {
		return 0;
	}

	// For data pins, type compatibility check
	if (from_p->kind == KnitPinKind::Data) {
		if (!from_p->type.is_compatible_with(to_p->type)) {
			return 0;
		}

		// Disconnect any existing connection on data input pin (data input accepts max 1 wire)
		for (int i = connections.size() - 1; i >= 0; i--) {
			if (connections[i].to_pin == p_to_pin) {
				connections.remove_at(i);
			}
		}
	}

	KnitConnection conn;
	conn.id = KnitIDGenerator::generate();
	conn.from_node = p_from_node;
	conn.from_pin = p_from_pin;
	conn.to_node = p_to_node;
	conn.to_pin = p_to_pin;

	from_p->is_connected = true;
	to_p->is_connected = true;

	connections.push_back(conn);
	return conn.id;
}

bool KnitsGraph::disconnect_pins(KnitPinID p_from_pin, KnitPinID p_to_pin) {
	for (int i = 0; i < connections.size(); i++) {
		if (connections[i].from_pin == p_from_pin && connections[i].to_pin == p_to_pin) {
			connections.remove_at(i);
			return true;
		}
	}
	return false;
}

bool KnitsGraph::disconnect_connection(KnitConnectionID p_connection_id) {
	for (int i = 0; i < connections.size(); i++) {
		if (connections[i].id == p_connection_id) {
			connections.remove_at(i);
			return true;
		}
	}
	return false;
}

const KnitConnection *KnitsGraph::get_connection_for_input_pin(KnitPinID p_input_pin) const {
	for (int i = 0; i < connections.size(); i++) {
		if (connections[i].to_pin == p_input_pin) {
			return &connections[i];
		}
	}
	return nullptr;
}

Vector<const KnitConnection *> KnitsGraph::get_connections_for_output_pin(KnitPinID p_output_pin) const {
	Vector<const KnitConnection *> result;
	for (int i = 0; i < connections.size(); i++) {
		if (connections[i].from_pin == p_output_pin) {
			result.push_back(&connections[i]);
		}
	}
	return result;
}

void KnitsGraph::add_variable(const StringName &p_name, const KnitTypeSignature &p_type, const Variant &p_default_val, bool p_exported) {
	KnitVariable v;
	v.name = p_name;
	v.type = p_type;
	v.default_value = p_default_val;
	v.is_exported = p_exported;
	variables[p_name] = v;
}

void KnitsGraph::remove_variable(const StringName &p_name) {
	variables.erase(p_name);
}

bool KnitsGraph::has_variable(const StringName &p_name) const {
	return variables.has(p_name);
}

void KnitsGraph::add_comment_box(const String &p_title, const Rect2 &p_bounds, const Color &p_color) {
	KnitCommentBox box;
	box.id = KnitIDGenerator::generate();
	box.title = p_title;
	box.bounds = p_bounds;
	box.color = p_color;
	comments.push_back(box);
}

KnitsGraph::KnitsGraph() {
	id = KnitIDGenerator::generate();
}

KnitsGraph::~KnitsGraph() {
}
