/**************************************************************************/
/*  knits_compiler.h                                                      */
/**************************************************************************/

#pragma once

#include "knits_bytecode.h"
#include "knits_node.h"
#include "knits_types.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/vector.h"

class KnitsCompiler {
private:
	struct PureNodeEvaluationState {
		HashSet<KnitNodeID> visited;
		HashSet<KnitNodeID> in_stack; // For cycle detection
		HashMap<KnitPinID, uint8_t> pin_to_register;
	};

	uint8_t next_available_register = 0;
	HashMap<KnitPinID, uint8_t> pin_registers;
	HashMap<Variant, int> constant_pool_map;

	bool lower_pure_node(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_state, String &r_error);
	bool resolve_data_input(const KnitsGraph &p_graph, const KnitPin &p_input_pin, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_state, uint8_t &r_reg, String &r_error);
	bool compile_exec_block(const KnitsGraph &p_graph, const Ref<KnitNode> &p_start_node, KnitCompiledGraph &r_compiled, PureNodeEvaluationState &p_pure_state, HashSet<KnitNodeID> &r_visited_exec, String &r_error);

public:
	uint8_t allocate_register();
	int get_or_add_constant(KnitCompiledGraph &r_compiled, const Variant &p_val);
	int get_or_add_variable(KnitCompiledGraph &r_compiled, const StringName &p_name);
	int get_or_add_method(KnitCompiledGraph &r_compiled, const StringName &p_name);

	bool compile(const Ref<KnitsGraph> &p_graph, KnitCompiledGraph &r_compiled, String &r_error);

	KnitsCompiler();
	~KnitsCompiler();
};
