/**************************************************************************/
/*  zelyn_script_language.cpp                                             */
/**************************************************************************/

#include "zelyn_script_language.h"
#include "zelyn_script.h"
#include "bridge/zelyn_variant_bridge.h"

#include "core/config/engine.h"
#include "core/os/os.h"
#include "zelyn/native_module.h"

#include "bridge/zelyn_language_features.h"

#ifdef TOOLS_ENABLED
#include "editor/zelyn_editor_language.h"
#endif

ZelynScriptLanguage *ZelynScriptLanguage::singleton = nullptr;

void ZelynScriptLanguage::_bind_methods() {}

void ZelynScriptLanguage::init() {
	singleton = this;

#ifdef TOOLS_ENABLED
	if (!editor_language) {
		editor_language = memnew(ZelynEditorLanguage);
	}
#endif

	// Bind engine global native functions & GDScript utilities into Zelyn NativeRegistry
	NativeRegistry reg;
	ZelynLanguageFeatures::register_godot_builtins(reg);
}

void ZelynScriptLanguage::finish() {
#ifdef TOOLS_ENABLED
	if (editor_language) {
		memdelete(editor_language);
		editor_language = nullptr;
	}
#endif

	if (singleton == this) {
		singleton = nullptr;
	}
}

Vector<String> ZelynScriptLanguage::get_reserved_words() const {
	return ZelynLanguageFeatures::get_all_keywords();
}

bool ZelynScriptLanguage::is_control_flow_keyword(const String &p_string) const {
	return ZelynLanguageFeatures::get_control_flow_keywords().has(p_string);
}

Vector<String> ZelynScriptLanguage::get_comment_delimiters() const {
	Vector<String> comments;
	comments.push_back("//");
	comments.push_back("/* */");
	return comments;
}

Vector<String> ZelynScriptLanguage::get_doc_comment_delimiters() const {
	Vector<String> doc_comments;
	doc_comments.push_back("///");
	return doc_comments;
}

Vector<String> ZelynScriptLanguage::get_string_delimiters() const {
	Vector<String> strings;
	strings.push_back("\" \"");
	strings.push_back("' '");
	return strings;
}

Ref<Script> ZelynScriptLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<ZelynScript> script;
	script.instantiate();

	String base_name = p_base_class_name.is_empty() ? "Node" : p_base_class_name;
	script->set_instance_base_type(base_name);

	String code;
	if (!p_template.is_empty()) {
		code = p_template;
		code = code.replace("_CLASS_", p_class_name);
		code = code.replace("_BASE_", base_name);
	} else {
		code += "// " + p_class_name + ".zl\n";
		code += "// extends " + base_name + "\n\n";
		code += "let speed = 300.0;\n\n";
		code += "func on_ready(self) {\n";
		code += "\tout(\"Zelyn script ready on: \" + self.get_name());\n";
		code += "}\n\n";
		code += "func on_process(self, dt) {\n";
		code += "\t// Zero-bloat gameplay logic\n";
		code += "}\n";
	}

	script->set_source_code(code);
	return script;
}

Vector<ScriptLanguage::ScriptTemplate> ZelynScriptLanguage::get_built_in_templates(const StringName &p_object) {
	Vector<ScriptTemplate> templates;

	ScriptTemplate templ;
	templ.inherit = p_object;
	templ.name = "Default";
	templ.description = "Default clean Zelyn script template";
	templ.content = "// _CLASS_.zl\n// extends _BASE_\n\nlet speed = 300.0;\n\nfunc on_ready(self) {\n\tout(\"Ready!\");\n}\n\nfunc on_process(self, dt) {\n}\n";
	templ.id = 0;
	templates.push_back(templ);

	return templates;
}

String ZelynScriptLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const {
	String f = "func " + p_name + "(";
	for (int i = 0; i < p_args.size(); i++) {
		if (i > 0) f += ", ";
		f += p_args[i];
	}
	f += ") {\n\t\n}\n";
	return f;
}

void ZelynScriptLanguage::frame() {
	// Frame boundary hook for game engine tick
}

#ifdef TOOLS_ENABLED
EditorLanguage *ZelynScriptLanguage::get_editor_language() {
	if (!editor_language) {
		editor_language = memnew(ZelynEditorLanguage);
	}
	return editor_language;
}
#endif

ZelynScriptLanguage::ZelynScriptLanguage() {
	singleton = this;
}

ZelynScriptLanguage::~ZelynScriptLanguage() {
#ifdef TOOLS_ENABLED
	if (editor_language) {
		memdelete(editor_language);
		editor_language = nullptr;
	}
#endif
	if (singleton == this) {
		singleton = nullptr;
	}
}
