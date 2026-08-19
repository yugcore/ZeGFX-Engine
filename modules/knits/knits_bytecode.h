/**************************************************************************/
/*  knits_bytecode.h                                                      */
/**************************************************************************/

#pragma once

#include "knits_types.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

enum class KnitOpcode : uint8_t {
	NOP = 0,
	LOAD_CONST,      // R[dst] = constants[imm32]
	LOAD_VAR,        // R[dst] = variables[imm32]
	STORE_VAR,       // variables[imm32] = R[src_a]
	MOVE,            // R[dst] = R[src_a]

	// Arithmetic (Float)
	ADD_F,           // R[dst] = R[src_a] + R[src_b]
	SUB_F,           // R[dst] = R[src_a] - R[src_b]
	MUL_F,           // R[dst] = R[src_a] * R[src_b]
	DIV_F,           // R[dst] = R[src_a] / R[src_b] (Fault if src_b == 0.0)

	// Arithmetic (Int)
	ADD_I,           // R[dst] = R[src_a] + R[src_b]
	SUB_I,           // R[dst] = R[src_a] - R[src_b]
	MUL_I,           // R[dst] = R[src_a] * R[src_b]
	DIV_I,           // R[dst] = R[src_a] / R[src_b] (Fault if src_b == 0)

	// Comparison
	CMP_EQ,          // R[dst] = (R[src_a] == R[src_b])
	CMP_NE,          // R[dst] = (R[src_a] != R[src_b])
	CMP_LT,          // R[dst] = (R[src_a] < R[src_b])
	CMP_LE,          // R[dst] = (R[src_a] <= R[src_b])
	CMP_GT,          // R[dst] = (R[src_a] > R[src_b])
	CMP_GE,          // R[dst] = (R[src_a] >= R[src_b])

	// Logical
	LOGICAL_NOT,     // R[dst] = !R[src_a]
	LOGICAL_AND,     // R[dst] = R[src_a] && R[src_b]
	LOGICAL_OR,      // R[dst] = R[src_a] || R[src_b]

	// Vector Math
	VEC3_ADD,        // R[dst] = Vector3(R[src_a]) + Vector3(R[src_b])
	VEC3_SCALE,      // R[dst] = Vector3(R[src_a]) * float(R[src_b])
	VEC3_DOT,        // R[dst] = Vector3(R[src_a]).dot(Vector3(R[src_b]))
	VEC3_CROSS,      // R[dst] = Vector3(R[src_a]).cross(Vector3(R[src_b]))

	// Control Flow
	JUMP,            // PC = imm32
	JUMP_IF_TRUE,    // if (bool(R[src_a])) PC = imm32
	JUMP_IF_FALSE,   // if (!bool(R[src_a])) PC = imm32

	// Engine Interop & Function Calls
	CALL_METHOD_BIND,// Target in R[src_a], Method name index in imm32, Args start at R[src_b], Arg count in extra byte
	CALL_SCRIPT_INST,// Cross-language call into GDScript / Zelyn
	CALL_LOCAL_FUNC, // Jump to local compiled function in same script

	// Resumable Coroutine State-Machine Operations
	YIELD_FRAMES,    // Suspend VM state for N frames (R[src_a])
	YIELD_SECONDS,   // Suspend VM state for DeltaTime accumulator (R[src_a])
	RETURN,          // Return from function with R[src_a]

	// Debug & Fault Traps
	DEBUG_TRAP       // Breakpoint check with KnitNodeID in imm_u64/imm32
};

struct KnitInstruction {
	KnitOpcode opcode = KnitOpcode::NOP;
	uint8_t dst = 0;
	uint8_t src_a = 0;
	uint8_t src_b = 0;
	uint8_t extra = 0; // e.g. arg_count
	int32_t imm32 = 0;
	KnitNodeID debug_node_id = 0;
};

struct KnitDebugSymbol {
	KnitNodeID node_id = 0;
	KnitPinID pin_id = 0;
};

struct KnitCompiledGraph {
	KnitGraphID graph_id = 0;
	StringName graph_name;
	bool is_function = false;

	Vector<KnitInstruction> instructions;
	Vector<Variant> constants;
	Vector<StringName> variable_names;
	Vector<StringName> method_names;

	HashMap<uint32_t, KnitDebugSymbol> debug_symbols; // Bytecode PC -> Debug Symbol
	uint8_t register_count = 16;
};
