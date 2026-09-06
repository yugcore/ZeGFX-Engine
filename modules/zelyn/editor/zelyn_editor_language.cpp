/**************************************************************************/
/*  zelyn_editor_language.cpp                                             */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "zelyn_editor_language.h"
#include "bridge/zelyn_language_features.h"
#include "core/variant/variant_utility.h"
#include "zelyn/tokenizer.h"
#include "zelyn/compiler.h"

ZelynEditorLanguage *ZelynEditorLanguage::singleton = nullptr;

Error ZelynEditorLanguage::complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) {
	if (!r_options) {
		return OK;
	}

	// Keywords
	Vector<String> keywords = ZelynLanguageFeatures::get_all_keywords();
	for (const String &kw : keywords) {
		r_options->push_back(ScriptLanguage::CodeCompletionOption(kw, ScriptLanguage::CODE_COMPLETION_KIND_PLAIN_TEXT));
	}

	// Native built-in utility functions
	List<StringName> ufuncs;
	Variant::get_utility_function_list(&ufuncs);
	for (const StringName &uf : ufuncs) {
		r_options->push_back(ScriptLanguage::CodeCompletionOption(uf, ScriptLanguage::CODE_COMPLETION_KIND_FUNCTION));
	}

	// Builtin callbacks & helpers
	static const char *builtins[] = {
		"on_ready", "on_process", "on_physics_process", "wait", "wait_seconds", "out", "print"
	};
	for (const char *bi : builtins) {
		r_options->push_back(ScriptLanguage::CodeCompletionOption(bi, ScriptLanguage::CODE_COMPLETION_KIND_FUNCTION));
	}

	if (p_owner) {
		List<MethodInfo> methods;
		p_owner->get_method_list(&methods);
		for (const MethodInfo &mi : methods) {
			r_options->push_back(ScriptLanguage::CodeCompletionOption(mi.name, ScriptLanguage::CODE_COMPLETION_KIND_FUNCTION));
		}
		List<PropertyInfo> props;
		p_owner->get_property_list(&props);
		for (const PropertyInfo &pi : props) {
			if (!(pi.usage & PROPERTY_USAGE_CATEGORY) && !(pi.usage & PROPERTY_USAGE_GROUP)) {
				r_options->push_back(ScriptLanguage::CodeCompletionOption(pi.name, ScriptLanguage::CODE_COMPLETION_KIND_MEMBER));
			}
		}
	}

	return OK;
}

Error ZelynEditorLanguage::lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, LookupResult &r_result) {
	if (p_symbol.is_empty()) {
		return ERR_UNAVAILABLE;
	}

	if (p_owner && p_owner->has_method(p_symbol)) {
		r_result.type = LookupResult::Type::CLASS_METHOD;
		r_result.class_name = p_owner->get_class_name();
		r_result.class_member = p_symbol;
		return OK;
	}

	return ERR_UNAVAILABLE;
}

int32_t ZelynEditorLanguage::find_function(const String &p_function, const String &p_code) const {
	if (p_code.is_empty() || p_function.is_empty()) {
		return -1;
	}

	CharString utf8 = p_code.utf8();
	std::string code_str(utf8.get_data(), utf8.length());
	try {
		std::vector<Token> raw_tokens = tokenize(code_str);
		std::vector<Token> tokens = ZelynLanguageFeatures::normalize_tokens(raw_tokens);
		std::string fn_name_target = p_function.utf8().get_data();
		for (size_t i = 0; i < tokens.size(); i++) {
			std::string t_val = tokens[i].getValue();
			if ((t_val == "func" || t_val == "fn") && i + 1 < tokens.size()) {
				if (tokens[i + 1].getValue() == fn_name_target) {
					return tokens[i].line > 0 ? tokens[i].line - 1 : 0;
				}
			}
		}
	} catch (...) {}

	return -1;
}

void ZelynEditorLanguage::format_code(String &r_code, uint32_t p_from_line, uint32_t p_to_line) const {
	// Formatting logic
}

bool ZelynEditorLanguage::validate(const String &p_code, const String &p_path, List<ScriptError> *r_errors, List<Warning> *r_warnings, List<String> *r_functions, HashSet<int> *r_safe_lines) const {
	if (p_code.is_empty()) {
		return true;
	}

	CharString utf8 = p_code.utf8();
	std::string code_str(utf8.get_data(), utf8.length());

	try {
		std::vector<Token> raw_tokens = tokenize(code_str);
		std::vector<Token> tokens = ZelynLanguageFeatures::normalize_tokens(raw_tokens);

		if (r_functions) {
			for (size_t i = 0; i < tokens.size(); i++) {
				std::string t_val = tokens[i].getValue();
				if ((t_val == "func" || t_val == "fn") && i + 1 < tokens.size()) {
					String fn_name = tokens[i + 1].getValue().c_str();
					int line_num = tokens[i].line > 0 ? tokens[i].line : 1;
					r_functions->push_back(fn_name + ":" + itos(line_num));
				}
			}
		}

		Compiler compiler;
		compiler.allowTopLevelStatements = true;
		compiler.compile(tokens);
		return true;
	} catch (const std::exception &e) {
		if (r_errors) {
			ScriptError err;
			err.path = p_path;
			err.message = e.what();
			err.start_line = 1;
			err.start_column = 1;
			err.end_line = 1;
			err.end_column = 1;
			r_errors->push_back(err);
		}
		return false;
	} catch (...) {
		if (r_errors) {
			ScriptError err;
			err.path = p_path;
			err.message = "Unknown compilation error";
			err.start_line = 1;
			r_errors->push_back(err);
		}
		return false;
	}
}

ZelynEditorLanguage::ZelynEditorLanguage() {
	if (!singleton) {
		singleton = this;
	}
}

ZelynEditorLanguage::~ZelynEditorLanguage() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

#endif // TOOLS_ENABLED
