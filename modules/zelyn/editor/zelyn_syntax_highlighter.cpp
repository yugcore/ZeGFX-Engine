/**************************************************************************/
/*  zelyn_syntax_highlighter.cpp                                          */
/**************************************************************************/

#include "zelyn_syntax_highlighter.h"
#include "bridge/zelyn_language_features.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/variant/variant_utility.h"
#include "editor/settings/editor_settings.h"

void ZelynSyntaxHighlighter::_bind_methods() {}

PackedStringArray ZelynSyntaxHighlighter::_get_supported_languages() const {
	PackedStringArray langs;
	langs.push_back("Zelyn");
	langs.push_back("zl");
	return langs;
}

Ref<EditorSyntaxHighlighter> ZelynSyntaxHighlighter::_create() const {
	Ref<ZelynSyntaxHighlighter> h;
	h.instantiate();
	return h;
}

void ZelynSyntaxHighlighter::_update_cache() {
	if (highlighter.is_null()) {
		return;
	}

	highlighter->set_text_edit(text_edit);
	highlighter->clear_keyword_colors();
	highlighter->clear_member_keyword_colors();
	highlighter->clear_color_regions();

	Color keyword_color = EDITOR_GET("text_editor/theme/highlighting/keyword_color");
	Color control_flow_color = EDITOR_GET("text_editor/theme/highlighting/control_flow_keyword_color");
	Color comment_color = EDITOR_GET("text_editor/theme/highlighting/comment_color");
	Color doc_comment_color = EDITOR_GET("text_editor/theme/highlighting/doc_comment_color");
	Color string_color = EDITOR_GET("text_editor/theme/highlighting/string_color");
	Color number_color = EDITOR_GET("text_editor/theme/highlighting/number_color");
	Color symbol_color = EDITOR_GET("text_editor/theme/highlighting/symbol_color");
	Color function_color = EDITOR_GET("text_editor/theme/highlighting/function_color");
	Color member_variable_color = EDITOR_GET("text_editor/theme/highlighting/member_variable_color");
	Color engine_type_color = EDITOR_GET("text_editor/theme/highlighting/engine_type_color");
	Color user_type_color = EDITOR_GET("text_editor/theme/highlighting/user_type_color");
	Color base_type_color = EDITOR_GET("text_editor/theme/highlighting/base_type_color");

	highlighter->set_symbol_color(symbol_color);
	highlighter->set_function_color(function_color);
	highlighter->set_number_color(number_color);
	highlighter->set_member_variable_color(member_variable_color);

	/* Engine types (Node, Sprite2D, CharacterBody2D, Camera3D, Node3D, etc.) */
	LocalVector<StringName> types;
	ClassDB::get_class_list(types);
	for (const StringName &type : types) {
		highlighter->add_keyword_color(type, engine_type_color);
	}

	/* User / Global types */
	LocalVector<StringName> global_classes;
	ScriptServer::get_global_class_list(global_classes);
	for (const StringName &class_name : global_classes) {
		highlighter->add_keyword_color(class_name, user_type_color);
	}

	/* Variant types */
	for (int type = 0; type < Variant::Type::VARIANT_MAX; type++) {
		if (type != Variant::Type::NIL && type != Variant::Type::OBJECT) {
			highlighter->add_keyword_color(Variant::get_type_name((Variant::Type)type), base_type_color);
		}
	}
	highlighter->add_keyword_color("float", base_type_color);
	highlighter->add_keyword_color("int", base_type_color);
	highlighter->add_keyword_color("bool", base_type_color);
	highlighter->add_keyword_color("void", base_type_color);

	/* All Zelyn and GDScript keywords */
	Vector<String> keywords = ZelynLanguageFeatures::get_all_keywords();
	Vector<String> ctrl_flow = ZelynLanguageFeatures::get_control_flow_keywords();
	for (const String &kw : keywords) {
		if (ctrl_flow.has(kw)) {
			highlighter->add_keyword_color(kw, control_flow_color);
		} else {
			highlighter->add_keyword_color(kw, keyword_color);
		}
	}

	/* Built-in GDScript / Engine utility functions */
	List<StringName> utility_funcs;
	Variant::get_utility_function_list(&utility_funcs);
	for (const StringName &uf : utility_funcs) {
		highlighter->add_keyword_color(uf, function_color);
	}

	/* Comment regions */
	highlighter->add_color_region("//", "", comment_color, true);
	highlighter->add_color_region("/*", "*/", comment_color, false);
	highlighter->add_color_region("///", "", doc_comment_color, true);

	/* String regions */
	highlighter->add_color_region("\"", "\"", string_color, false);
	highlighter->add_color_region("'", "'", string_color, false);
}

Dictionary ZelynSyntaxHighlighter::_get_line_syntax_highlighting_impl(int p_line) {
	if (highlighter.is_valid()) {
		return highlighter->get_line_syntax_highlighting(p_line);
	}
	return Dictionary();
}

ZelynSyntaxHighlighter::ZelynSyntaxHighlighter() {
	highlighter.instantiate();
}

ZelynSyntaxHighlighter::~ZelynSyntaxHighlighter() {}
