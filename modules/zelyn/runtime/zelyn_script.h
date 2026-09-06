/**************************************************************************/
/*  zelyn_script.h                                                        */
/**************************************************************************/

#pragma once

#include "core/object/script_language.h"
#include "core/templates/hash_set.h"
#include "zelyn/bytecode.h"
#include "zelyn/compiler.h"

class ZelynScriptInstance;

#include "zelyn_state_machine.h"

struct ZelynDeclarativeHandler {
	String node_name;
	String signal_name;
	String function_name;
};

class ZelynScript : public Script {
	GDCLASS(ZelynScript, Script);

private:
	String source_code;
	Chunk compiled_chunk;
	bool is_valid = false;
	StringName instance_base_type = "Node";

	HashMap<StringName, MethodInfo> method_info_map;
	List<PropertyInfo> property_info_list;
	HashMap<StringName, Variant> property_default_values;
	Vector<ZelynDeclarativeHandler> declarative_handlers;
	Vector<ZelynState> declared_states;

#ifdef TOOLS_ENABLED
	HashSet<PlaceHolderScriptInstance *> placeholders;
	virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) override {
		placeholders.erase(p_placeholder);
	}
#endif

protected:
	static void _bind_methods();

public:
	virtual bool can_instantiate() const override { return is_valid; }
	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual StringName get_global_name() const override { return StringName(); }
	virtual bool inherits_script(const Ref<Script> &p_script) const override { return false; }

	void set_instance_base_type(const StringName &p_type) { instance_base_type = p_type; }
	virtual StringName get_instance_base_type() const override { return instance_base_type; }

	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) override;

	virtual bool has_source_code() const override { return true; }
	virtual String get_source_code() const override { return source_code; }
	virtual void set_source_code(const String &p_code) override {
		source_code = p_code;
		reload();
	}
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override { return "ZelynScript"; }
	virtual Vector<DocData::ClassDoc> get_documentation() const override { return Vector<DocData::ClassDoc>(); }
	virtual String get_class_icon_path() const override { return ""; }
#endif // TOOLS_ENABLED

	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual bool is_tool() const override { return false; }
	virtual bool is_script_valid() const override { return is_valid; }
	virtual bool is_abstract() const override { return false; }

	virtual ScriptLanguage *get_language() const override;

	virtual bool has_script_signal(const StringName &p_signal) const override { return false; }
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override {}

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;
	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;

	virtual const Variant get_rpc_config() const override { return Variant(); }

	const Chunk &get_compiled_chunk() const { return compiled_chunk; }
	const Vector<ZelynDeclarativeHandler> &get_declarative_handlers() const { return declarative_handlers; }
	const Vector<ZelynState> &get_declared_states() const { return declared_states; }

	ZelynScript();
	~ZelynScript();
};
