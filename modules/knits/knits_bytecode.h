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
	MOD_F,           // R[dst] = fmod(R[src_a], R[src_b])
	POW_F,           // R[dst] = pow(R[src_a], R[src_b])
	NEG_F,           // R[dst] = -R[src_a]

	// Arithmetic (Int)
	ADD_I,           // R[dst] = R[src_a] + R[src_b]
	SUB_I,           // R[dst] = R[src_a] - R[src_b]
	MUL_I,           // R[dst] = R[src_a] * R[src_b]
	DIV_I,           // R[dst] = R[src_a] / R[src_b] (Fault if src_b == 0)
	MOD_I,           // R[dst] = R[src_a] % R[src_b]
	NEG_I,           // R[dst] = -R[src_a]

	// Bitwise Operations
	BIT_AND,         // R[dst] = R[src_a] & R[src_b]
	BIT_OR,          // R[dst] = R[src_a] | R[src_b]
	BIT_XOR,         // R[dst] = R[src_a] ^ R[src_b]
	BIT_NOT,         // R[dst] = ~R[src_a]
	BIT_SHL,         // R[dst] = R[src_a] << R[src_b]
	BIT_SHR,         // R[dst] = R[src_a] >> R[src_b]

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
	VEC2_ADD,        // R[dst] = Vector2(R[src_a]) + Vector2(R[src_b])
	VEC2_SUB,        // R[dst] = Vector2(R[src_a]) - Vector2(R[src_b])
	VEC2_SCALE,      // R[dst] = Vector2(R[src_a]) * float(R[src_b])
	VEC2_DOT,        // R[dst] = Vector2(R[src_a]).dot(Vector2(R[src_b]))
	VEC3_ADD,        // R[dst] = Vector3(R[src_a]) + Vector3(R[src_b])
	VEC3_SUB,        // R[dst] = Vector3(R[src_a]) - Vector3(R[src_b])
	VEC3_SCALE,      // R[dst] = Vector3(R[src_a]) * float(R[src_b])
	VEC3_DOT,        // R[dst] = Vector3(R[src_a]).dot(Vector3(R[src_b]))
	VEC3_CROSS,      // R[dst] = Vector3(R[src_a]).cross(Vector3(R[src_b]))

	// Containers & Indexing
	GET_INDEXED,     // R[dst] = R[src_a][R[src_b]]
	SET_INDEXED,     // R[src_a][R[src_b]] = R[extra]
	ARRAY_CONSTRUCT, // R[dst] = Array from R[src_a..src_a+extra-1]
	ARRAY_APPEND,    // Array(R[src_a]).push_back(R[src_b])
	ARRAY_SIZE,      // R[dst] = Array(R[src_a]).size()
	DICT_CONSTRUCT,  // R[dst] = Dictionary from pairs in R[src_a..src_a+extra*2-1]
	DICT_GET,        // R[dst] = Dict(R[src_a]).get(R[src_b], R[extra])
	DICT_SET,        // Dict(R[src_a])[R[src_b]] = R[extra]
	DICT_HAS,        // R[dst] = Dict(R[src_a]).has(R[src_b])

	// Type Testing & Reflection
	TYPE_TEST,       // R[dst] = (R[src_a] is type/class in imm32)
	TYPE_CAST,       // R[dst] = Variant::construct(type in imm32, R[src_a])
	TYPE_OF,         // R[dst] = typeof(R[src_a])
	IS_INSTANCE_VALID,// R[dst] = is_instance_valid(R[src_a])

	// Control Flow
	JUMP,            // PC = imm32
	JUMP_IF_TRUE,    // if (bool(R[src_a])) PC = imm32
	JUMP_IF_FALSE,   // if (!bool(R[src_a])) PC = imm32

	// Engine Interop & Function Calls
	CALL_METHOD_BIND,// Target in R[src_a], Method name index in imm32, Args start at R[src_b], Arg count in extra byte
	CALL_UTILITY,    // Utility function name index in imm32, Args start at R[src_b], Arg count in extra byte
	CALL_SCRIPT_INST,// Cross-language call into GDScript / Zelyn
	CALL_LOCAL_FUNC, // Jump to local compiled function in same script

	// Signals & Callables
	SIGNAL_EMIT,     // Emit signal on R[src_a] with name index imm32 and args at R[src_b]
	SIGNAL_CONNECT,  // Connect signal R[src_a] name imm32 to callable R[src_b]
	CALLABLE_CALL,   // Call callable in R[src_a] with args at R[src_b]

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
