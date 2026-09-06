/**************************************************************************/
/*  zelyn_script_instance.h                                               */
/**************************************************************************/

#pragma once

#include "zelyn_script.h"
#include "core/object/script_language.h"
#include "core/templates/hash_map.h"

class ZelynScriptInstance : public ScriptInstance {
private:
	Object *owner = nullptr;
	Ref<ZelynScript> script;
	HashMap<StringName, Variant> member_properties;
	ZelynStateMachine state_machine;

	void _wire_declarative_signals();

public:
	bool transition_to(const StringName &p_state);
	StringName get_current_state() const { return state_machine.get_current_state(); }
	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
	virtual void validate_property(PropertyInfo &p_property) const override;

	virtual bool property_can_revert(const StringName &p_name) const override;
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const override;

	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) override;
	virtual void notification(int p_notification, bool p_reversed = false) override;
	virtual String to_string(bool *r_valid) override;

	virtual Ref<Script> get_script() const override;
	virtual ScriptLanguage *get_language() override;

	virtual const Variant get_rpc_config() const override { return Variant(); }

	ZelynScriptInstance(Object *p_owner, const Ref<ZelynScript> &p_script);
	virtual ~ZelynScriptInstance();
};
