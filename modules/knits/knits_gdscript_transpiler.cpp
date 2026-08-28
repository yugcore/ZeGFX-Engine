/**************************************************************************/
/*  knits_gdscript_transpiler.cpp                                         */
/**************************************************************************/

#include "knits_gdscript_transpiler.h"
#include "modules/gdscript/gdscript_parser.h"
#include "core/variant/variant_utility.h"

String KnitsGDScriptTranspiler::_expression_to_string(const GDScriptParser::ExpressionNode *p_expr) {
	if (!p_expr) return "";
	switch (p_expr->type) {
		case GDScriptParser::Node::LITERAL: {
			const GDScriptParser::LiteralNode *lit = static_cast<const GDScriptParser::LiteralNode *>(p_expr);
			if (lit->value.get_type() == Variant::STRING) {
				return vformat("\"%s\"", String(lit->value));
			}
			return String(lit->value);
		}
		case GDScriptParser::Node::IDENTIFIER: {
			const GDScriptParser::IdentifierNode *id = static_cast<const GDScriptParser::IdentifierNode *>(p_expr);
			return String(id->name);
		}
		case GDScriptParser::Node::BINARY_OPERATOR: {
			const GDScriptParser::BinaryOpNode *b = static_cast<const GDScriptParser::BinaryOpNode *>(p_expr);
			String op = "+";
			switch (b->operation) {
				case GDScriptParser::BinaryOpNode::OP_ADDITION: op = "+"; break;
				case GDScriptParser::BinaryOpNode::OP_SUBTRACTION: op = "-"; break;
				case GDScriptParser::BinaryOpNode::OP_MULTIPLICATION: op = "*"; break;
				case GDScriptParser::BinaryOpNode::OP_DIVISION: op = "/"; break;
				case GDScriptParser::BinaryOpNode::OP_MODULO: op = "%"; break;
				case GDScriptParser::BinaryOpNode::OP_POWER: op = "**"; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_EQUAL: op = "=="; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_NOT_EQUAL: op = "!="; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_LESS: op = "<"; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_LESS_EQUAL: op = "<="; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_GREATER: op = ">"; break;
				case GDScriptParser::BinaryOpNode::OP_COMP_GREATER_EQUAL: op = ">="; break;
				case GDScriptParser::BinaryOpNode::OP_LOGIC_AND: op = "and"; break;
				case GDScriptParser::BinaryOpNode::OP_LOGIC_OR: op = "or"; break;
				default: op = "+"; break;
			}
			return vformat("(%s %s %s)", _expression_to_string(b->left_operand), op, _expression_to_string(b->right_operand));
		}
		case GDScriptParser::Node::UNARY_OPERATOR: {
			const GDScriptParser::UnaryOpNode *u = static_cast<const GDScriptParser::UnaryOpNode *>(p_expr);
			return vformat("(-%s)", _expression_to_string(u->operand));
		}
		case GDScriptParser::Node::CALL: {
			const GDScriptParser::CallNode *c = static_cast<const GDScriptParser::CallNode *>(p_expr);
			String args;
			int arg_count = (int)c->arguments.size();
			for (int a = 0; a < arg_count; a++) {
				if (a > 0) args += ", ";
				args += _expression_to_string(c->arguments[a]);
			}
			if (c->callee) {
				return vformat("%s.%s(%s)", _expression_to_string(c->callee), String(c->function_name), args);
			}
			return vformat("%s(%s)", String(c->function_name), args);
		}
		case GDScriptParser::Node::SUBSCRIPT: {
			const GDScriptParser::SubscriptNode *s = static_cast<const GDScriptParser::SubscriptNode *>(p_expr);
			if (s->is_attribute) {
				return vformat("%s.%s", _expression_to_string(s->base), String(s->attribute->name));
			}
			return vformat("%s[%s]", _expression_to_string(s->base), _expression_to_string(s->index));
		}
		default:
			break;
	}
	return "0";
}

void KnitsGDScriptTranspiler::_import_statement(const GDScriptParser::Node *p_stmt, const Ref<KnitsGraph> &p_graph, KnitNodeID &r_last_node, KnitPinID &r_last_exec_pin, Vector2 &r_pos) {
	if (!p_stmt || p_graph.is_null()) return;

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;
	KnitTypeSignature sig_bool;
	sig_bool.kind = KnitDataType::Bool;
	KnitTypeSignature sig_str;
	sig_str.kind = KnitDataType::String;
	KnitTypeSignature sig_wild;
	sig_wild.kind = KnitDataType::Wildcard;

	switch (p_stmt->type) {
		case GDScriptParser::Node::IF: {
			const GDScriptParser::IfNode *if_node = static_cast<const GDScriptParser::IfNode *>(p_stmt);
			Ref<KnitNode> branch = p_graph->create_node(KnitNodeCategory::FlowControl, "Branch", r_pos);
			KnitPinID in_pin = branch->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
			KnitPinID cond_pin = branch->add_input_pin("Condition", KnitPinKind::Data, sig_bool, true);
			KnitPinID true_pin = branch->add_output_pin("True", KnitPinKind::Execution, sig_exec);
			KnitPinID false_pin = branch->add_output_pin("False", KnitPinKind::Execution, sig_exec);

			if (r_last_exec_pin != 0) {
				p_graph->connect_pins(r_last_node, r_last_exec_pin, branch->id, in_pin);
			}

			// Condition expression
			String cond_str = _expression_to_string(if_node->condition);
			if (cond_str == "true") {
				branch->input_pins.write[1].default_value = true;
			} else if (cond_str == "false") {
				branch->input_pins.write[1].default_value = false;
			} else if (!cond_str.is_empty()) {
				Ref<KnitNode> expr = p_graph->create_node(KnitNodeCategory::PureFunction, "Math Expression", r_pos + Vector2(-220, 60));
				expr->target_symbol = cond_str;
				KnitPinID expr_out = expr->add_output_pin("Result", KnitPinKind::Data, sig_bool);
				p_graph->connect_pins(expr->id, expr_out, branch->id, cond_pin);
			}

			// True branch
			KnitNodeID t_last = branch->id;
			KnitPinID t_exec = true_pin;
			Vector2 t_pos = r_pos + Vector2(320, -60);
			_import_suite(if_node->true_block, p_graph, t_last, t_exec, t_pos);

			// False branch
			if (if_node->false_block) {
				KnitNodeID f_last = branch->id;
				KnitPinID f_exec = false_pin;
				Vector2 f_pos = r_pos + Vector2(320, 160);
				_import_suite(if_node->false_block, p_graph, f_last, f_exec, f_pos);
			}

			r_last_node = t_last;
			r_last_exec_pin = t_exec;
			r_pos.x = MAX(t_pos.x, r_pos.x + 350);
		} break;

		case GDScriptParser::Node::WHILE: {
			const GDScriptParser::WhileNode *while_node = static_cast<const GDScriptParser::WhileNode *>(p_stmt);
			Ref<KnitNode> loop = p_graph->create_node(KnitNodeCategory::FlowControl, "While Loop", r_pos);
			KnitPinID in_pin = loop->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
			KnitPinID cond_pin = loop->add_input_pin("Condition", KnitPinKind::Data, sig_bool, true);
			KnitPinID body_pin = loop->add_output_pin("LoopBody", KnitPinKind::Execution, sig_exec);
			KnitPinID comp_pin = loop->add_output_pin("Completed", KnitPinKind::Execution, sig_exec);

			if (r_last_exec_pin != 0) {
				p_graph->connect_pins(r_last_node, r_last_exec_pin, loop->id, in_pin);
			}

			String cond_str = _expression_to_string(while_node->condition);
			if (!cond_str.is_empty()) {
				Ref<KnitNode> expr = p_graph->create_node(KnitNodeCategory::PureFunction, "Math Expression", r_pos + Vector2(-220, 60));
				expr->target_symbol = cond_str;
				KnitPinID expr_out = expr->add_output_pin("Result", KnitPinKind::Data, sig_bool);
				p_graph->connect_pins(expr->id, expr_out, loop->id, cond_pin);
			}

			KnitNodeID b_last = loop->id;
			KnitPinID b_exec = body_pin;
			Vector2 b_pos = r_pos + Vector2(320, -50);
			_import_suite(while_node->loop, p_graph, b_last, b_exec, b_pos);

			r_last_node = loop->id;
			r_last_exec_pin = comp_pin;
			r_pos.x += 350;
		} break;

		case GDScriptParser::Node::FOR: {
			const GDScriptParser::ForNode *for_node = static_cast<const GDScriptParser::ForNode *>(p_stmt);
			Ref<KnitNode> loop = p_graph->create_node(KnitNodeCategory::FlowControl, "For Each Loop", r_pos);
			KnitPinID in_pin = loop->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
			loop->add_input_pin("Collection", KnitPinKind::Data, sig_wild);
			KnitPinID body_pin = loop->add_output_pin("LoopBody", KnitPinKind::Execution, sig_exec);
			loop->add_output_pin("Element", KnitPinKind::Data, sig_wild);
			KnitPinID comp_pin = loop->add_output_pin("Completed", KnitPinKind::Execution, sig_exec);

			if (r_last_exec_pin != 0) {
				p_graph->connect_pins(r_last_node, r_last_exec_pin, loop->id, in_pin);
			}

			KnitNodeID b_last = loop->id;
			KnitPinID b_exec = body_pin;
			Vector2 b_pos = r_pos + Vector2(320, -50);
			_import_suite(for_node->loop, p_graph, b_last, b_exec, b_pos);

			r_last_node = loop->id;
			r_last_exec_pin = comp_pin;
			r_pos.x += 350;
		} break;

		case GDScriptParser::Node::RETURN: {
			const GDScriptParser::ReturnNode *ret_node = static_cast<const GDScriptParser::ReturnNode *>(p_stmt);
			Ref<KnitNode> ret = p_graph->create_node(KnitNodeCategory::ImpureAction, "return", r_pos);
			KnitPinID in_pin = ret->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
			KnitPinID val_pin = ret->add_input_pin("val", KnitPinKind::Data, sig_wild);

			if (r_last_exec_pin != 0) {
				p_graph->connect_pins(r_last_node, r_last_exec_pin, ret->id, in_pin);
			}

			if (ret_node->return_value) {
				String val_str = _expression_to_string(ret_node->return_value);
				Ref<KnitNode> expr = p_graph->create_node(KnitNodeCategory::PureFunction, "Math Expression", r_pos + Vector2(-220, 50));
				expr->target_symbol = val_str;
				KnitPinID expr_out = expr->add_output_pin("Result", KnitPinKind::Data, sig_wild);
				p_graph->connect_pins(expr->id, expr_out, ret->id, val_pin);
			}

			r_last_node = ret->id;
			r_last_exec_pin = 0; // Terminal
			r_pos.x += 320;
		} break;

		case GDScriptParser::Node::CALL: {
			const GDScriptParser::CallNode *call_node = static_cast<const GDScriptParser::CallNode *>(p_stmt);
			String fn = String(call_node->function_name);

			Ref<KnitNode> act;
			if (fn == "move_and_slide") {
				act = p_graph->create_node(KnitNodeCategory::ImpureAction, "move_and_slide", r_pos);
				KnitPinID in_pin = act->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
				KnitPinID out_pin = act->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
				if (r_last_exec_pin != 0) {
					p_graph->connect_pins(r_last_node, r_last_exec_pin, act->id, in_pin);
				}
				r_last_node = act->id;
				r_last_exec_pin = out_pin;
			} else {
				act = p_graph->create_node(KnitNodeCategory::ImpureAction, fn, r_pos);
				act->target_symbol = fn;
				KnitPinID in_pin = act->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
				int arg_count = (int)call_node->arguments.size();
				for (int a = 0; a < arg_count; a++) {
					String arg_name = vformat("arg%d", a);
					String arg_val = _expression_to_string(call_node->arguments[a]);
					KnitPinID arg_pin = act->add_input_pin(arg_name, KnitPinKind::Data, sig_wild, arg_val);
				}
				KnitPinID out_pin = act->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
				if (r_last_exec_pin != 0) {
					p_graph->connect_pins(r_last_node, r_last_exec_pin, act->id, in_pin);
				}
				r_last_node = act->id;
				r_last_exec_pin = out_pin;
			}
			r_pos.x += 320;
		} break;

		case GDScriptParser::Node::ASSIGNMENT: {
			const GDScriptParser::AssignmentNode *assign = static_cast<const GDScriptParser::AssignmentNode *>(p_stmt);
			String target_name = _expression_to_string(assign->assignee);
			String val_name = _expression_to_string(assign->assigned_value);

			Ref<KnitNode> set_node = p_graph->create_node(KnitNodeCategory::ImpureAction, vformat("Set %s", target_name), r_pos);
			set_node->target_symbol = target_name;
			KnitPinID in_pin = set_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
			KnitPinID val_pin = set_node->add_input_pin("Value", KnitPinKind::Data, sig_wild, val_name);
			KnitPinID out_pin = set_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

			if (r_last_exec_pin != 0) {
				p_graph->connect_pins(r_last_node, r_last_exec_pin, set_node->id, in_pin);
			}

			r_last_node = set_node->id;
			r_last_exec_pin = out_pin;
			r_pos.x += 320;
		} break;

		default:
			break;
	}
}

void KnitsGDScriptTranspiler::_import_suite(const GDScriptParser::SuiteNode *p_suite, const Ref<KnitsGraph> &p_graph, KnitNodeID &r_last_node, KnitPinID &r_last_exec_pin, Vector2 &r_pos) {
	if (!p_suite) return;
	int stmt_count = (int)p_suite->statements.size();
	for (int i = 0; i < stmt_count; i++) {
		_import_statement(p_suite->statements[i], p_graph, r_last_node, r_last_exec_pin, r_pos);
	}
}

bool KnitsGDScriptTranspiler::gdscript_to_knit_graph(const String &p_code, Ref<KnitsGraph> &r_graph, String &r_error) {
	if (r_graph.is_null()) {
		r_graph.instantiate();
	}
	r_graph->nodes.clear();
	r_graph->connections.clear();

	GDScriptParser parser;
	Error err = parser.parse(p_code, "", false);
	if (err != OK) {
		r_error = parser.get_errors().is_empty() ? "GDScript parse failed" : parser.get_errors().front()->get().message;
		return false;
	}

	const GDScriptParser::ClassNode *tree = parser.get_tree();
	if (!tree) {
		r_error = "Empty or invalid syntax tree";
		return false;
	}

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;
	KnitTypeSignature sig_wild;
	sig_wild.kind = KnitDataType::Wildcard;

	Vector2 func_pos(100, 100);

	int member_count = (int)tree->members.size();
	for (int m = 0; m < member_count; m++) {
		const GDScriptParser::ClassNode::Member &member = tree->members[m];
		if (member.type == GDScriptParser::ClassNode::Member::FUNCTION) {
			const GDScriptParser::FunctionNode *fn = member.function;
			if (!fn || !fn->identifier) continue;

			String fn_name = String(fn->identifier->name);
			Ref<KnitNode> event_node = r_graph->create_node(KnitNodeCategory::Event, vformat("Event: %s", fn_name), func_pos);
			event_node->target_symbol = fn_name;
			KnitPinID event_out = event_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

			// Add parameters as data output pins
			int param_count = (int)fn->parameters.size();
			for (int p = 0; p < param_count; p++) {
				if (fn->parameters[p] && fn->parameters[p]->identifier) {
					String param_name = String(fn->parameters[p]->identifier->name);
					event_node->add_output_pin(param_name, KnitPinKind::Data, sig_wild);
				}
			}

			KnitNodeID last_node = event_node->id;
			KnitPinID last_exec = event_out;
			Vector2 cur_pos = func_pos + Vector2(300, 0);

			_import_suite(fn->body, r_graph, last_node, last_exec, cur_pos);

			func_pos.y += 380;
		}
	}

	return true;
}

String KnitsGDScriptTranspiler::_export_pure_expr(const KnitsGraph &p_graph, const KnitPin &p_pin, HashSet<KnitNodeID> &p_visited) {
	const KnitConnection *conn = p_graph.get_connection_for_input_pin(p_pin.id);
	if (!conn) {
		if (p_pin.default_value.get_type() == Variant::STRING) {
			return vformat("\"%s\"", String(p_pin.default_value));
		}
		return String(p_pin.default_value);
	}

	Ref<KnitNode> from_node = p_graph.get_node(conn->from_node);
	if (from_node.is_null()) return "null";

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
		// Output from event parameter
		for (int i = 0; i < from_node->output_pins.size(); i++) {
			if (from_node->output_pins[i].id == conn->from_pin) {
				return from_node->output_pins[i].name;
			}
		}
	}

	return from_node->target_symbol.is_empty() ? from_node->title : String(from_node->target_symbol);
}

String KnitsGDScriptTranspiler::_export_exec_chain(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, int p_indent, HashSet<KnitNodeID> &p_visited) {
	if (p_node.is_null()) return "";
	if (p_visited.has(p_node->id)) return "";
	p_visited.insert(p_node->id);

	String indent_str;
	for (int i = 0; i < p_indent; i++) indent_str += "\t";

	String code;
	String title = p_node->title.to_lower();

	if (title == "branch" || title == "if") {
		String cond = _export_pure_expr(p_graph, p_node->input_pins[1], p_visited);
		code += vformat("%sif %s:\n", indent_str, cond);

		// True branch
		const KnitPin *true_pin = nullptr;
		const KnitPin *false_pin = nullptr;
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			if (p_node->output_pins[i].name == "True") true_pin = &p_node->output_pins[i];
			else if (p_node->output_pins[i].name == "False") false_pin = &p_node->output_pins[i];
		}

		bool has_true = false;
		if (true_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(true_pin->id);
			if (conns.size() > 0) {
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				String block = _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
				if (!block.is_empty()) {
					code += block;
					has_true = true;
				}
			}
		}
		if (!has_true) code += indent_str + "\tpass\n";

		if (false_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(false_pin->id);
			if (conns.size() > 0) {
				code += vformat("%selse:\n", indent_str);
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				String block = _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
				code += block.is_empty() ? (indent_str + "\tpass\n") : block;
			}
		}
		return code;
	} else if (title == "while loop" || title == "while") {
		String cond = _export_pure_expr(p_graph, p_node->input_pins[1], p_visited);
		code += vformat("%swhile %s:\n", indent_str, cond);

		const KnitPin *body_pin = nullptr;
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			if (p_node->output_pins[i].name == "LoopBody") body_pin = &p_node->output_pins[i];
		}

		bool has_body = false;
		if (body_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(body_pin->id);
			if (conns.size() > 0) {
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				String block = _export_exec_chain(p_graph, next, p_indent + 1, p_visited);
				if (!block.is_empty()) {
					code += block;
					has_body = true;
				}
			}
		}
		if (!has_body) code += indent_str + "\tpass\n";

		// Completed branch
		const KnitPin *comp_pin = nullptr;
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			if (p_node->output_pins[i].name == "Completed") comp_pin = &p_node->output_pins[i];
		}
		if (comp_pin) {
			Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(comp_pin->id);
			if (conns.size() > 0) {
				Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
				code += _export_exec_chain(p_graph, next, p_indent, p_visited);
			}
		}
		return code;
	} else if (title == "return") {
		String val = (p_node->input_pins.size() > 1) ? _export_pure_expr(p_graph, p_node->input_pins[1], p_visited) : "";
		if (val.is_empty()) {
			code += vformat("%sreturn\n", indent_str);
		} else {
			code += vformat("%sreturn %s\n", indent_str, val);
		}
		return code;
	} else if (title == "move_and_slide") {
		code += vformat("%smove_and_slide()\n", indent_str);
	} else if (title.begins_with("set ")) {
		String prop = p_node->target_symbol.is_empty() ? p_node->title.substr(4) : String(p_node->target_symbol);
		String val = (p_node->input_pins.size() > 1) ? _export_pure_expr(p_graph, p_node->input_pins[1], p_visited) : "null";
		code += vformat("%s%s = %s\n", indent_str, prop, val);
	} else {
		String fn = p_node->target_symbol.is_empty() ? p_node->title : String(p_node->target_symbol);
		String args;
		for (int a = 1; a < p_node->input_pins.size(); a++) {
			if (a > 1) args += ", ";
			args += _export_pure_expr(p_graph, p_node->input_pins[a], p_visited);
		}
		code += vformat("%s%s(%s)\n", indent_str, fn, args);
	}

	// Follow execution wire
	KnitPin *out_exec = nullptr;
	for (int i = 0; i < p_node->output_pins.size(); i++) {
		if (p_node->output_pins[i].kind == KnitPinKind::Execution) {
			out_exec = &p_node->output_pins.write[i];
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

bool KnitsGDScriptTranspiler::knit_graph_to_gdscript(const Ref<KnitsGraph> &p_graph, String &r_code, String &r_error) {
	if (p_graph.is_null()) {
		r_error = "Graph is null";
		return false;
	}

	r_code = "# Generated by ZeGFX KnitNodes Transpiler\n";
	r_code += "extends Node\n\n";

	HashSet<KnitNodeID> visited;

	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
		const Ref<KnitNode> &node = E.value;
		if (node.is_valid() && node->category == KnitNodeCategory::Event) {
			String fn_name = node->target_symbol.is_empty() ? node->title : String(node->target_symbol);
			if (fn_name.begins_with("Event: ")) {
				fn_name = fn_name.substr(7);
			}

			String args;
			for (int a = 0; a < node->output_pins.size(); a++) {
				if (node->output_pins[a].kind == KnitPinKind::Data) {
					if (!args.is_empty()) args += ", ";
					args += node->output_pins[a].name;
				}
			}

			r_code += vformat("func %s(%s):\n", fn_name, args);

			KnitPin *out_exec = nullptr;
			for (int i = 0; i < node->output_pins.size(); i++) {
				if (node->output_pins[i].kind == KnitPinKind::Execution) {
					out_exec = &node->output_pins.write[i];
					break;
				}
			}

			if (out_exec) {
				Vector<const KnitConnection *> conns = p_graph->get_connections_for_output_pin(out_exec->id);
				if (conns.size() > 0) {
					Ref<KnitNode> first = p_graph->get_node(conns[0]->to_node);
					String body = _export_exec_chain(*p_graph.ptr(), first, 1, visited);
					r_code += body.is_empty() ? "\tpass\n\n" : (body + "\n");
				} else {
					r_code += "\tpass\n\n";
				}
			} else {
				r_code += "\tpass\n\n";
			}
		}
	}

	return true;
}
