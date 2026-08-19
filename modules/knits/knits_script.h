/**************************************************************************/
/*  knits_script.h                                                        */
/**************************************************************************/

#pragma once

#include "knits_bytecode.h"
#include "knits_compiler.h"
#include "knits_node.h"
#include "knits_serializer.h"
#include "knits_vm.h"
#include "core/object/script_language.h"

class KnitsScriptInstance;

class KnitsScript : public Script {
	GDCLASS(KnitsScript, Script);

private:
	Ref<KnitsGraph> main_graph;
	KnitCompiledGraph compiled_graph;
	bool is_valid = false;

protected:
	static void _bind_methods();

public:
	virtual bool can_instantiate() const override { return is_valid; }
	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual StringName get_global_name() const override { return StringName(); }
	virtual bool inherits_script(const Ref<Script> &p_script) const override { return false; }
	virtual StringName get_instance_base_type() const override { return "Object"; }

	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) override { return nullptr; }

	virtual bool has_source_code() const override { return true; }
	virtual String get_source_code() const override { return main_graph.is_valid() ? KnitsSerializer::serialize(main_graph) : ""; }
	virtual void set_source_code(const String &p_code) override {
		if (main_graph.is_null()) {
			main_graph.instantiate();
		}
		String err;
		KnitsSerializer::deserialize(p_code, main_graph, err);
		reload();
	}
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override { return "KnitsScript"; }
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

	void set_graph(const Ref<KnitsGraph> &p_graph);
	Ref<KnitsGraph> get_graph() const { return main_graph; }
	const KnitCompiledGraph &get_compiled_graph() const { return compiled_graph; }

	KnitsScript();
	~KnitsScript();
};

class KnitsScriptInstance : public ScriptInstance {
private:
	Object *owner = nullptr;
	Ref<KnitsScript> script;
	KnitsBytecodeVM vm;

public:
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

	virtual Ref<Script> get_script() const override { return script; }
	virtual Object *get_owner() override { return owner; }
	virtual ScriptLanguage *get_language() override;

	KnitsBytecodeVM &get_vm() { return vm; }

	KnitsScriptInstance(Object *p_owner, const Ref<KnitsScript> &p_script);
	virtual ~KnitsScriptInstance();
};
