/**************************************************************************/
/*  zelyn_state_machine.cpp                                               */
/**************************************************************************/

#include "zelyn_state_machine.h"

void ZelynStateMachine::register_state(const ZelynState &p_state) {
	states[p_state.name] = p_state;
	if (current_state == StringName()) {
		current_state = p_state.name;
	}
}

bool ZelynStateMachine::has_state(const StringName &p_name) const {
	return states.has(p_name);
}

bool ZelynStateMachine::transition_to(ScriptInstance *p_instance, const StringName &p_target_state) {
	if (!states.has(p_target_state) || !p_instance) {
		return false;
	}

	// 1. Exit current state
	if (states.has(current_state)) {
		const ZelynState &cur = states[current_state];
		if (cur.exit_method != StringName() && p_instance->has_method(cur.exit_method)) {
			Callable::CallError err;
			p_instance->callp(cur.exit_method, nullptr, 0, err);
		}
	}

	previous_state = current_state;
	current_state = p_target_state;

	// 2. Enter new state
	const ZelynState &next = states[current_state];
	if (next.enter_method != StringName() && p_instance->has_method(next.enter_method)) {
		Callable::CallError err;
		p_instance->callp(next.enter_method, nullptr, 0, err);
	}

	return true;
}

void ZelynStateMachine::update(ScriptInstance *p_instance, double p_delta) {
	if (!p_instance || current_state == StringName()) return;

	if (states.has(current_state)) {
		const ZelynState &cur = states[current_state];
		if (cur.update_method != StringName() && p_instance->has_method(cur.update_method)) {
			Variant v_delta = p_delta;
			const Variant *args[1] = { &v_delta };
			Callable::CallError err;
			p_instance->callp(cur.update_method, args, 1, err);
		}
	}
}

ZelynStateMachine::ZelynStateMachine() {}

ZelynStateMachine::~ZelynStateMachine() {}
