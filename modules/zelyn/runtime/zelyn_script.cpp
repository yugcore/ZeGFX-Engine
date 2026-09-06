/**************************************************************************/
/*  zelyn_script.cpp                                                      */
/**************************************************************************/

#include "zelyn_script.h"
#include "zelyn_script_instance.h"
#include "zelyn_script_language.h"

#include "core/object/class_db.h"
#include "bridge/zelyn_language_features.h"
#include "zelyn/tokenizer.h"
#include "zelyn/compiler.h"

void ZelynScript::_bind_methods() {}

ScriptInstance *ZelynScript::instance_create(Object *p_this) {
	ZelynScriptInstance *inst = memnew(ZelynScriptInstance(p_this, Ref<ZelynScript>(this)));
	return inst;
}

PlaceHolderScriptInstance *ZelynScript::placeholder_instance_create(Object *p_this) {
#ifdef TOOLS_ENABLED
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(ZelynScriptLanguage::get_singleton(), Ref<Script>(this), p_this));
	placeholders.insert(si);
	si->update(property_info_list, property_default_values);
	return si;
#else
	return nullptr;
#endif
}

Error ZelynScript::reload(bool p_keep_state) {
	is_valid = false;
	method_info_map.clear();
	property_info_list.clear();
	property_default_values.clear();
	declarative_handlers.clear();
	declared_states.clear();

	if (source_code.is_empty()) {
		return OK;
	}

	// Check for "extends <BaseType>" in source
	int extends_idx = source_code.find("extends ");
	if (extends_idx != -1) {
		int start = extends_idx + 8;
		while (start < source_code.length() && (source_code[start] == ' ' || source_code[start] == '\t')) {
			start++;
		}
		int end = start;
		while (end < source_code.length() && ((source_code[end] >= 'a' && source_code[end] <= 'z') || (source_code[end] >= 'A' && source_code[end] <= 'Z') || (source_code[end] >= '0' && source_code[end] <= '9') || source_code[end] == '_')) {
			end++;
		}
		if (end > start) {
			String parsed_base = source_code.substr(start, end - start);
			if (ClassDB::class_exists(parsed_base)) {
				instance_base_type = StringName(parsed_base);
			}
		}
	}
	if (instance_base_type == StringName()) {
		instance_base_type = "Node";
	}

	CharString utf8 = source_code.utf8();
	std::string code_str(utf8.get_data(), utf8.length());

	try {
		std::vector<Token> raw_tokens = tokenize(code_str);
		std::vector<Token> tokens = ZelynLanguageFeatures::normalize_tokens(raw_tokens);

		// Scan for function signatures, properties, and declarative signal handlers
		for (size_t i = 0; i < tokens.size(); i++) {
			std::string t_val = tokens[i].getValue();
			if (t_val == "func" || t_val == "fn") {
				if (i + 1 < tokens.size()) {
					StringName fn_name(tokens[i + 1].getValue().c_str());
					MethodInfo mi;
					mi.name = fn_name;
					method_info_map[fn_name] = mi;
				}
			} else if (t_val == "let" || t_val == "var") {
				if (i + 1 < tokens.size()) {
					String prop_name(tokens[i + 1].getValue().c_str());
					PropertyInfo pi(Variant::FLOAT, prop_name);
					property_info_list.push_back(pi);
				}
			} else if (t_val == "on") {
				// Declarative signal handler: on NodeName.signal_name(...)
				if (i + 3 < tokens.size() && tokens[i + 2].getValue() == ".") {
					ZelynDeclarativeHandler handler;
					handler.node_name = tokens[i + 1].getValue().c_str();
					handler.signal_name = tokens[i + 3].getValue().c_str();
					handler.function_name = "_on_" + handler.node_name + "_" + handler.signal_name;
					declarative_handlers.push_back(handler);

					MethodInfo mi;
					mi.name = StringName(handler.function_name);
					method_info_map[mi.name] = mi;
				}
			} else if (t_val == "state") {
				if (i + 1 < tokens.size()) {
					String state_name = tokens[i + 1].getValue().c_str();
					ZelynState st;
					st.name = StringName(state_name);
					st.enter_method = StringName("_state_" + state_name + "_enter");
					st.update_method = StringName("_state_" + state_name + "_update");
					st.exit_method = StringName("_state_" + state_name + "_exit");
					declared_states.push_back(st);

					MethodInfo mi_enter; mi_enter.name = st.enter_method; method_info_map[st.enter_method] = mi_enter;
					MethodInfo mi_update; mi_update.name = st.update_method; method_info_map[st.update_method] = mi_update;
					MethodInfo mi_exit; mi_exit.name = st.exit_method; method_info_map[st.exit_method] = mi_exit;
				}
			}
		}

		Compiler compiler;
		compiler.allowTopLevelStatements = true;
		compiled_chunk = compiler.compile(tokens);
		BytecodeVM &vm = ZelynScriptLanguage::get_singleton()->get_shared_vm();
		for (auto &pair : compiler.userFunctions) {
			vm.userFunctions[pair.first] = pair.second;
		}
		is_valid = true;

#ifdef TOOLS_ENABLED
		for (PlaceHolderScriptInstance *pi : placeholders) {
			pi->update(property_info_list, property_default_values);
		}
#endif
	} catch (const std::exception &e) {
		ERR_PRINT(String("Zelyn compile error: ") + e.what());
		is_valid = false;
		return ERR_PARSE_ERROR;
	} catch (...) {
		ERR_PRINT("Zelyn compile error: unknown exception");
		is_valid = false;
		return ERR_PARSE_ERROR;
	}

	return OK;
}

bool ZelynScript::has_method(const StringName &p_method) const {
	return method_info_map.has(p_method);
}

MethodInfo ZelynScript::get_method_info(const StringName &p_method) const {
	const MethodInfo *mi = method_info_map.getptr(p_method);
	return mi ? *mi : MethodInfo();
}

ScriptLanguage *ZelynScript::get_language() const {
	return ZelynScriptLanguage::get_singleton();
}

bool ZelynScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	const Variant *val = property_default_values.getptr(p_property);
	if (val) {
		r_value = *val;
		return true;
	}
	return false;
}

void ZelynScript::get_script_method_list(List<MethodInfo> *p_list) const {
	for (const KeyValue<StringName, MethodInfo> &E : method_info_map) {
		p_list->push_back(E.value);
	}
}

void ZelynScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	for (const PropertyInfo &pi : property_info_list) {
		p_list->push_back(pi);
	}
}

ZelynScript::ZelynScript() {}

ZelynScript::~ZelynScript() {}
