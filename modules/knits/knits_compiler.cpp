/**************************************************************************/
/*  knits_compiler.cpp                                                    */
/**************************************************************************/

#include "knits_compiler.h"

namespace {

struct ExprToken {
	enum Type {
		END,
		NUMBER,
		IDENT,
		PLUS,
		MINUS,
		STAR,
		SLASH,
		PERCENT,
		CARET,
		LPAREN,
		RPAREN,
		COMMA
	} type = END;
	double number_val = 0.0;
	String ident_val;
};

class ExprLexer {
	String src;
	int pos = 0;

public:
	ExprLexer(const String &p_src) : src(p_src), pos(0) {}

	ExprToken next_token() {
		while (pos < src.length() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) {
			pos++;
		}
		if (pos >= src.length()) {
			return { ExprToken::END };
		}

		char32_t c = src[pos];

		if (is_digit(c) || (c == '.' && pos + 1 < src.length() && is_digit(src[pos + 1]))) {
			int start = pos;
			while (pos < src.length() && (is_digit(src[pos]) || src[pos] == '.')) {
				pos++;
			}
			String num_str = src.substr(start, pos - start);
			ExprToken tok;
			tok.type = ExprToken::NUMBER;
			tok.number_val = num_str.to_float();
			return tok;
		}

		if (is_unicode_identifier_start(c) || c == '_') {
			int start = pos;
			while (pos < src.length() && (is_unicode_identifier_continue(src[pos]) || src[pos] == '_')) {
				pos++;
			}
			ExprToken tok;
			tok.type = ExprToken::IDENT;
			tok.ident_val = src.substr(start, pos - start);
			return tok;
		}

		pos++;
		switch (c) {
			case '+': return { ExprToken::PLUS };
			case '-': return { ExprToken::MINUS };
			case '*': return { ExprToken::STAR };
			case '/': return { ExprToken::SLASH };
			case '%': return { ExprToken::PERCENT };
			case '^': return { ExprToken::CARET };
			case '(': return { ExprToken::LPAREN };
			case ')': return { ExprToken::RPAREN };
			case ',': return { ExprToken::COMMA };
			default: return { ExprToken::END };
		}
	}
};

class ExprParser {
	ExprLexer lexer;
	ExprToken current_tok;
	KnitsCompiler *compiler;
	KnitCompiledGraph &compiled;
	const HashMap<String, uint8_t> &var_map;
	KnitNodeID debug_node_id;

	void advance() {
		current_tok = lexer.next_token();
	}

public:
	ExprParser(const String &p_expr, KnitsCompiler *p_compiler, KnitCompiledGraph &r_compiled, const HashMap<String, uint8_t> &p_var_map, KnitNodeID p_debug_node_id)
		: lexer(p_expr), compiler(p_compiler), compiled(r_compiled), var_map(p_var_map), debug_node_id(p_debug_node_id) {
		advance();
	}

	uint8_t parse_expr() {
		uint8_t left = parse_term();
		while (current_tok.type == ExprToken::PLUS || current_tok.type == ExprToken::MINUS) {
			ExprToken::Type op = current_tok.type;
			advance();
			uint8_t right = parse_term();
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			inst.opcode = (op == ExprToken::PLUS) ? KnitOpcode::ADD_F : KnitOpcode::SUB_F;
			inst.dst = dst;
			inst.src_a = left;
			inst.src_b = right;
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			left = dst;
		}
		return left;
	}

	uint8_t parse_term() {
		uint8_t left = parse_power();
		while (current_tok.type == ExprToken::STAR || current_tok.type == ExprToken::SLASH || current_tok.type == ExprToken::PERCENT) {
			ExprToken::Type op = current_tok.type;
			advance();
			uint8_t right = parse_power();
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			if (op == ExprToken::STAR) inst.opcode = KnitOpcode::MUL_F;
			else if (op == ExprToken::SLASH) inst.opcode = KnitOpcode::DIV_F;
			else inst.opcode = KnitOpcode::MOD_F;
			inst.dst = dst;
			inst.src_a = left;
			inst.src_b = right;
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			left = dst;
		}
		return left;
	}

	uint8_t parse_power() {
		uint8_t left = parse_unary();
		if (current_tok.type == ExprToken::CARET) {
			advance();
			uint8_t right = parse_power();
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			inst.opcode = KnitOpcode::POW_F;
			inst.dst = dst;
			inst.src_a = left;
			inst.src_b = right;
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			left = dst;
		}
		return left;
	}

	uint8_t parse_unary() {
		if (current_tok.type == ExprToken::MINUS) {
			advance();
			uint8_t operand = parse_unary();
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			inst.opcode = KnitOpcode::NEG_F;
			inst.dst = dst;
			inst.src_a = operand;
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			return dst;
		}
		return parse_primary();
	}

	uint8_t parse_primary() {
		if (current_tok.type == ExprToken::NUMBER) {
			double val = current_tok.number_val;
			advance();
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			inst.opcode = KnitOpcode::LOAD_CONST;
			inst.dst = dst;
			inst.imm32 = compiler->get_or_add_constant(compiled, val);
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			return dst;
		}

		if (current_tok.type == ExprToken::LPAREN) {
			advance();
			uint8_t res = parse_expr();
			if (current_tok.type == ExprToken::RPAREN) {
				advance();
			}
			return res;
		}

		if (current_tok.type == ExprToken::IDENT) {
			String name = current_tok.ident_val;
			advance();

			// Check for function call: func(...)
			if (current_tok.type == ExprToken::LPAREN) {
				advance();
				Vector<uint8_t> args;
				if (current_tok.type != ExprToken::RPAREN) {
					args.push_back(parse_expr());
					while (current_tok.type == ExprToken::COMMA) {
						advance();
						args.push_back(parse_expr());
					}
				}
				if (current_tok.type == ExprToken::RPAREN) {
					advance();
				}

				uint8_t dst = compiler->allocate_register();
				KnitInstruction inst;
				inst.opcode = KnitOpcode::CALL_UTILITY;
				inst.dst = dst;
				inst.imm32 = compiler->get_or_add_method(compiled, StringName(name.to_lower()));
				inst.src_b = args.size() > 0 ? args[0] : 0;
				inst.extra = (uint8_t)args.size();
				inst.debug_node_id = debug_node_id;

				compiled.instructions.push_back(inst);
				return dst;
			}

			// Constant identifiers
			if (name.to_upper() == "PI") {
				uint8_t dst = compiler->allocate_register();
				KnitInstruction inst;
				inst.opcode = KnitOpcode::LOAD_CONST;
				inst.dst = dst;
				inst.imm32 = compiler->get_or_add_constant(compiled, 3.14159265358979323846);
				inst.debug_node_id = debug_node_id;
				compiled.instructions.push_back(inst);
				return dst;
			} else if (name.to_upper() == "TAU") {
				uint8_t dst = compiler->allocate_register();
				KnitInstruction inst;
				inst.opcode = KnitOpcode::LOAD_CONST;
				inst.dst = dst;
				inst.imm32 = compiler->get_or_add_constant(compiled, 6.28318530717958647692);
				inst.debug_node_id = debug_node_id;
				compiled.instructions.push_back(inst);
				return dst;
			}

			// Variable input pin lookup
			if (var_map.has(name)) {
				return var_map[name];
			}
			for (const KeyValue<String, uint8_t> &E : var_map) {
				if (E.key.to_lower() == name.to_lower()) {
					return E.value;
				}
			}

			// Default zero fallback
			uint8_t dst = compiler->allocate_register();
			KnitInstruction inst;
			inst.opcode = KnitOpcode::LOAD_CONST;
			inst.dst = dst;
			inst.imm32 = compiler->get_or_add_constant(compiled, 0.0);
			inst.debug_node_id = debug_node_id;
			compiled.instructions.push_back(inst);
			return dst;
		}

		return 0;
	}
};

} // anonymous namespace

uint8_t KnitsCompiler::allocate_register() {
	uint8_t reg = next_available_register++;
	if (next_available_register >= 64) {
		next_available_register = 63; // Clamp to max register limit
	}
	return reg;
}

int KnitsCompiler::get_or_add_constant(KnitCompiledGraph &r_compiled, const Variant &p_val) {
	if (constant_pool_map.has(p_val)) {
		return constant_pool_map[p_val];
	}
	int idx = r_compiled.constants.size();
	r_compiled.constants.push_back(p_val);
	constant_pool_map[p_val] = idx;
	return idx;
}

int KnitsCompiler::get_or_add_variable(KnitCompiledGraph &r_compiled, const StringName &p_name) {
	for (int i = 0; i < r_compiled.variable_names.size(); i++) {
		if (r_compiled.variable_names[i] == p_name) {
			return i;
		}
	}
	int idx = r_compiled.variable_names.size();
	r_compiled.variable_names.push_back(p_name);
	return idx;
}

int KnitsCompiler::get_or_add_method(KnitCompiledGraph &r_compiled, const StringName &p_name) {
	for (int i = 0; i < r_compiled.method_names.size(); i++) {
		if (r_compiled.method_names[i] == p_name) {
			return i;
		}
	}
	int idx = r_compiled.method_names.size();
	r_compiled.method_names.push_back(p_name);
	return idx;
}

bool KnitsCompiler::lower_pure_node(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_state, String &r_error) {
	if (p_node.is_null()) {
		return true;
	}

	// Cycle detection in pure expression trees
	if (p_state.in_stack.has(p_node->id)) {
		r_error = vformat("Cycle detected in pure expression subgraph containing node 0x%X (%s)", (uint64_t)p_node->id, p_node->title);
		return false;
	}

	if (p_state.visited.has(p_node->id)) {
		return true; // Already lowered, registers are available in pin_registers
	}

	p_state.in_stack.insert(p_node->id);

	// Recursively lower all input dependencies
	Vector<uint8_t> input_regs;
	for (int i = 0; i < p_node->input_pins.size(); i++) {
		const KnitPin &pin = p_node->input_pins[i];
		if (pin.kind == KnitPinKind::Data) {
			uint8_t r = 0;
			if (!resolve_data_input(p_graph, pin, r_compiled, p_state, r, r_error)) {
				return false;
			}
			input_regs.push_back(r);
		}
	}

	String title = p_node->title.to_lower();

	// 1. Reroute Knot (Pass-through register assignment)
	if (title == "reroute") {
		uint8_t pass_reg = input_regs.size() > 0 ? input_regs[0] : allocate_register();
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			p_state.pin_to_register[p_node->output_pins[i].id] = pass_reg;
			pin_registers[p_node->output_pins[i].id] = pass_reg;
		}
		p_state.visited.insert(p_node->id);
		p_state.in_stack.erase(p_node->id);
		return true;
	}

	// 2. Math Expression Node
	if (title == "expression" || title == "math_expression" || title == "math expression") {
		String expr_str = p_node->target_symbol.is_empty() ? "0" : String(p_node->target_symbol);
		HashMap<String, uint8_t> var_map;
		for (int i = 0; i < p_node->input_pins.size(); i++) {
			if (i < input_regs.size()) {
				var_map[p_node->input_pins[i].name] = input_regs[i];
				var_map[p_node->input_pins[i].display_label] = input_regs[i];
			}
		}

		ExprParser parser(expr_str, this, r_compiled, var_map, p_node->id);
		uint8_t res_reg = parser.parse_expr();
		for (int i = 0; i < p_node->output_pins.size(); i++) {
			p_state.pin_to_register[p_node->output_pins[i].id] = res_reg;
			pin_registers[p_node->output_pins[i].id] = res_reg;
		}
		p_state.visited.insert(p_node->id);
		p_state.in_stack.erase(p_node->id);
		return true;
	}

	// Allocate destination registers for multi-output or single-output pure nodes
	uint8_t base_out_reg = allocate_register();
	for (int i = 0; i < p_node->output_pins.size(); i++) {
		uint8_t out_reg = (i == 0) ? base_out_reg : allocate_register();
		p_state.pin_to_register[p_node->output_pins[i].id] = out_reg;
		pin_registers[p_node->output_pins[i].id] = out_reg;
	}

	// Emit pure instruction based on node symbol/title
	KnitInstruction inst;
	inst.debug_node_id = p_node->id;
	inst.dst = base_out_reg;

	if (title == "add" || title == "+" || title == "math::add") {
		inst.opcode = KnitOpcode::ADD_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "sub" || title == "-" || title == "math::sub") {
		inst.opcode = KnitOpcode::SUB_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "mul" || title == "*" || title == "math::mul") {
		inst.opcode = KnitOpcode::MUL_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "div" || title == "/" || title == "math::div") {
		inst.opcode = KnitOpcode::DIV_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "mod" || title == "%" || title == "math::mod" || title == "fmod") {
		inst.opcode = KnitOpcode::MOD_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "pow" || title == "**" || title == "math::pow") {
		inst.opcode = KnitOpcode::POW_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "neg" || title == "negate") {
		inst.opcode = KnitOpcode::NEG_F;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "bit_and" || title == "&" || title == "bitwise::and") {
		inst.opcode = KnitOpcode::BIT_AND;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "bit_or" || title == "|" || title == "bitwise::or") {
		inst.opcode = KnitOpcode::BIT_OR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "bit_xor" || title == "^" || title == "bitwise::xor") {
		inst.opcode = KnitOpcode::BIT_XOR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "bit_not" || title == "~" || title == "bitwise::not") {
		inst.opcode = KnitOpcode::BIT_NOT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "bit_shl" || title == "<<" || title == "bitwise::shl") {
		inst.opcode = KnitOpcode::BIT_SHL;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "bit_shr" || title == ">>" || title == "bitwise::shr") {
		inst.opcode = KnitOpcode::BIT_SHR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_eq" || title == "==" || title == "equal") {
		inst.opcode = KnitOpcode::CMP_EQ;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_ne" || title == "!=" || title == "not_equal") {
		inst.opcode = KnitOpcode::CMP_NE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_lt" || title == "<" || title == "less_than") {
		inst.opcode = KnitOpcode::CMP_LT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_le" || title == "<=" || title == "less_equal") {
		inst.opcode = KnitOpcode::CMP_LE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_gt" || title == ">" || title == "greater_than") {
		inst.opcode = KnitOpcode::CMP_GT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_ge" || title == ">=" || title == "greater_equal") {
		inst.opcode = KnitOpcode::CMP_GE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "not" || title == "!" || title == "logical_not") {
		inst.opcode = KnitOpcode::LOGICAL_NOT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "and" || title == "&&" || title == "logical_and") {
		inst.opcode = KnitOpcode::LOGICAL_AND;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "or" || title == "||" || title == "logical_or") {
		inst.opcode = KnitOpcode::LOGICAL_OR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec2_add") {
		inst.opcode = KnitOpcode::VEC2_ADD;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec2_sub") {
		inst.opcode = KnitOpcode::VEC2_SUB;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec2_scale") {
		inst.opcode = KnitOpcode::VEC2_SCALE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec2_dot") {
		inst.opcode = KnitOpcode::VEC2_DOT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec3_add") {
		inst.opcode = KnitOpcode::VEC3_ADD;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec3_sub") {
		inst.opcode = KnitOpcode::VEC3_SUB;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec3_scale") {
		inst.opcode = KnitOpcode::VEC3_SCALE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec3_dot") {
		inst.opcode = KnitOpcode::VEC3_DOT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "vec3_cross") {
		inst.opcode = KnitOpcode::VEC3_CROSS;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;

	// Make / Break Structs & Components
	} else if (title == "make_vec2" || title == "make_vector2" || title == "vec2") {
		inst.opcode = KnitOpcode::MAKE_VEC2;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_vec2" || title == "break_vector2") {
		inst.opcode = KnitOpcode::BREAK_VEC2;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_vec3" || title == "make_vector3" || title == "vec3") {
		inst.opcode = KnitOpcode::MAKE_VEC3;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
		inst.extra = input_regs.size() > 2 ? input_regs[2] : 0;
	} else if (title == "break_vec3" || title == "break_vector3") {
		inst.opcode = KnitOpcode::BREAK_VEC3;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_vec4" || title == "make_vector4" || title == "vec4") {
		inst.opcode = KnitOpcode::MAKE_VEC4;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_vec4" || title == "break_vector4") {
		inst.opcode = KnitOpcode::BREAK_VEC4;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_color" || title == "color") {
		inst.opcode = KnitOpcode::MAKE_COLOR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_color") {
		inst.opcode = KnitOpcode::BREAK_COLOR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_rect2" || title == "rect2") {
		inst.opcode = KnitOpcode::MAKE_RECT2;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_rect2") {
		inst.opcode = KnitOpcode::BREAK_RECT2;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_transform2d") {
		inst.opcode = KnitOpcode::MAKE_TRANSFORM2D;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_transform2d") {
		inst.opcode = KnitOpcode::BREAK_TRANSFORM2D;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "make_transform3d" || title == "transform3d") {
		inst.opcode = KnitOpcode::MAKE_TRANSFORM3D;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "break_transform3d") {
		inst.opcode = KnitOpcode::BREAK_TRANSFORM3D;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;

	// Logic & Pure Selection
	} else if (title == "select" || title == "ternary" || title == "if_pure") {
		inst.opcode = KnitOpcode::SELECT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
		inst.extra = input_regs.size() > 2 ? input_regs[2] : 0;

	// String Formatting & Concatenation
	} else if (title == "format_str" || title == "str_format" || title == "concat") {
		inst.opcode = KnitOpcode::FORMAT_STR;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.extra = (uint8_t)input_regs.size();

	// Property & Member Accessors
	} else if (title == "get_prop" || title == "get_property" || title == "prop_get" || title.begins_with("get_")) {
		inst.opcode = KnitOpcode::GET_PROP;
		StringName prop_name = p_node->target_symbol.is_empty() ? StringName(p_node->title) : p_node->target_symbol;
		inst.imm32 = get_or_add_variable(r_compiled, prop_name);
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;

	} else if (title == "array_construct" || title == "make_array" || title == "create_array") {
		inst.opcode = KnitOpcode::ARRAY_CONSTRUCT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.extra = (uint8_t)input_regs.size();
	} else if (title == "array_size" || title == "len") {
		inst.opcode = KnitOpcode::ARRAY_SIZE;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "dict_construct" || title == "make_dict" || title == "create_dictionary") {
		inst.opcode = KnitOpcode::DICT_CONSTRUCT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.extra = (uint8_t)(input_regs.size() / 2);
	} else if (title == "dict_get" || title == "get_key") {
		inst.opcode = KnitOpcode::DICT_GET;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
		inst.extra = input_regs.size() > 2 ? input_regs[2] : 0;
	} else if (title == "dict_has" || title == "has_key") {
		inst.opcode = KnitOpcode::DICT_HAS;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "get_indexed" || title == "index_get") {
		inst.opcode = KnitOpcode::GET_INDEXED;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "typeof" || title == "type_of") {
		inst.opcode = KnitOpcode::TYPE_OF;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "is_instance_valid") {
		inst.opcode = KnitOpcode::IS_INSTANCE_VALID;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
	} else if (title == "type_test" || title == "is" || title == "is_instance_of") {
		inst.opcode = KnitOpcode::TYPE_TEST;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.imm32 = get_or_add_constant(r_compiled, p_node->target_symbol.is_empty() ? Variant(StringName(p_node->title)) : Variant(p_node->target_symbol));
	} else if (title == "type_cast" || title == "as" || title == "convert") {
		inst.opcode = KnitOpcode::TYPE_CAST;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.imm32 = (int32_t)(p_node->output_pins.size() > 0 ? (int)p_node->output_pins[0].type.kind : 0);
	} else if (p_node->category == KnitNodeCategory::VariableGet) {
		inst.opcode = KnitOpcode::LOAD_VAR;
		inst.imm32 = get_or_add_variable(r_compiled, p_node->target_symbol);
	} else {
		StringName func_name = p_node->target_symbol.is_empty() ? StringName(p_node->title) : p_node->target_symbol;
		if (Variant::has_utility_function(func_name)) {
			inst.opcode = KnitOpcode::CALL_UTILITY;
			inst.imm32 = get_or_add_method(r_compiled, func_name);
			inst.src_b = input_regs.size() > 0 ? input_regs[0] : 0;
			inst.extra = (uint8_t)input_regs.size();
		} else {
			// Pure method call (e.g. Vector3::distance_to, Input::is_action_pressed)
			inst.opcode = KnitOpcode::CALL_METHOD_BIND;
			inst.imm32 = get_or_add_method(r_compiled, func_name);
			if (p_node->input_pins.size() > 0 && p_node->input_pins[0].type.kind == KnitDataType::ObjectRef && input_regs.size() > 0) {
				inst.src_a = input_regs[0]; // Target object
				inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
				inst.extra = (uint8_t)(input_regs.size() - 1);
			} else {
				inst.src_a = 0; // Default target (self)
				inst.src_b = input_regs.size() > 0 ? input_regs[0] : 0;
				inst.extra = (uint8_t)input_regs.size();
			}
		}
	}

	uint32_t pc = r_compiled.instructions.size();
	r_compiled.instructions.push_back(inst);
	r_compiled.debug_symbols[pc] = { p_node->id, p_node->output_pins.size() > 0 ? p_node->output_pins[0].id : 0 };

	p_state.in_stack.erase(p_node->id);
	p_state.visited.insert(p_node->id);

	return true;
}

bool KnitsCompiler::resolve_data_input(const KnitsGraph &p_graph, const KnitPin &p_input_pin, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_state, uint8_t &r_reg, String &r_error) {
	const KnitConnection *conn = p_graph.get_connection_for_input_pin(p_input_pin.id);
	if (conn) {
		// Connected to an output pin
		if (pin_registers.has(conn->from_pin)) {
			r_reg = pin_registers[conn->from_pin];
			return true;
		}

		Ref<KnitNode> source_node = p_graph.get_node(conn->from_node);
		if (source_node.is_valid() && (source_node->category == KnitNodeCategory::PureFunction || source_node->category == KnitNodeCategory::VariableGet)) {
			if (!lower_pure_node(p_graph, source_node, r_compiled, p_state, r_error)) {
				return false;
			}
			if (pin_registers.has(conn->from_pin)) {
				r_reg = pin_registers[conn->from_pin];
				return true;
			}
		}
	}

	// Not connected or literal default value
	r_reg = allocate_register();
	KnitInstruction inst;
	inst.opcode = KnitOpcode::LOAD_CONST;
	inst.dst = r_reg;
	inst.imm32 = get_or_add_constant(r_compiled, p_input_pin.default_value);
	inst.debug_node_id = p_input_pin.owner_node;

	uint32_t pc = r_compiled.instructions.size();
	r_compiled.instructions.push_back(inst);
	r_compiled.debug_symbols[pc] = { p_input_pin.owner_node, p_input_pin.id };

	return true;
}

bool KnitsCompiler::compile_exec_block(const KnitsGraph &p_graph, const Ref<KnitNode> &p_start_node, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_pure_state, HashSet<KnitNodeID> &r_visited_exec, String &r_error) {
	Ref<KnitNode> current = p_start_node;

	while (current.is_valid() && !r_visited_exec.has(current->id)) {
		r_visited_exec.insert(current->id);

		// Resolve all data inputs for this exec node
		Vector<uint8_t> data_in_regs;
		for (int i = 0; i < current->input_pins.size(); i++) {
			const KnitPin &pin = current->input_pins[i];
			if (pin.kind == KnitPinKind::Data) {
				uint8_t r = 0;
				if (!resolve_data_input(p_graph, pin, r_compiled, p_pure_state, r, r_error)) {
					return false;
				}
				data_in_regs.push_back(r);
			}
		}

		String title = current->title.to_lower();
		KnitInstruction inst;
		inst.debug_node_id = current->id;

		// 1. Branch (If / Else) Flow Control
		if (title == "branch" || title == "if" || title == "if_else") {
			uint8_t cond_reg = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.opcode = KnitOpcode::JUMP_IF_FALSE;
			inst.src_a = cond_reg;
			inst.imm32 = 0; // Placeholder for false branch PC

			uint32_t jump_false_inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(inst);
			r_compiled.debug_symbols[jump_false_inst_idx] = { current->id, 0 };

			// Compile True branch
			const KnitPin *true_pin = nullptr;
			const KnitPin *false_pin = nullptr;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].name == "True" || current->output_pins[i].display_label == "True") {
					true_pin = &current->output_pins[i];
				} else if (current->output_pins[i].name == "False" || current->output_pins[i].display_label == "False") {
					false_pin = &current->output_pins[i];
				}
			}

			if (true_pin) {
				Vector<const KnitConnection *> true_conns = p_graph.get_connections_for_output_pin(true_pin->id);
				if (true_conns.size() > 0) {
					Ref<KnitNode> true_node = p_graph.get_node(true_conns[0]->to_node);
					if (true_node.is_valid()) {
						if (!compile_exec_block(p_graph, true_node, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}

			// Emit Jump over false branch
			KnitInstruction jump_exit;
			jump_exit.opcode = KnitOpcode::JUMP;
			jump_exit.imm32 = 0; // Placeholder for exit PC
			jump_exit.debug_node_id = current->id;
			uint32_t jump_exit_inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(jump_exit);
			r_compiled.debug_symbols[jump_exit_inst_idx] = { current->id, 0 };

			// Patch False branch start target
			r_compiled.instructions.write[jump_false_inst_idx].imm32 = r_compiled.instructions.size();

			// Compile False branch
			if (false_pin) {
				Vector<const KnitConnection *> false_conns = p_graph.get_connections_for_output_pin(false_pin->id);
				if (false_conns.size() > 0) {
					Ref<KnitNode> false_node = p_graph.get_node(false_conns[0]->to_node);
					if (false_node.is_valid()) {
						if (!compile_exec_block(p_graph, false_node, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}

			// Patch Jump exit target
			r_compiled.instructions.write[jump_exit_inst_idx].imm32 = r_compiled.instructions.size();
			return true;
		}

		// 2. Sequence (Then 0, Then 1, Then 2, ...) Flow Control
		if (title == "sequence") {
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Execution) {
					Vector<const KnitConnection *> seq_conns = p_graph.get_connections_for_output_pin(current->output_pins[i].id);
					if (seq_conns.size() > 0) {
						Ref<KnitNode> seq_node = p_graph.get_node(seq_conns[0]->to_node);
						if (seq_node.is_valid()) {
							if (!compile_exec_block(p_graph, seq_node, r_compiled, p_pure_state, r_visited_exec, r_error)) {
								return false;
							}
						}
					}
				}
			}
			return true;
		}

		// 3. While Loop Flow Control
		if (title == "while" || title == "while_loop") {
			uint32_t loop_start_pc = r_compiled.instructions.size();
			uint8_t cond_reg = data_in_regs.size() > 0 ? data_in_regs[0] : 0;

			inst.opcode = KnitOpcode::JUMP_IF_FALSE;
			inst.src_a = cond_reg;
			inst.imm32 = 0; // Placeholder for loop exit PC
			uint32_t jump_exit_inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(inst);
			r_compiled.debug_symbols[jump_exit_inst_idx] = { current->id, 0 };

			const KnitPin *body_pin = nullptr;
			const KnitPin *completed_pin = nullptr;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].name == "LoopBody" || current->output_pins[i].display_label == "LoopBody") {
					body_pin = &current->output_pins[i];
				} else if (current->output_pins[i].name == "Completed" || current->output_pins[i].display_label == "Completed") {
					completed_pin = &current->output_pins[i];
				}
			}

			if (body_pin) {
				Vector<const KnitConnection *> body_conns = p_graph.get_connections_for_output_pin(body_pin->id);
				if (body_conns.size() > 0) {
					Ref<KnitNode> body_node = p_graph.get_node(body_conns[0]->to_node);
					if (body_node.is_valid()) {
						if (!compile_exec_block(p_graph, body_node, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}

			// Jump back to start of loop
			KnitInstruction jump_start;
			jump_start.opcode = KnitOpcode::JUMP;
			jump_start.imm32 = loop_start_pc;
			jump_start.debug_node_id = current->id;
			r_compiled.instructions.push_back(jump_start);

			// Patch loop exit
			r_compiled.instructions.write[jump_exit_inst_idx].imm32 = r_compiled.instructions.size();

			if (completed_pin) {
				Vector<const KnitConnection *> comp_conns = p_graph.get_connections_for_output_pin(completed_pin->id);
				if (comp_conns.size() > 0) {
					Ref<KnitNode> comp_node = p_graph.get_node(comp_conns[0]->to_node);
					if (comp_node.is_valid()) {
						if (!compile_exec_block(p_graph, comp_node, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}
			return true;
		}

		// 4. Do Once Flow Gate
		if (title == "do once" || title == "do_once") {
			inst.opcode = KnitOpcode::DO_ONCE;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			uint32_t inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(inst);
			r_compiled.debug_symbols[inst_idx] = { current->id, 0 };

			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Execution) {
					Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(current->output_pins[i].id);
					if (conns.size() > 0) {
						Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
						if (next.is_valid()) {
							if (!compile_exec_block(p_graph, next, r_compiled, p_pure_state, r_visited_exec, r_error)) {
								return false;
							}
						}
					}
					break;
				}
			}
			r_compiled.instructions.write[inst_idx].imm32 = r_compiled.instructions.size();
			return true;
		}

		// 5. Flip Flop Alternating Gate
		if (title == "flip flop" || title == "flipflop" || title == "flip_flop") {
			inst.opcode = KnitOpcode::FLIP_FLOP;
			uint8_t is_a_reg = allocate_register();
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Data) {
					p_pure_state.pin_to_register[current->output_pins[i].id] = is_a_reg;
					pin_registers[current->output_pins[i].id] = is_a_reg;
				}
			}
			inst.dst = is_a_reg;
			uint32_t inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(inst);
			r_compiled.debug_symbols[inst_idx] = { current->id, 0 };

			const KnitPin *pin_a = nullptr;
			const KnitPin *pin_b = nullptr;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].name == "A" || current->output_pins[i].display_label == "A") pin_a = &current->output_pins[i];
				else if (current->output_pins[i].name == "B" || current->output_pins[i].display_label == "B") pin_b = &current->output_pins[i];
			}

			if (pin_a) {
				Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(pin_a->id);
				if (conns.size() > 0) {
					Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
					if (next.is_valid()) {
						if (!compile_exec_block(p_graph, next, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}

			KnitInstruction jump_end;
			jump_end.opcode = KnitOpcode::JUMP;
			jump_end.debug_node_id = current->id;
			uint32_t jump_end_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(jump_end);

			r_compiled.instructions.write[inst_idx].imm32 = r_compiled.instructions.size();
			if (pin_b) {
				Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(pin_b->id);
				if (conns.size() > 0) {
					Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
					if (next.is_valid()) {
						if (!compile_exec_block(p_graph, next, r_compiled, p_pure_state, r_visited_exec, r_error)) {
							return false;
						}
					}
				}
			}
			r_compiled.instructions.write[jump_end_idx].imm32 = r_compiled.instructions.size();
			return true;
		}

		// 6. Gate Flow Control
		if (title == "gate") {
			inst.opcode = KnitOpcode::GATE;
			inst.src_a = 0; // Enter
			uint32_t inst_idx = r_compiled.instructions.size();
			r_compiled.instructions.push_back(inst);
			r_compiled.debug_symbols[inst_idx] = { current->id, 0 };

			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Execution) {
					Vector<const KnitConnection *> conns = p_graph.get_connections_for_output_pin(current->output_pins[i].id);
					if (conns.size() > 0) {
						Ref<KnitNode> next = p_graph.get_node(conns[0]->to_node);
						if (next.is_valid()) {
							if (!compile_exec_block(p_graph, next, r_compiled, p_pure_state, r_visited_exec, r_error)) {
								return false;
							}
						}
					}
					break;
				}
			}
			r_compiled.instructions.write[inst_idx].imm32 = r_compiled.instructions.size();
			return true;
		}

		// 7. Gameplay & Physics Operations
		if (title == "character move & jump 3d" || title == "char_move_jump_3d" || title == "move_and_jump_3d" || title == "char_move_jump") {
			inst.opcode = KnitOpcode::CHAR_MOVE_JUMP_3D;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			uint8_t ground_reg = allocate_register();
			uint8_t vel_reg = allocate_register();
			inst.dst = ground_reg;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Data) {
					String pname = String(current->output_pins[i].name);
					if (pname.findn("ground") != -1 || pname.findn("floor") != -1) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = ground_reg;
						pin_registers[current->output_pins[i].id] = ground_reg;
					} else if (pname.findn("vel") != -1) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = vel_reg;
						pin_registers[current->output_pins[i].id] = vel_reg;
					}
				}
			}
		} else if (title == "raycast query 3d" || title == "raycast_3d" || title == "raycast") {
			inst.opcode = KnitOpcode::RAYCAST_3D;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			inst.extra = data_in_regs.size() > 2 ? data_in_regs[2] : 0xFF;
			uint8_t hit_reg = allocate_register();
			uint8_t pos_reg = allocate_register();
			uint8_t norm_reg = allocate_register();
			uint8_t col_reg = allocate_register();
			inst.dst = hit_reg;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Data) {
					String pname = String(current->output_pins[i].name);
					if (pname.findn("hit") != -1 && current->output_pins[i].type.kind == KnitDataType::Bool) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = hit_reg;
						pin_registers[current->output_pins[i].id] = hit_reg;
					} else if (pname.findn("pos") != -1) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = pos_reg;
						pin_registers[current->output_pins[i].id] = pos_reg;
					} else if (pname.findn("norm") != -1) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = norm_reg;
						pin_registers[current->output_pins[i].id] = norm_reg;
					} else if (pname.findn("col") != -1) {
						p_pure_state.pin_to_register[current->output_pins[i].id] = col_reg;
						pin_registers[current->output_pins[i].id] = col_reg;
					}
				}
			}
		} else if (title == "tween property" || title == "tween_property" || title == "tween_prop") {
			inst.opcode = KnitOpcode::TWEEN_PROP;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			uint8_t tw_reg = allocate_register();
			inst.dst = tw_reg;
			for (int i = 0; i < current->output_pins.size(); i++) {
				if (current->output_pins[i].kind == KnitPinKind::Data) {
					p_pure_state.pin_to_register[current->output_pins[i].id] = tw_reg;
					pin_registers[current->output_pins[i].id] = tw_reg;
				}
			}

		// 8. Standard Action Nodes
		} else if (current->category == KnitNodeCategory::VariableSet) {
			inst.opcode = KnitOpcode::STORE_VAR;
			inst.imm32 = get_or_add_variable(r_compiled, current->target_symbol);
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "set_prop" || title == "set_property" || title == "prop_set") {
			inst.opcode = KnitOpcode::SET_PROP;
			inst.imm32 = get_or_add_variable(r_compiled, current->target_symbol.is_empty() ? StringName(current->title) : current->target_symbol);
			if (data_in_regs.size() >= 2) {
				inst.src_a = data_in_regs[0]; // Target object
				inst.src_b = data_in_regs[1]; // Value
			} else {
				inst.src_a = 0; // Self
				inst.src_b = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			}
		} else if (title == "yield_seconds" || title == "delay" || title == "wait_seconds") {
			inst.opcode = KnitOpcode::YIELD_SECONDS;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "yield_frames" || title == "wait_frames") {
			inst.opcode = KnitOpcode::YIELD_FRAMES;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "return") {
			inst.opcode = KnitOpcode::RETURN;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "print" || title == "print_rich" || title == "printerr" || title == "push_error" || title == "push_warning") {
			inst.opcode = KnitOpcode::CALL_UTILITY;
			inst.imm32 = get_or_add_method(r_compiled, StringName(title));
			inst.src_b = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.extra = (uint8_t)data_in_regs.size();
		} else if (title == "assert") {
			inst.opcode = KnitOpcode::DEBUG_TRAP;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "emit_signal" || title == "signal_emit") {
			inst.opcode = KnitOpcode::SIGNAL_EMIT;
			inst.imm32 = get_or_add_method(r_compiled, current->target_symbol.is_empty() ? StringName(current->title) : current->target_symbol);
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			inst.extra = (uint8_t)(data_in_regs.size() > 1 ? data_in_regs.size() - 1 : 0);
		} else if (title == "connect_signal" || title == "signal_connect") {
			inst.opcode = KnitOpcode::SIGNAL_CONNECT;
			inst.imm32 = get_or_add_method(r_compiled, current->target_symbol.is_empty() ? StringName(current->title) : current->target_symbol);
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
		} else if (title == "array_append") {
			inst.opcode = KnitOpcode::ARRAY_APPEND;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
		} else if (title == "dict_set") {
			inst.opcode = KnitOpcode::DICT_SET;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			inst.extra = data_in_regs.size() > 2 ? data_in_regs[2] : 0;
		} else if (title == "set_indexed") {
			inst.opcode = KnitOpcode::SET_INDEXED;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
			inst.extra = data_in_regs.size() > 2 ? data_in_regs[2] : 0;
		} else if (current->category == KnitNodeCategory::ImpureAction) {
			StringName action_name = current->target_symbol.is_empty() ? StringName(current->title) : current->target_symbol;
			if (Variant::has_utility_function(action_name)) {
				inst.opcode = KnitOpcode::CALL_UTILITY;
				inst.imm32 = get_or_add_method(r_compiled, action_name);
				inst.dst = allocate_register();
				inst.src_b = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
				inst.extra = (uint8_t)data_in_regs.size();
			} else {
				inst.opcode = KnitOpcode::CALL_METHOD_BIND;
				inst.imm32 = get_or_add_method(r_compiled, action_name);
				inst.dst = allocate_register();
				if (current->input_pins.size() > 0 && current->input_pins[0].type.kind == KnitDataType::ObjectRef && data_in_regs.size() > 0) {
					inst.src_a = data_in_regs[0]; // Target Object
					inst.src_b = data_in_regs.size() > 1 ? data_in_regs[1] : 0;
					inst.extra = (uint8_t)(data_in_regs.size() > 1 ? data_in_regs.size() - 1 : 0);
				} else {
					inst.src_a = 0; // Target instance (Self)
					inst.src_b = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
					inst.extra = (uint8_t)data_in_regs.size();
				}
			}
		} else {
			inst.opcode = KnitOpcode::NOP;
		}

		uint32_t pc = r_compiled.instructions.size();
		r_compiled.instructions.push_back(inst);
		r_compiled.debug_symbols[pc] = { current->id, 0 };

		// Follow the first output execution pin to next node
		KnitPin *exec_out = nullptr;
		for (int i = 0; i < current->output_pins.size(); i++) {
			if (current->output_pins[i].kind == KnitPinKind::Execution) {
				exec_out = &current->output_pins.write[i];
				break;
			}
		}

		if (exec_out) {
			Vector<const KnitConnection *> out_conns = p_graph.get_connections_for_output_pin(exec_out->id);
			if (out_conns.size() > 0) {
				current = p_graph.get_node(out_conns[0]->to_node);
			} else {
				current = Ref<KnitNode>();
			}
		} else {
			current = Ref<KnitNode>();
		}
	}

	return true;
}

bool KnitsCompiler::compile(const Ref<KnitsGraph> &p_graph, KnitCompiledGraph &r_compiled, String &r_error) {
	if (p_graph.is_null()) {
		r_error = "Invalid graph resource (null)";
		return false;
	}

	r_compiled.graph_id = p_graph->id;
	r_compiled.graph_name = StringName(p_graph->graph_name);
	r_compiled.is_function = p_graph->is_function;
	r_compiled.instructions.clear();
	r_compiled.constants.clear();
	r_compiled.variable_names.clear();
	r_compiled.method_names.clear();
	r_compiled.debug_symbols.clear();

	constant_pool_map.clear();
	pin_registers.clear();
	next_available_register = 0;

	// 1. Check for unconstrained wildcard generic pins (Tier 1 Rule 3)
	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
		const Ref<KnitNode> &node = E.value;
		if (node.is_null() || node->title.to_lower() == "reroute") continue;

		for (int i = 0; i < node->input_pins.size(); i++) {
			const KnitPin &pin = node->input_pins[i];
			if (pin.type.kind == KnitDataType::Wildcard && !pin.type.generic_symbol.is_empty() && !node->generic_bindings.has(pin.type.generic_symbol)) {
				r_error = vformat("Unresolved generic type '%s' on Node 0x%X (%s)", String(pin.type.generic_symbol), (uint64_t)node->id, node->title);
				return false;
			}
		}
		for (int i = 0; i < node->output_pins.size(); i++) {
			const KnitPin &pin = node->output_pins[i];
			if (pin.type.kind == KnitDataType::Wildcard && !pin.type.generic_symbol.is_empty() && !node->generic_bindings.has(pin.type.generic_symbol)) {
				r_error = vformat("Unresolved generic type '%s' on Node 0x%X (%s)", String(pin.type.generic_symbol), (uint64_t)node->id, node->title);
				return false;
			}
		}
	}

	// 2. Locate entry points (Event nodes or first execution node)
	Ref<KnitNode> entry_node;
	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
		if (E.value.is_valid() && E.value->category == KnitNodeCategory::Event) {
			entry_node = E.value;
			break;
		}
	}

	if (entry_node.is_null()) {
		for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : p_graph->nodes) {
			if (E.value.is_valid() && (E.value->category == KnitNodeCategory::ImpureAction || E.value->category == KnitNodeCategory::FlowControl)) {
				entry_node = E.value;
				break;
			}
		}
	}

	PureNodeEvaluationState pure_state;
	HashSet<KnitNodeID> visited_exec;

	if (entry_node.is_valid()) {
		if (!compile_exec_block(*p_graph.ptr(), entry_node, r_compiled, pure_state, visited_exec, r_error)) {
			return false;
		}
	}

	// Emit trailing RETURN if not already present
	if (r_compiled.instructions.is_empty() || r_compiled.instructions[r_compiled.instructions.size() - 1].opcode != KnitOpcode::RETURN) {
		KnitInstruction ret;
		ret.opcode = KnitOpcode::RETURN;
		ret.src_a = 0;
		r_compiled.instructions.push_back(ret);
	}

	r_compiled.register_count = next_available_register;
	return true;
}

KnitsCompiler::KnitsCompiler() {
}

KnitsCompiler::~KnitsCompiler() {
}
