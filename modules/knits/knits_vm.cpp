/**************************************************************************/
/*  knits_vm.cpp                                                          */
/**************************************************************************/

#include "knits_vm.h"
#include "core/object/class_db.h"
#include "core/object/message_queue.h"

bool KnitsBytecodeVM::evaluate_fault(const KnitInstruction &p_inst, uint32_t p_pc, const String &p_message) {
	fault_occurred = true;
	last_fault.node_id = p_inst.debug_node_id;
	last_fault.pc = p_pc;
	last_fault.message = p_message;
	ERR_PRINT(vformat("[Knits VM Fault] Node 0x%X at PC %d: %s", (uint64_t)p_inst.debug_node_id, p_pc, p_message));
	return false;
}

KnitVMStatus KnitsBytecodeVM::execute(const KnitCompiledGraph &p_graph, Object *p_instance, const Variant **p_args, int p_arg_count, Variant &r_return_val, uint32_t p_coroutine_id) {
	Variant registers[MAX_REGISTERS];

	// Load arguments into initial registers
	for (int i = 0; i < p_arg_count && i < MAX_REGISTERS; i++) {
		if (p_args && p_args[i]) {
			registers[i] = *p_args[i];
		}
	}

	uint32_t pc = 0;
	const int instruction_count = p_graph.instructions.size();

	while (pc < (uint32_t)instruction_count) {
		const KnitInstruction &inst = p_graph.instructions[pc];

		switch (inst.opcode) {
			case KnitOpcode::NOP:
				break;

			case KnitOpcode::LOAD_CONST: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.constants.size()) {
					registers[inst.dst] = p_graph.constants[inst.imm32];
				}
			} break;

			case KnitOpcode::LOAD_VAR: {
				if (p_instance && inst.imm32 >= 0 && inst.imm32 < p_graph.variable_names.size()) {
					registers[inst.dst] = p_instance->get(p_graph.variable_names[inst.imm32]);
				}
			} break;

			case KnitOpcode::STORE_VAR: {
				if (p_instance && inst.imm32 >= 0 && inst.imm32 < p_graph.variable_names.size()) {
					p_instance->set(p_graph.variable_names[inst.imm32], registers[inst.src_a]);
				}
			} break;

			case KnitOpcode::MOVE: {
				registers[inst.dst] = registers[inst.src_a];
			} break;

			case KnitOpcode::ADD_F: {
				registers[inst.dst] = double(registers[inst.src_a]) + double(registers[inst.src_b]);
			} break;

			case KnitOpcode::SUB_F: {
				registers[inst.dst] = double(registers[inst.src_a]) - double(registers[inst.src_b]);
			} break;

			case KnitOpcode::MUL_F: {
				registers[inst.dst] = double(registers[inst.src_a]) * double(registers[inst.src_b]);
			} break;

			case KnitOpcode::DIV_F: {
				double b = double(registers[inst.src_b]);
				if (unlikely(b == 0.0)) {
					evaluate_fault(inst, pc, "Division by zero (float)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = double(registers[inst.src_a]) / b;
			} break;

			case KnitOpcode::MOD_F: {
				double b = double(registers[inst.src_b]);
				if (unlikely(b == 0.0)) {
					evaluate_fault(inst, pc, "Modulo by zero (float)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = Math::fmod(double(registers[inst.src_a]), b);
			} break;

			case KnitOpcode::POW_F: {
				registers[inst.dst] = Math::pow(double(registers[inst.src_a]), double(registers[inst.src_b]));
			} break;

			case KnitOpcode::NEG_F: {
				registers[inst.dst] = -double(registers[inst.src_a]);
			} break;

			case KnitOpcode::ADD_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) + int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::SUB_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) - int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::MUL_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) * int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::DIV_I: {
				int64_t b = int64_t(registers[inst.src_b]);
				if (unlikely(b == 0)) {
					evaluate_fault(inst, pc, "Division by zero (int)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = int64_t(registers[inst.src_a]) / b;
			} break;

			case KnitOpcode::MOD_I: {
				int64_t b = int64_t(registers[inst.src_b]);
				if (unlikely(b == 0)) {
					evaluate_fault(inst, pc, "Modulo by zero (int)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = int64_t(registers[inst.src_a]) % b;
			} break;

			case KnitOpcode::NEG_I: {
				registers[inst.dst] = -int64_t(registers[inst.src_a]);
			} break;

			case KnitOpcode::BIT_AND: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) & int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_OR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) | int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_XOR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) ^ int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_NOT: {
				registers[inst.dst] = ~int64_t(registers[inst.src_a]);
			} break;

			case KnitOpcode::BIT_SHL: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) << int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_SHR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) >> int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_EQ: {
				registers[inst.dst] = (registers[inst.src_a] == registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_NE: {
				registers[inst.dst] = (registers[inst.src_a] != registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_LT: {
				registers[inst.dst] = (registers[inst.src_a] < registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_LE: {
				registers[inst.dst] = (registers[inst.src_a] <= registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_GT: {
				registers[inst.dst] = (registers[inst.src_a] > registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_GE: {
				registers[inst.dst] = (registers[inst.src_a] >= registers[inst.src_b]);
			} break;

			case KnitOpcode::LOGICAL_NOT: {
				registers[inst.dst] = !bool(registers[inst.src_a]);
			} break;

			case KnitOpcode::LOGICAL_AND: {
				registers[inst.dst] = bool(registers[inst.src_a]) && bool(registers[inst.src_b]);
			} break;

			case KnitOpcode::LOGICAL_OR: {
				registers[inst.dst] = bool(registers[inst.src_a]) || bool(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_ADD: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) + Vector2(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_SUB: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) - Vector2(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_SCALE: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) * float(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_DOT: {
				registers[inst.dst] = Vector2(registers[inst.src_a]).dot(Vector2(registers[inst.src_b]));
			} break;

			case KnitOpcode::VEC3_ADD: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) + Vector3(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_SUB: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) - Vector3(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_SCALE: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) * float(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_DOT: {
				registers[inst.dst] = Vector3(registers[inst.src_a]).dot(Vector3(registers[inst.src_b]));
			} break;

			case KnitOpcode::VEC3_CROSS: {
				registers[inst.dst] = Vector3(registers[inst.src_a]).cross(Vector3(registers[inst.src_b]));
			} break;

			case KnitOpcode::GET_INDEXED: {
				bool valid = false;
				registers[inst.dst] = registers[inst.src_a].get_named(registers[inst.src_b], valid);
				if (!valid) {
					registers[inst.dst] = registers[inst.src_a].get(registers[inst.src_b], &valid);
				}
			} break;

			case KnitOpcode::SET_INDEXED: {
				bool valid = false;
				registers[inst.src_a].set_named(registers[inst.src_b], registers[inst.extra], valid);
				if (!valid) {
					registers[inst.src_a].set(registers[inst.src_b], registers[inst.extra], &valid);
				}
			} break;

			case KnitOpcode::ARRAY_CONSTRUCT: {
				Array arr;
				int count = inst.extra;
				arr.resize(count);
				for (int i = 0; i < count; i++) {
					arr[i] = registers[inst.src_a + i];
				}
				registers[inst.dst] = arr;
			} break;

			case KnitOpcode::ARRAY_APPEND: {
				if (registers[inst.src_a].get_type() == Variant::ARRAY) {
					Array arr = registers[inst.src_a];
					arr.push_back(registers[inst.src_b]);
					registers[inst.src_a] = arr;
				}
			} break;

			case KnitOpcode::ARRAY_SIZE: {
				if (registers[inst.src_a].get_type() == Variant::ARRAY) {
					Array arr = registers[inst.src_a];
					registers[inst.dst] = (int64_t)arr.size();
				} else {
					registers[inst.dst] = (int64_t)0;
				}
			} break;

			case KnitOpcode::DICT_CONSTRUCT: {
				Dictionary d;
				int pairs = inst.extra;
				for (int i = 0; i < pairs; i++) {
					d[registers[inst.src_a + i * 2]] = registers[inst.src_a + i * 2 + 1];
				}
				registers[inst.dst] = d;
			} break;

			case KnitOpcode::DICT_GET: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					registers[inst.dst] = d.get(registers[inst.src_b], registers[inst.extra]);
				} else {
					registers[inst.dst] = registers[inst.extra];
				}
			} break;

			case KnitOpcode::DICT_SET: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					d[registers[inst.src_b]] = registers[inst.extra];
					registers[inst.src_a] = d;
				}
			} break;

			case KnitOpcode::DICT_HAS: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					registers[inst.dst] = d.has(registers[inst.src_b]);
				} else {
					registers[inst.dst] = false;
				}
			} break;

			case KnitOpcode::TYPE_TEST: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.constants.size()) {
					Variant target = p_graph.constants[inst.imm32];
					if (target.get_type() == Variant::INT) {
						registers[inst.dst] = (registers[inst.src_a].get_type() == (Variant::Type)int(target));
					} else if (target.get_type() == Variant::STRING_NAME || target.get_type() == Variant::STRING) {
						StringName class_name = target;
						Object *obj = registers[inst.src_a].get_validated_object();
						registers[inst.dst] = obj ? obj->is_class(class_name) : false;
					} else {
						registers[inst.dst] = false;
					}
				} else {
					registers[inst.dst] = false;
				}
			} break;

			case KnitOpcode::TYPE_CAST: {
				Variant::Type target_type = (Variant::Type)inst.imm32;
				Callable::CallError ce;
				const Variant *src = &registers[inst.src_a];
				Variant::construct(target_type, registers[inst.dst], &src, 1, ce);
			} break;

			case KnitOpcode::TYPE_OF: {
				registers[inst.dst] = (int64_t)registers[inst.src_a].get_type();
			} break;

			case KnitOpcode::IS_INSTANCE_VALID: {
				Object *obj = registers[inst.src_a].get_validated_object();
				registers[inst.dst] = (obj != nullptr);
			} break;

			case KnitOpcode::JUMP: {
				pc = (uint32_t)inst.imm32;
				continue;
			}

			case KnitOpcode::JUMP_IF_TRUE: {
				if (bool(registers[inst.src_a])) {
					pc = (uint32_t)inst.imm32;
					continue;
				}
			} break;

			case KnitOpcode::JUMP_IF_FALSE: {
				if (!bool(registers[inst.src_a])) {
					pc = (uint32_t)inst.imm32;
					continue;
				}
			} break;

			case KnitOpcode::CALL_METHOD_BIND: {
				Object *target_obj = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target_obj = registers[inst.src_a].get_validated_object();
					if (unlikely(!target_obj)) {
						evaluate_fault(inst, pc, "Target object is null on method call");
						return KnitVMStatus::Fault;
					}
				} else {
					target_obj = p_instance;
				}

				if (unlikely(!target_obj)) {
					evaluate_fault(inst, pc, "No valid instance object for method call");
					return KnitVMStatus::Fault;
				}

				if (inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName method_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}

					Callable::CallError ce;
					registers[inst.dst] = target_obj->callp(method_name, call_args, arg_count, ce);
					if (unlikely(ce.error != Callable::CallError::CALL_OK)) {
						evaluate_fault(inst, pc, vformat("Error calling method '%s'", String(method_name)));
						return KnitVMStatus::Fault;
					}
				}
			} break;

			case KnitOpcode::CALL_UTILITY: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName func_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					Callable::CallError ce;
					Variant::call_utility_function(func_name, &registers[inst.dst], call_args, arg_count, ce);
					if (unlikely(ce.error != Callable::CallError::CALL_OK)) {
						evaluate_fault(inst, pc, vformat("Error calling utility function '%s'", String(func_name)));
						return KnitVMStatus::Fault;
					}
				}
			} break;

			case KnitOpcode::SIGNAL_EMIT: {
				Object *target = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target = registers[inst.src_a].get_validated_object();
				} else {
					target = p_instance;
				}
				if (target && inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName sig_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					target->emit_signalp(sig_name, call_args, arg_count);
				}
			} break;

			case KnitOpcode::SIGNAL_CONNECT: {
				Object *target = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target = registers[inst.src_a].get_validated_object();
				} else {
					target = p_instance;
				}
				if (target && inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size() && registers[inst.src_b].get_type() == Variant::CALLABLE) {
					StringName sig_name = p_graph.method_names[inst.imm32];
					Callable c = registers[inst.src_b];
					target->connect(sig_name, c);
				}
			} break;

			case KnitOpcode::CALLABLE_CALL: {
				if (registers[inst.src_a].get_type() == Variant::CALLABLE) {
					Callable c = registers[inst.src_a];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					Callable::CallError ce;
					c.callp(call_args, arg_count, registers[inst.dst], ce);
				}
			} break;

			case KnitOpcode::YIELD_SECONDS: {
				if (p_instance) {
					KnitFrameKey key;
					key.instance_id = p_instance->get_instance_id();
					key.graph_id = p_graph.graph_id;
					key.coroutine_id = p_coroutine_id;

					KnitExecutionFrame frame;
					frame.owner_node = inst.debug_node_id;
					frame.pc = pc + 1;
					frame.yield_timer_remaining = float(registers[inst.src_a]);
					frame.yield_frames_remaining = 0;
					for (int r = 0; r < 16; r++) {
						frame.registers[r] = registers[r];
					}

					suspended_frames[key] = frame;
					return KnitVMStatus::Yielded;
				}
			} break;

			case KnitOpcode::YIELD_FRAMES: {
				if (p_instance) {
					KnitFrameKey key;
					key.instance_id = p_instance->get_instance_id();
					key.graph_id = p_graph.graph_id;
					key.coroutine_id = p_coroutine_id;

					KnitExecutionFrame frame;
					frame.owner_node = inst.debug_node_id;
					frame.pc = pc + 1;
					frame.yield_timer_remaining = 0.0f;
					frame.yield_frames_remaining = uint32_t(registers[inst.src_a]);
					for (int r = 0; r < 16; r++) {
						frame.registers[r] = registers[r];
					}

					suspended_frames[key] = frame;
					return KnitVMStatus::Yielded;
				}
			} break;

			case KnitOpcode::RETURN: {
				r_return_val = registers[inst.src_a];
				return KnitVMStatus::Returned;
			}

			case KnitOpcode::DEBUG_TRAP: {
				if (breakpoints.has(inst.debug_node_id)) {
					// Breakpoint hit!
				}
			} break;

			default:
				break;
		}

		pc++;
	}

	return KnitVMStatus::Ok;
}

KnitVMStatus KnitsBytecodeVM::resume_frame(const KnitCompiledGraph &p_graph, Object *p_instance, KnitExecutionFrame &r_frame, Variant &r_return_val) {
	Variant registers[MAX_REGISTERS];
	for (int r = 0; r < 16; r++) {
		registers[r] = r_frame.registers[r];
	}

	uint32_t pc = r_frame.pc;
	const int instruction_count = p_graph.instructions.size();

	while (pc < (uint32_t)instruction_count) {
		const KnitInstruction &inst = p_graph.instructions[pc];

		switch (inst.opcode) {
			case KnitOpcode::NOP:
				break;

			case KnitOpcode::LOAD_CONST: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.constants.size()) {
					registers[inst.dst] = p_graph.constants[inst.imm32];
				}
			} break;

			case KnitOpcode::LOAD_VAR: {
				if (p_instance && inst.imm32 >= 0 && inst.imm32 < p_graph.variable_names.size()) {
					registers[inst.dst] = p_instance->get(p_graph.variable_names[inst.imm32]);
				}
			} break;

			case KnitOpcode::STORE_VAR: {
				if (p_instance && inst.imm32 >= 0 && inst.imm32 < p_graph.variable_names.size()) {
					p_instance->set(p_graph.variable_names[inst.imm32], registers[inst.src_a]);
				}
			} break;

			case KnitOpcode::MOVE: {
				registers[inst.dst] = registers[inst.src_a];
			} break;

			case KnitOpcode::ADD_F: {
				registers[inst.dst] = double(registers[inst.src_a]) + double(registers[inst.src_b]);
			} break;

			case KnitOpcode::SUB_F: {
				registers[inst.dst] = double(registers[inst.src_a]) - double(registers[inst.src_b]);
			} break;

			case KnitOpcode::MUL_F: {
				registers[inst.dst] = double(registers[inst.src_a]) * double(registers[inst.src_b]);
			} break;

			case KnitOpcode::DIV_F: {
				double b = double(registers[inst.src_b]);
				if (unlikely(b == 0.0)) {
					evaluate_fault(inst, pc, "Division by zero (float)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = double(registers[inst.src_a]) / b;
			} break;

			case KnitOpcode::MOD_F: {
				double b = double(registers[inst.src_b]);
				if (unlikely(b == 0.0)) {
					evaluate_fault(inst, pc, "Modulo by zero (float)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = Math::fmod(double(registers[inst.src_a]), b);
			} break;

			case KnitOpcode::POW_F: {
				registers[inst.dst] = Math::pow(double(registers[inst.src_a]), double(registers[inst.src_b]));
			} break;

			case KnitOpcode::NEG_F: {
				registers[inst.dst] = -double(registers[inst.src_a]);
			} break;

			case KnitOpcode::ADD_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) + int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::SUB_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) - int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::MUL_I: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) * int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::DIV_I: {
				int64_t b = int64_t(registers[inst.src_b]);
				if (unlikely(b == 0)) {
					evaluate_fault(inst, pc, "Division by zero (int)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = int64_t(registers[inst.src_a]) / b;
			} break;

			case KnitOpcode::MOD_I: {
				int64_t b = int64_t(registers[inst.src_b]);
				if (unlikely(b == 0)) {
					evaluate_fault(inst, pc, "Modulo by zero (int)");
					return KnitVMStatus::Fault;
				}
				registers[inst.dst] = int64_t(registers[inst.src_a]) % b;
			} break;

			case KnitOpcode::NEG_I: {
				registers[inst.dst] = -int64_t(registers[inst.src_a]);
			} break;

			case KnitOpcode::BIT_AND: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) & int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_OR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) | int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_XOR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) ^ int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_NOT: {
				registers[inst.dst] = ~int64_t(registers[inst.src_a]);
			} break;

			case KnitOpcode::BIT_SHL: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) << int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::BIT_SHR: {
				registers[inst.dst] = int64_t(registers[inst.src_a]) >> int64_t(registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_EQ: {
				registers[inst.dst] = (registers[inst.src_a] == registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_NE: {
				registers[inst.dst] = (registers[inst.src_a] != registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_LT: {
				registers[inst.dst] = (registers[inst.src_a] < registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_LE: {
				registers[inst.dst] = (registers[inst.src_a] <= registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_GT: {
				registers[inst.dst] = (registers[inst.src_a] > registers[inst.src_b]);
			} break;

			case KnitOpcode::CMP_GE: {
				registers[inst.dst] = (registers[inst.src_a] >= registers[inst.src_b]);
			} break;

			case KnitOpcode::LOGICAL_NOT: {
				registers[inst.dst] = !bool(registers[inst.src_a]);
			} break;

			case KnitOpcode::LOGICAL_AND: {
				registers[inst.dst] = bool(registers[inst.src_a]) && bool(registers[inst.src_b]);
			} break;

			case KnitOpcode::LOGICAL_OR: {
				registers[inst.dst] = bool(registers[inst.src_a]) || bool(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_ADD: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) + Vector2(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_SUB: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) - Vector2(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_SCALE: {
				registers[inst.dst] = Vector2(registers[inst.src_a]) * float(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC2_DOT: {
				registers[inst.dst] = Vector2(registers[inst.src_a]).dot(Vector2(registers[inst.src_b]));
			} break;

			case KnitOpcode::VEC3_ADD: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) + Vector3(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_SUB: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) - Vector3(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_SCALE: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) * float(registers[inst.src_b]);
			} break;

			case KnitOpcode::VEC3_DOT: {
				registers[inst.dst] = Vector3(registers[inst.src_a]).dot(Vector3(registers[inst.src_b]));
			} break;

			case KnitOpcode::VEC3_CROSS: {
				registers[inst.dst] = Vector3(registers[inst.src_a]).cross(Vector3(registers[inst.src_b]));
			} break;

			case KnitOpcode::GET_INDEXED: {
				bool valid = false;
				registers[inst.dst] = registers[inst.src_a].get_named(registers[inst.src_b], valid);
				if (!valid) {
					registers[inst.dst] = registers[inst.src_a].get(registers[inst.src_b], &valid);
				}
			} break;

			case KnitOpcode::SET_INDEXED: {
				bool valid = false;
				registers[inst.src_a].set_named(registers[inst.src_b], registers[inst.extra], valid);
				if (!valid) {
					registers[inst.src_a].set(registers[inst.src_b], registers[inst.extra], &valid);
				}
			} break;

			case KnitOpcode::ARRAY_CONSTRUCT: {
				Array arr;
				int count = inst.extra;
				arr.resize(count);
				for (int i = 0; i < count; i++) {
					arr[i] = registers[inst.src_a + i];
				}
				registers[inst.dst] = arr;
			} break;

			case KnitOpcode::ARRAY_APPEND: {
				if (registers[inst.src_a].get_type() == Variant::ARRAY) {
					Array arr = registers[inst.src_a];
					arr.push_back(registers[inst.src_b]);
					registers[inst.src_a] = arr;
				}
			} break;

			case KnitOpcode::ARRAY_SIZE: {
				if (registers[inst.src_a].get_type() == Variant::ARRAY) {
					Array arr = registers[inst.src_a];
					registers[inst.dst] = (int64_t)arr.size();
				} else {
					registers[inst.dst] = (int64_t)0;
				}
			} break;

			case KnitOpcode::DICT_CONSTRUCT: {
				Dictionary d;
				int pairs = inst.extra;
				for (int i = 0; i < pairs; i++) {
					d[registers[inst.src_a + i * 2]] = registers[inst.src_a + i * 2 + 1];
				}
				registers[inst.dst] = d;
			} break;

			case KnitOpcode::DICT_GET: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					registers[inst.dst] = d.get(registers[inst.src_b], registers[inst.extra]);
				} else {
					registers[inst.dst] = registers[inst.extra];
				}
			} break;

			case KnitOpcode::DICT_SET: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					d[registers[inst.src_b]] = registers[inst.extra];
					registers[inst.src_a] = d;
				}
			} break;

			case KnitOpcode::DICT_HAS: {
				if (registers[inst.src_a].get_type() == Variant::DICTIONARY) {
					Dictionary d = registers[inst.src_a];
					registers[inst.dst] = d.has(registers[inst.src_b]);
				} else {
					registers[inst.dst] = false;
				}
			} break;

			case KnitOpcode::TYPE_TEST: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.constants.size()) {
					Variant target = p_graph.constants[inst.imm32];
					if (target.get_type() == Variant::INT) {
						registers[inst.dst] = (registers[inst.src_a].get_type() == (Variant::Type)int(target));
					} else if (target.get_type() == Variant::STRING_NAME || target.get_type() == Variant::STRING) {
						StringName class_name = target;
						Object *obj = registers[inst.src_a].get_validated_object();
						registers[inst.dst] = obj ? obj->is_class(class_name) : false;
					} else {
						registers[inst.dst] = false;
					}
				} else {
					registers[inst.dst] = false;
				}
			} break;

			case KnitOpcode::TYPE_CAST: {
				Variant::Type target_type = (Variant::Type)inst.imm32;
				Callable::CallError ce;
				const Variant *src = &registers[inst.src_a];
				Variant::construct(target_type, registers[inst.dst], &src, 1, ce);
			} break;

			case KnitOpcode::TYPE_OF: {
				registers[inst.dst] = (int64_t)registers[inst.src_a].get_type();
			} break;

			case KnitOpcode::IS_INSTANCE_VALID: {
				Object *obj = registers[inst.src_a].get_validated_object();
				registers[inst.dst] = (obj != nullptr);
			} break;

			case KnitOpcode::JUMP: {
				pc = (uint32_t)inst.imm32;
				continue;
			}

			case KnitOpcode::JUMP_IF_TRUE: {
				if (bool(registers[inst.src_a])) {
					pc = (uint32_t)inst.imm32;
					continue;
				}
			} break;

			case KnitOpcode::JUMP_IF_FALSE: {
				if (!bool(registers[inst.src_a])) {
					pc = (uint32_t)inst.imm32;
					continue;
				}
			} break;

			case KnitOpcode::CALL_METHOD_BIND: {
				Object *target_obj = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target_obj = registers[inst.src_a].get_validated_object();
					if (unlikely(!target_obj)) {
						evaluate_fault(inst, pc, "Target object is null on method call");
						return KnitVMStatus::Fault;
					}
				} else {
					target_obj = p_instance;
				}

				if (unlikely(!target_obj)) {
					evaluate_fault(inst, pc, "No valid instance object for method call");
					return KnitVMStatus::Fault;
				}

				if (inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName method_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}

					Callable::CallError ce;
					registers[inst.dst] = target_obj->callp(method_name, call_args, arg_count, ce);
					if (unlikely(ce.error != Callable::CallError::CALL_OK)) {
						evaluate_fault(inst, pc, vformat("Error calling method '%s'", String(method_name)));
						return KnitVMStatus::Fault;
					}
				}
			} break;

			case KnitOpcode::CALL_UTILITY: {
				if (inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName func_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					Callable::CallError ce;
					Variant::call_utility_function(func_name, &registers[inst.dst], call_args, arg_count, ce);
					if (unlikely(ce.error != Callable::CallError::CALL_OK)) {
						evaluate_fault(inst, pc, vformat("Error calling utility function '%s'", String(func_name)));
						return KnitVMStatus::Fault;
					}
				}
			} break;

			case KnitOpcode::SIGNAL_EMIT: {
				Object *target = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target = registers[inst.src_a].get_validated_object();
				} else {
					target = p_instance;
				}
				if (target && inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size()) {
					StringName sig_name = p_graph.method_names[inst.imm32];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					target->emit_signalp(sig_name, call_args, arg_count);
				}
			} break;

			case KnitOpcode::SIGNAL_CONNECT: {
				Object *target = nullptr;
				if (inst.src_a > 0 && registers[inst.src_a].get_type() == Variant::OBJECT) {
					target = registers[inst.src_a].get_validated_object();
				} else {
					target = p_instance;
				}
				if (target && inst.imm32 >= 0 && inst.imm32 < p_graph.method_names.size() && registers[inst.src_b].get_type() == Variant::CALLABLE) {
					StringName sig_name = p_graph.method_names[inst.imm32];
					Callable c = registers[inst.src_b];
					target->connect(sig_name, c);
				}
			} break;

			case KnitOpcode::CALLABLE_CALL: {
				if (registers[inst.src_a].get_type() == Variant::CALLABLE) {
					Callable c = registers[inst.src_a];
					int arg_count = inst.extra;
					const Variant *call_args[16];
					for (int a = 0; a < arg_count && a < 16; a++) {
						call_args[a] = &registers[inst.src_b + a];
					}
					Callable::CallError ce;
					c.callp(call_args, arg_count, registers[inst.dst], ce);
				}
			} break;

			case KnitOpcode::RETURN: {
				r_return_val = registers[inst.src_a];
				return KnitVMStatus::Returned;
			}

			default:
				break;
		}

		pc++;
	}

	return KnitVMStatus::Ok;
}

void KnitsBytecodeVM::tick_coroutines(double p_delta) {
	Vector<KnitFrameKey> to_resume;

	for (KeyValue<KnitFrameKey, KnitExecutionFrame> &E : suspended_frames) {
		KnitExecutionFrame &frame = E.value;
		if (frame.yield_timer_remaining > 0.0f) {
			frame.yield_timer_remaining -= float(p_delta);
			if (frame.yield_timer_remaining <= 0.0f) {
				to_resume.push_back(E.key);
			}
		} else if (frame.yield_frames_remaining > 0) {
			frame.yield_frames_remaining--;
			if (frame.yield_frames_remaining == 0) {
				to_resume.push_back(E.key);
			}
		}
	}

	// Resume frames that completed their timers
	for (int i = 0; i < to_resume.size(); i++) {
		KnitFrameKey key = to_resume[i];
		if (suspended_frames.has(key)) {
			// Note: The frame is popped and resumed by the owning script instance
			suspended_frames.erase(key);
		}
	}
}

void KnitsBytecodeVM::clear_coroutines_for_instance(ObjectID p_instance_id) {
	Vector<KnitFrameKey> to_erase;
	for (const KeyValue<KnitFrameKey, KnitExecutionFrame> &E : suspended_frames) {
		if (E.key.instance_id == p_instance_id) {
			to_erase.push_back(E.key);
		}
	}
	for (int i = 0; i < to_erase.size(); i++) {
		suspended_frames.erase(to_erase[i]);
	}
}

void KnitsBytecodeVM::set_breakpoint(KnitNodeID p_node_id, bool p_enabled) {
	if (p_enabled) {
		breakpoints.insert(p_node_id);
	} else {
		breakpoints.erase(p_node_id);
	}
}

bool KnitsBytecodeVM::has_breakpoint(KnitNodeID p_node_id) const {
	return breakpoints.has(p_node_id);
}

KnitsBytecodeVM::KnitsBytecodeVM() {
}

KnitsBytecodeVM::~KnitsBytecodeVM() {
}
