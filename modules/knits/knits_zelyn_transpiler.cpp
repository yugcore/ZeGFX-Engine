/**************************************************************************/
/*  knits_zelyn_transpiler.cpp                                            */
/**************************************************************************/

#include "knits_zelyn_transpiler.h"
#include "core/variant/variant_utility.h"

String KnitsZelynTranspiler::_export_pure_expr(const KnitsGraph &p_graph, const KnitPin &p_pin, HashSet<KnitNodeID> &p_visited) {
	const KnitConnection *conn = p_graph.get_connection_for_input_pin(p_pin.id);
	if (!conn) {
		if (p_pin.default_value.get_type() == Variant::STRING) {
			return vformat("\"%s\"", String(p_pin.default_value));
		} else if (p_pin.default_value.get_type() == Variant::BOOL) {
			return (bool)p_pin.default_value ? "true" : "false";
		}
		return String(p_pin.default_value);
	}
	Ref<KnitNode> from_node = p_graph.get_node(conn->from_node);
	if (from_node.is_null()) return "nil";

	String title = from_node->title.to_lower();
	if (title == "math expression" || title == "expression") {
		return from_node->target_symbol.is_empty() ? "0" : String(from_node->target_symbol);
	} else if (title == "add (+)" || title == "add") {
		String a = _export_pure_expr(p_graph, from_node->input_pins[0], p_visited);
		String b = _export_pure_expr(p_graph, from_node->input_pins[1], p_visited);
		return vformat("(%s + %s)", a, b);
	} else if (title == "subtract (-)" || title == "subtract") {
		String a = _export_pure_expr(p_graph, from_node->input_pins[0], p_visited);
		String b = _export_pure_expr(p_graph, from_node->input_pins[1], p_visited);
		return vformat("(%s - %s)", a, b);
	} else if (title == "multiply (*)" || title == "multiply") {
		String a = _export_pure_expr(p_graph, from_node->input_pins[0], p_visited);
		String b = _export_pure_expr(p_graph, from_node->input_pins[1], p_visited);
		return vformat("(%s * %s)", a, b);
	} else if (title == "divide (/)" || title == "divide") {
		String a = _export_pure_expr(p_graph, from_node->input_pins[0], p_visited);
		String b = _export_pure_expr(p_graph, from_node->input_pins[1], p_visited);
		return vformat("(%s / %s)", a, b);
	} else if (from_node->category == KnitNodeCategory::Event) {
		for (int i = 0; i < from_node->output_pins.size(); i++) {
			if (from_node->output_pins[i].id == conn->from_pin) {
				return from_node->output_pins[i].name;
			}
		}
	}

	return from_node->target_symbol.is_empty() ? from_node->title : String(from_node->target_symbol);
}

String KnitsZelynTranspiler::_export_exec_chain(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, int p_indent, HashSet<KnitNodeID> &p_visited) {
	if (p_node.is_null()) return "";
	if (p_visited.has(p_node->id)) return "";
	p_visited.insert(p_node->id);

	String indent_str;
	for (int i = 0; i < p_indent; i++) indent_str += "\t";

	String code;
	String title = p_node->title.to_lower();

	if (title == "branch" || title == "if") {
		String cond = _export_pure_expr(p_graph, p_node->input_pins[1], p_visited);
		code += vformat("%sif (%s) {\n", indent_str, cond);

		const KnitPin *true_pin = nullptr;
		const KnitPin *false_pin = nullptr;
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			if (p_node->output_pins[i].name == "True") true_pin = &p_node->output_pins[i];
			else if (p_node->output_pins[i].name == "False") false_pin = &p_node->output_pins[i];
		}

		if (true_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(true_pin->id);
			if (conns.size() > 0) {
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				code += _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
			}
		}
		code += indent_str + "}";

		if (false_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(false_pin->id);
			if (conns.size() > 0) {
				code += " else {\n";
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				code += _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
				code += indent_str + "}\n";
			} else {
				code += "\n";
			}
		} else {
			code += "\n";
		}
		return code;
	} else if (title == "while loop" || title == "while") {
		String cond = _export_pure_expr(p_graph, p_node->input_pins[1], p_visited);
		code += vformat("%swhile (%s) {\n", indent_str, cond);

		const KnitPin *body_pin = nullptr;
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			if (p_node->output_pins[i].name == "LoopBody") body_pin = &p_node->output_pins[i];
		}

		if (body_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(body_pin->id);
			if (conns.size() > 0) {
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				code += _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
			}
		}
		code += indent_str + "}\n";
		return code;
	} else if (title == "return") {
		String val = (p_node->input_pins.size() > 1) ? _export_pure_expr(p_graph, p_node->input_pins[1], p_visited) : "";
		if (val.is_empty()) {
			code += vformat("%sreturn;\n", indent_str);
		} else {
			code += vformat("%sreturn %s;\n", indent_str, val);
		}
		return code;
	} else if (title == "wait" || title == "wait_seconds") {
		String dur = (p_node->input_pins.size() > 1) ? _export_pure_expr(p_graph, p_node->input_pins[1], p_visited) : "1.0";
		code += vformat("%swait(%s);\n", indent_str, dur);
	} else if (title.begins_with("set ")) {
		String prop = p_node->target_symbol.is_empty() ? p_node->title.substr(4) : String(p_node->target_symbol);
		String val = (p_node->input_pins.size() > 1) ? _export_pure_expr(p_graph, p_node->input_pins[1], p_visited) : "nil";
		code += vformat("%s%s = %s;\n", indent_str, prop, val);
	} else {
		String fn = p_node->target_symbol.is_empty() ? p_node->title : String(p_node->target_symbol);
		String args;
		for (int a = 1; a < p_node->input_pins.size(); a++) {
			if (a > 1) args += ", ";
			args += _export_pure_expr(p_graph, p_node->input_pins[a], p_visited);
		}
		code += vformat("%s%s(%s);\n", indent_str, fn, args);
	}

	// Follow execution wire
	const KnitPin *out_exec = nullptr;
	for (int i = 0; i < p_node->output_pins.size(); i++) {
		if (p_node->output_pins[i].kind == KnitPinKind::Execution) {
			out_exec = &p_node->output_pins[i];
			break;
		}
	}

	if (out_exec) {
		Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(out_exec->id);
		if (conns.size() > 0) {
			Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
			code += _export_exec_chain(p_graph, next, p_indent, p_visited);
		}
	}

	return code;
}

bool KnitsZelynTranspiler::knit_graph_to_zelyn(const Ref<KnitsGraph> &p_graph, String &r_code, String &r_error) {
	if (p_graph.is_null()) {
		r_error = "Graph is null";
		return false;
	}

	r_code = "// Generated by ZeGFX KnitNodes to Zelyn Transpiler\n\n";

	HashSet<KnitNodeID> visited;

	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
		const Ref<KnitNode> &node = E.value;
		if (node.is_valid() && node->category == KnitNodeCategory::Event) {
			String fn_name = node->target_symbol.is_empty() ? node->title : String(node->target_symbol);
			if (fn_name.begins_with("Event: ")) {
				fn_name = fn_name.substr(7);
			}

			// Map standard engine event names to clean Zelyn lifecycle conventions
			if (fn_name == "_ready" || fn_name == "Ready Event") {
				fn_name = "on_ready";
			} else if (fn_name == "_process" || fn_name == "Process Event") {
				fn_name = "on_process";
			} else if (fn_name == "_physics_process" || fn_name == "Physics Process Event") {
				fn_name = "on_physics_process";
			} else if (fn_name == "_input" || fn_name == "Input Event") {
				fn_name = "on_input";
			}

			String args = "self";
			for (int a = 0; a < node->output_pins.size(); a++) {
				if (node->output_pins[a].kind == KnitPinKind::Data) {
					args += ", ";
					args += node->output_pins[a].name;
				}
			}

			r_code += vformat("func %s(%s) {\n", fn_name, args);

			const KnitPin *out_exec = nullptr;
			for (int i = 0; i < node->output_pins.size(); i++) {
				if (node->output_pins[i].kind == KnitPinKind::Execution) {
					out_exec = &node->output_pins[i];
					break;
				}
			}

			if (out_exec) {
				Vector<const KnitConnection *> conns = p_graph->get_connections_for_output_pin(out_exec->id);
				if (conns.size() > 0) {
					Ref<KnitNode> first = p_graph->get_node(conns[0]->to_node);
					String body = _export_exec_chain(*p_graph.ptr(), first, 1, visited);
					r_code += body.is_empty() ? "\t\n}\n\n" : (body + "}\n\n");
				} else {
					r_code += "}\n\n";
				}
			} else {
				r_code += "}\n\n";
			}
		}
	}

	return true;
}

bool KnitsZelynTranspiler::zelyn_to_knit_graph(const String &p_code, Ref<KnitsGraph> &r_graph, String &r_error) {
	if (r_graph.is_null()) {
		r_graph.instantiate();
	}

	// Minimal parser generating an entry event node from the first function
	Ref<KnitNode> ready_node = r_graph->create_node(KnitNodeCategory::Event, "on_ready");
	ready_node->position = Vector2(100, 100);

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	ready_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	return true;
}
