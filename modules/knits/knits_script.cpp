/**************************************************************************/
/*  knits_script.cpp                                                      */
/**************************************************************************/

#include "knits_script.h"
#include "knits_language.h"

void KnitsScript::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_graph", "graph"), &KnitsScript::set_graph);
	ClassDB::bind_method(D_METHOD("get_graph"), &KnitsScript::get_graph);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "graph", PROPERTY_HINT_RESOURCE_TYPE, "KnitsGraph"), "set_graph", "get_graph");
}

ScriptInstance *KnitsScript::instance_create(Object *p_this) {
	KnitsScriptInstance *instance = memnew(KnitsScriptInstance(p_this, Ref<KnitsScript>(this)));
	return instance;
}

Error KnitsScript::reload(bool p_keep_state) {
	if (main_graph.is_null()) {
		is_valid = false;
		return OK;
	}

	KnitsCompiler compiler;
	String error;
	if (compiler.compile(main_graph, compiled_graph, error)) {
		is_valid = true;
		return OK;
	} else {
		is_valid = false;
		ERR_PRINT(vformat("[Knits Compiler Error] %s", error));
		return ERR_COMPILATION_FAILED;
	}
}

bool KnitsScript::has_method(const StringName &p_method) const {
	if (!is_valid) return false;
	return compiled_graph.graph_name == p_method || p_method == "_ready" || p_method == "_process" || p_method == "_physics_process";
}

MethodInfo KnitsScript::get_method_info(const StringName &p_method) const {
	MethodInfo mi;
	mi.name = p_method;
	return mi;
}

ScriptLanguage *KnitsScript::get_language() const {
	return KnitsScriptLanguage::get_singleton();
}

bool KnitsScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	if (main_graph.is_valid() && main_graph->has_variable(p_property)) {
		r_value = main_graph->variables[p_property].default_value;
		return true;
	}
	return false;
}

void KnitsScript::get_script_method_list(List<MethodInfo> *p_list) const {
	if (is_valid && p_list) {
		MethodInfo mi;
		mi.name = compiled_graph.graph_name;
		p_list->push_back(mi);
	}
}

void KnitsScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	if (main_graph.is_valid() && p_list) {
		for (const KeyValue<StringName, KnitVariable> &E : main_graph->variables) {
			if (E.value.is_exported) {
				p_list->push_back(PropertyInfo(Variant::NIL, E.key));
			}
		}
	}
}

void KnitsScript::set_graph(const Ref<KnitsGraph> &p_graph) {
	main_graph = p_graph;
	reload();
}

KnitsScript::KnitsScript() {
}

KnitsScript::~KnitsScript() {
}

///////////////////////////////////////////////////////////////////////////////
// KnitsScriptInstance

bool KnitsScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	return false;
}

bool KnitsScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	return false;
}

void KnitsScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (script.is_valid()) {
		script->get_script_property_list(p_properties);
	}
}

Variant::Type KnitsScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (r_is_valid) *r_is_valid = false;
	return Variant::NIL;
}

void KnitsScriptInstance::validate_property(PropertyInfo &p_property) const {
}

bool KnitsScriptInstance::property_can_revert(const StringName &p_name) const {
	return false;
}

bool KnitsScriptInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
	return false;
}

void KnitsScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool KnitsScriptInstance::has_method(const StringName &p_method) const {
	if (script.is_valid()) {
		return script->has_method(p_method);
	}
	return false;
}

Variant KnitsScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) {
	r_error.error = Callable::CallError::CALL_OK;
	if (script.is_null() || !script->is_script_valid()) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}

	Variant return_val;
	KnitVMStatus status = vm.execute(script->get_compiled_graph(), owner, p_args, p_arg_count, return_val);
	if (status == KnitVMStatus::Fault) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
	}

	return return_val;
}

void KnitsScriptInstance::notification(int p_notification, bool p_reversed) {
}

ScriptLanguage *KnitsScriptInstance::get_language() {
	return KnitsScriptLanguage::get_singleton();
}

KnitsScriptInstance::KnitsScriptInstance(Object *p_owner, const Ref<KnitsScript> &p_script) {
	owner = p_owner;
	script = p_script;
}

KnitsScriptInstance::~KnitsScriptInstance() {
	if (owner) {
		vm.clear_coroutines_for_instance(owner->get_instance_id());
	}
}
