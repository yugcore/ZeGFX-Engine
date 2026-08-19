/**************************************************************************/
/*  knits_serializer.cpp                                                  */
/**************************************************************************/

#include "knits_serializer.h"
#include "core/io/json.h"

static String _category_to_string(KnitNodeCategory p_cat) {
	switch (p_cat) {
		case KnitNodeCategory::Event: return "Event";
		case KnitNodeCategory::ImpureAction: return "ImpureAction";
		case KnitNodeCategory::PureFunction: return "PureFunction";
		case KnitNodeCategory::FlowControl: return "FlowControl";
		case KnitNodeCategory::VariableGet: return "VariableGet";
		case KnitNodeCategory::VariableSet: return "VariableSet";
		case KnitNodeCategory::SubGraph: return "SubGraph";
		case KnitNodeCategory::Reroute: return "Reroute";
		case KnitNodeCategory::Comment: return "Comment";
		default: return "ImpureAction";
	}
}

static KnitNodeCategory _string_to_category(const String &p_str) {
	if (p_str == "Event") return KnitNodeCategory::Event;
	if (p_str == "PureFunction") return KnitNodeCategory::PureFunction;
	if (p_str == "FlowControl") return KnitNodeCategory::FlowControl;
	if (p_str == "VariableGet") return KnitNodeCategory::VariableGet;
	if (p_str == "VariableSet") return KnitNodeCategory::VariableSet;
	if (p_str == "SubGraph") return KnitNodeCategory::SubGraph;
	if (p_str == "Reroute") return KnitNodeCategory::Reroute;
	if (p_str == "Comment") return KnitNodeCategory::Comment;
	return KnitNodeCategory::ImpureAction;
}

static String _data_type_to_string(KnitDataType p_type) {
	switch (p_type) {
		case KnitDataType::Bool: return "Bool";
		case KnitDataType::Int32: return "Int32";
		case KnitDataType::Int64: return "Int64";
		case KnitDataType::Float: return "Float";
		case KnitDataType::Double: return "Double";
		case KnitDataType::String: return "String";
		case KnitDataType::StringName: return "StringName";
		case KnitDataType::Vector2: return "Vector2";
		case KnitDataType::Vector3: return "Vector3";
		case KnitDataType::Color: return "Color";
		case KnitDataType::Transform3D: return "Transform3D";
		case KnitDataType::ObjectRef: return "ObjectRef";
		case KnitDataType::Enum: return "Enum";
		case KnitDataType::Array: return "Array";
		case KnitDataType::Dictionary: return "Dictionary";
		case KnitDataType::Wildcard: return "Wildcard";
		default: return "Void";
	}
}

static KnitDataType _string_to_data_type(const String &p_str) {
	if (p_str == "Bool") return KnitDataType::Bool;
	if (p_str == "Int32") return KnitDataType::Int32;
	if (p_str == "Int64") return KnitDataType::Int64;
	if (p_str == "Float") return KnitDataType::Float;
	if (p_str == "Double") return KnitDataType::Double;
	if (p_str == "String") return KnitDataType::String;
	if (p_str == "StringName") return KnitDataType::StringName;
	if (p_str == "Vector2") return KnitDataType::Vector2;
	if (p_str == "Vector3") return KnitDataType::Vector3;
	if (p_str == "Color") return KnitDataType::Color;
	if (p_str == "Transform3D") return KnitDataType::Transform3D;
	if (p_str == "ObjectRef") return KnitDataType::ObjectRef;
	if (p_str == "Enum") return KnitDataType::Enum;
	if (p_str == "Array") return KnitDataType::Array;
	if (p_str == "Dictionary") return KnitDataType::Dictionary;
	if (p_str == "Wildcard") return KnitDataType::Wildcard;
	return KnitDataType::Void;
}

String KnitsSerializer::serialize(const Ref<KnitsGraph> &p_graph) {
	if (p_graph.is_null()) {
		return "knit_version: 2\ngraph_name: \"MainGraph\"\n";
	}

	String out = "knit_version: 2\n";
	out += vformat("graph_name: \"%s\"\n", p_graph->graph_name);
	out += vformat("is_function: %s\n", p_graph->is_function ? "true" : "false");

	// Variables
	if (p_graph->variables.size() > 0) {
		out += "\nvariables:\n";
		for (const KeyValue<StringName, KnitVariable> &E : p_graph->variables) {
			const KnitVariable &v = E.value;
			out += vformat("  %s:\n", String(v.name));
			out += vformat("    type: \"%s\"\n", _data_type_to_string(v.type.kind));
			out += vformat("    default: %s\n", JSON::stringify(v.default_value));
			out += vformat("    exported: %s\n", v.is_exported ? "true" : "false");
		}
	}

	// Nodes (Sorted by UUID hex for deterministic diffs)
	out += "\nnodes:\n";
	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
		const Ref<KnitNode> &node = E.value;
		if (node.is_null()) continue;

		out += vformat("  0x%X:\n", (uint64_t)node->id);
		out += vformat("    title: \"%s\"\n", node->title);
		out += vformat("    category: \"%s\"\n", _category_to_string(node->category));
		if (!node->target_symbol.is_empty()) {
			out += vformat("    target: \"%s\"\n", String(node->target_symbol));
		}
		if (node->signature_hash != 0) {
			out += vformat("    signature_hash: 0x%X\n", node->signature_hash);
		}
		if (!node->macro_resource_path.is_empty()) {
			out += vformat("    graph_ref: \"%s\"\n", node->macro_resource_path);
		}
		out += vformat("    pos: [%d, %d]\n", (int)node->position.x, (int)node->position.y);

		out += "    pins:\n";
		for (int i = 0; i < node->input_pins.size(); i++) {
			const KnitPin &p = node->input_pins[i];
			out += vformat("      in_%s: { id: 0x%X, name: \"%s\", kind: \"%s\", type: \"%s\", dir: \"Input\" }\n",
				String(p.name), (uint64_t)p.id, String(p.name),
				p.kind == KnitPinKind::Execution ? "Execution" : "Data",
				_data_type_to_string(p.type.kind));
		}
		for (int i = 0; i < node->output_pins.size(); i++) {
			const KnitPin &p = node->output_pins[i];
			out += vformat("      out_%s: { id: 0x%X, name: \"%s\", kind: \"%s\", type: \"%s\", dir: \"Output\" }\n",
				String(p.name), (uint64_t)p.id, String(p.name),
				p.kind == KnitPinKind::Execution ? "Execution" : "Data",
				_data_type_to_string(p.type.kind));
		}
	}

	// Connections
	out += "\nconnections:\n";
	for (int i = 0; i < p_graph->connections.size(); i++) {
		const KnitConnection &c = p_graph->connections[i];
		out += vformat("  - id: 0x%X\n", (uint64_t)c.id);
		out += vformat("    from: [0x%X, 0x%X]\n", (uint64_t)c.from_node, (uint64_t)c.from_pin);
		out += vformat("    to:   [0x%X, 0x%X]\n", (uint64_t)c.to_node, (uint64_t)c.to_pin);
	}

	// Comments
	if (p_graph->comments.size() > 0) {
		out += "\ncomments:\n";
		for (int i = 0; i < p_graph->comments.size(); i++) {
			const KnitCommentBox &cb = p_graph->comments[i];
			out += vformat("  - id: 0x%X\n", (uint64_t)cb.id);
			out += vformat("    title: \"%s\"\n", cb.title);
			out += vformat("    bounds: [%d, %d, %d, %d]\n", (int)cb.bounds.position.x, (int)cb.bounds.position.y, (int)cb.bounds.size.x, (int)cb.bounds.size.y);
			out += vformat("    color: \"%s\"\n", cb.color.to_html());
		}
	}

	return out;
}

bool KnitsSerializer::deserialize(const String &p_text, Ref<KnitsGraph> &r_graph, String &r_error) {
	if (r_graph.is_null()) {
		r_graph.instantiate();
	}

	r_graph->nodes.clear();
	r_graph->connections.clear();
	r_graph->variables.clear();
	r_graph->comments.clear();

	// Basic Line-Oriented YAML Parser
	PackedStringArray lines = p_text.split("\n");
	String current_section;
	Ref<KnitNode> current_node;

	for (int l = 0; l < lines.size(); l++) {
		String line = lines[l].strip_edges(false, true); // Keep leading whitespace
		String stripped = line.strip_edges();
		if (stripped.is_empty() || stripped.begins_with("#")) {
			continue;
		}

		if (!line.begins_with(" ") && stripped.ends_with(":")) {
			current_section = stripped.substr(0, stripped.length() - 1);
			continue;
		}

		if (current_section.is_empty()) {
			if (stripped.begins_with("graph_name:")) {
				r_graph->graph_name = stripped.get_slice(":", 1).strip_edges().unquote();
			} else if (stripped.begins_with("is_function:")) {
				r_graph->is_function = stripped.get_slice(":", 1).strip_edges() == "true";
			}
		} else if (current_section == "nodes") {
			if (line.begins_with("  0x") && stripped.ends_with(":")) {
				uint64_t node_id = stripped.substr(0, stripped.length() - 1).hex_to_int();
				current_node.instantiate();
				current_node->id = node_id;
				r_graph->nodes[node_id] = current_node;
			} else if (current_node.is_valid()) {
				if (stripped.begins_with("title:")) {
					current_node->title = stripped.get_slice(":", 1).strip_edges().unquote();
				} else if (stripped.begins_with("category:")) {
					current_node->category = _string_to_category(stripped.get_slice(":", 1).strip_edges().unquote());
				} else if (stripped.begins_with("target:")) {
					current_node->target_symbol = StringName(stripped.get_slice(":", 1).strip_edges().unquote());
				} else if (stripped.begins_with("signature_hash:")) {
					current_node->signature_hash = (uint32_t)stripped.get_slice(":", 1).strip_edges().hex_to_int();
				} else if (stripped.begins_with("graph_ref:")) {
					current_node->macro_resource_path = stripped.get_slice(":", 1).strip_edges().unquote();
				} else if (stripped.begins_with("pos:")) {
					String pos_str = stripped.get_slice("[", 1).get_slice("]", 0);
					PackedStringArray parts = pos_str.split(",");
					if (parts.size() >= 2) {
						current_node->position = Vector2(parts[0].to_float(), parts[1].to_float());
					}
				} else if (stripped.begins_with("in_") || stripped.begins_with("out_")) {
					// Parse pin: in_name: { id: 0x..., name: "...", kind: "...", type: "...", dir: "..." }
					String pin_data = stripped.get_slice("{", 1).get_slice("}", 0);
					PackedStringArray kv_pairs = pin_data.split(",");
					KnitPin pin;
					pin.owner_node = current_node->id;
					for (int k = 0; k < kv_pairs.size(); k++) {
						String kv = kv_pairs[k].strip_edges();
						String key = kv.get_slice(":", 0).strip_edges();
						String val = kv.get_slice(":", 1).strip_edges().unquote();
						if (key == "id") {
							pin.id = val.hex_to_int();
						} else if (key == "name") {
							pin.name = StringName(val);
							pin.display_label = val;
						} else if (key == "kind") {
							pin.kind = val == "Execution" ? KnitPinKind::Execution : KnitPinKind::Data;
						} else if (key == "type") {
							pin.type.kind = _string_to_data_type(val);
						} else if (key == "dir") {
							pin.direction = val == "Output" ? KnitPinDirection::Output : KnitPinDirection::Input;
						}
					}
					if (pin.direction == KnitPinDirection::Output) {
						current_node->output_pins.push_back(pin);
					} else {
						current_node->input_pins.push_back(pin);
					}
				}
			}
		} else if (current_section == "connections") {
			if (stripped.begins_with("- id:")) {
				KnitConnection conn;
				conn.id = stripped.get_slice(":", 1).strip_edges().hex_to_int();
				// Read following from / to lines if present
				if (l + 1 < lines.size() && lines[l + 1].strip_edges().begins_with("from:")) {
					String from_str = lines[l + 1].strip_edges().get_slice("[", 1).get_slice("]", 0);
					PackedStringArray parts = from_str.split(",");
					if (parts.size() >= 2) {
						conn.from_node = parts[0].strip_edges().hex_to_int();
						conn.from_pin = parts[1].strip_edges().hex_to_int();
					}
					l++;
				}
				if (l + 1 < lines.size() && lines[l + 1].strip_edges().begins_with("to:")) {
					String to_str = lines[l + 1].strip_edges().get_slice("[", 1).get_slice("]", 0);
					PackedStringArray parts = to_str.split(",");
					if (parts.size() >= 2) {
						conn.to_node = parts[0].strip_edges().hex_to_int();
						conn.to_pin = parts[1].strip_edges().hex_to_int();
					}
					l++;
				}
				r_graph->connections.push_back(conn);
			}
		}
	}

	return true;
}

Error KnitsSerializer::save_to_file(const Ref<KnitsGraph> &p_graph, const String &p_path) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	if (file.is_null()) {
		return ERR_FILE_CANT_WRITE;
	}

	String text = serialize(p_graph);
	file->store_string(text);
	return OK;
}

Ref<KnitsGraph> KnitsSerializer::load_from_file(const String &p_path, Error *r_error) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_null()) {
		if (r_error) *r_error = ERR_FILE_NOT_FOUND;
		return Ref<KnitsGraph>();
	}

	String text = file->get_as_utf8_string();
	Ref<KnitsGraph> graph;
	graph.instantiate();
	String err_msg;
	if (!deserialize(text, graph, err_msg)) {
		if (r_error) *r_error = ERR_PARSE_ERROR;
		return Ref<KnitsGraph>();
	}

	if (r_error) *r_error = OK;
	return graph;
}
