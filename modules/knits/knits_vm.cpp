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

			case KnitOpcode::VEC3_ADD: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) + Vector3(registers[inst.src_b]);
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

			case KnitOpcode::VEC3_ADD: {
				registers[inst.dst] = Vector3(registers[inst.src_a]) + Vector3(registers[inst.src_b]);
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
