/**************************************************************************/
/*  zelyn_state_machine.h                                                 */
/**************************************************************************/

#pragma once

#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/object/script_language.h"

struct ZelynState {
	StringName name;
	StringName enter_method;
	StringName update_method;
	StringName exit_method;
};

class ZelynStateMachine {
private:
	HashMap<StringName, ZelynState> states;
	StringName current_state;
	StringName previous_state;

public:
	void register_state(const ZelynState &p_state);
	bool has_state(const StringName &p_name) const;

	StringName get_current_state() const { return current_state; }
	StringName get_previous_state() const { return previous_state; }

	bool transition_to(ScriptInstance *p_instance, const StringName &p_target_state);
	void update(ScriptInstance *p_instance, double p_delta);

	bool is_empty() const { return states.is_empty(); }

	ZelynStateMachine();
	~ZelynStateMachine();
};
