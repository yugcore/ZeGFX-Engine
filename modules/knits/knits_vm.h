/**************************************************************************/
/*  knits_vm.h                                                            */
/**************************************************************************/

#pragma once

#include "knits_bytecode.h"
#include "knits_types.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

enum class KnitVMStatus : uint8_t {
	Ok,
	Yielded,
	Fault,
	Returned
};

struct KnitVMFault {
	KnitNodeID node_id = 0;
	uint32_t pc = 0;
	String message;
};

struct KnitFrameKeyHasher {
	static _FORCE_INLINE_ uint32_t hash(const KnitFrameKey &p_key) {
		return p_key.hash();
	}
};

class KnitsBytecodeVM {
private:
	HashMap<KnitFrameKey, KnitExecutionFrame, KnitFrameKeyHasher> suspended_frames;
	HashSet<KnitNodeID> breakpoints;
	KnitVMFault last_fault;
	bool fault_occurred = false;

	bool evaluate_fault(const KnitInstruction &p_inst, uint32_t p_pc, const String &p_message);

public:
	static constexpr int MAX_REGISTERS = 64;

	KnitVMStatus execute(const KnitCompiledGraph &p_graph, Object *p_instance, const Variant **p_args, int p_arg_count, Variant &r_return_val, uint32_t p_coroutine_id = 0);
	KnitVMStatus resume_frame(const KnitCompiledGraph &p_graph, Object *p_instance, KnitExecutionFrame &r_frame, Variant &r_return_val);

	void tick_coroutines(double p_delta);
	void clear_coroutines_for_instance(ObjectID p_instance_id);

	void set_breakpoint(KnitNodeID p_node_id, bool p_enabled);
	bool has_breakpoint(KnitNodeID p_node_id) const;

	bool has_fault() const { return fault_occurred; }
	KnitVMFault get_last_fault() const { return last_fault; }
	void clear_fault() { fault_occurred = false; last_fault = KnitVMFault(); }

	int get_suspended_frame_count() const { return suspended_frames.size(); }

	KnitsBytecodeVM();
	~KnitsBytecodeVM();
};
