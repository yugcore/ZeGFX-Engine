/**************************************************************************/
/*  knits_compiler.cpp                                                    */
/**************************************************************************/

#include "knits_compiler.h"

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

	// First lower all incoming data inputs for this pure node
	Vector<uint8_t> input_regs;
	for (int i = 0; i < p_node->input_pins.size(); i++) {
		const KnitPin &pin = p_node->input_pins[i];
		if (pin.kind != KnitPinKind::Data) {
			continue;
		}

		uint8_t in_reg = 0;
		if (!resolve_data_input(p_graph, pin, r_compiled, p_state, in_reg, r_error)) {
			return false;
		}
		input_regs.push_back(in_reg);
	}

	// Allocate register for output pin
	uint8_t out_reg = allocate_register();
	if (p_node->output_pins.size() > 0) {
		p_state.pin_to_register[p_node->output_pins[0].id] = out_reg;
		pin_registers[p_node->output_pins[0].id] = out_reg;
	}

	// Emit pure instruction based on node symbol/title
	String title = p_node->title.to_lower();
	KnitInstruction inst;
	inst.debug_node_id = p_node->id;
	inst.dst = out_reg;

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
	} else if (title == "cmp_eq" || title == "==") {
		inst.opcode = KnitOpcode::CMP_EQ;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_lt" || title == "<") {
		inst.opcode = KnitOpcode::CMP_LT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (title == "cmp_gt" || title == ">") {
		inst.opcode = KnitOpcode::CMP_GT;
		inst.src_a = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.src_b = input_regs.size() > 1 ? input_regs[1] : 0;
	} else if (p_node->category == KnitNodeCategory::VariableGet) {
		inst.opcode = KnitOpcode::LOAD_VAR;
		inst.imm32 = get_or_add_variable(r_compiled, p_node->target_symbol);
	} else {
		// Pure method call (e.g. Vector3::distance_to, Input::is_action_pressed)
		inst.opcode = KnitOpcode::CALL_METHOD_BIND;
		inst.imm32 = get_or_add_method(r_compiled, p_node->target_symbol.is_empty() ? StringName(p_node->title) : p_node->target_symbol);
		inst.src_a = 0; // Default target
		inst.src_b = input_regs.size() > 0 ? input_regs[0] : 0;
		inst.extra = (uint8_t)input_regs.size();
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
		if (node.is_null()) continue;

		for (int i = 0; i < node->input_pins.size(); i++) {
			const KnitPin &pin = node->input_pins[i];
			if (pin.type.kind == KnitDataType::Wildcard && !node->generic_bindings.has(pin.type.generic_symbol)) {
				r_error = vformat("Unresolved generic type '%s' on Node 0x%X (%s)", String(pin.type.generic_symbol), (uint64_t)node->id, node->title);
				return false;
			}
		}
		for (int i = 0; i < node->output_pins.size(); i++) {
			const KnitPin &pin = node->output_pins[i];
			if (pin.type.kind == KnitDataType::Wildcard && !node->generic_bindings.has(pin.type.generic_symbol)) {
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

	// 3. Walk execution paths starting from entry node
	Ref<KnitNode> current = entry_node;
	HashSet<KnitNodeID> visited_exec;

	while (current.is_valid() && !visited_exec.has(current->id)) {
		visited_exec.insert(current->id);

		// Resolve all data inputs for this exec node
		Vector<uint8_t> data_in_regs;
		for (int i = 0; i < current->input_pins.size(); i++) {
			const KnitPin &pin = current->input_pins[i];
			if (pin.kind == KnitPinKind::Data) {
				uint8_t r = 0;
				if (!resolve_data_input(*p_graph.ptr(), pin, r_compiled, pure_state, r, r_error)) {
					return false;
				}
				data_in_regs.push_back(r);
			}
		}

		// Emit instruction for this Exec node
		String title = current->title.to_lower();
		KnitInstruction inst;
		inst.debug_node_id = current->id;

		if (current->category == KnitNodeCategory::VariableSet) {
			inst.opcode = KnitOpcode::STORE_VAR;
			inst.imm32 = get_or_add_variable(r_compiled, current->target_symbol);
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "yield_seconds" || title == "delay" || title == "wait_seconds") {
			inst.opcode = KnitOpcode::YIELD_SECONDS;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "yield_frames" || title == "wait_frames") {
			inst.opcode = KnitOpcode::YIELD_FRAMES;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (title == "return") {
			inst.opcode = KnitOpcode::RETURN;
			inst.src_a = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
		} else if (current->category == KnitNodeCategory::ImpureAction) {
			inst.opcode = KnitOpcode::CALL_METHOD_BIND;
			inst.imm32 = get_or_add_method(r_compiled, current->target_symbol.is_empty() ? StringName(current->title) : current->target_symbol);
			inst.dst = allocate_register();
			inst.src_a = 0; // Target instance
			inst.src_b = data_in_regs.size() > 0 ? data_in_regs[0] : 0;
			inst.extra = (uint8_t)data_in_regs.size();
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
			Vector<const KnitConnection *> out_conns = p_graph->get_connections_for_output_pin(exec_out->id);
			if (out_conns.size() > 0) {
				current = p_graph->get_node(out_conns[0]->to_node);
			} else {
				current = Ref<KnitNode>();
			}
		} else {
			current = Ref<KnitNode>();
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
