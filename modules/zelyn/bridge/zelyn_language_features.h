/**************************************************************************/
/*  zelyn_language_features.h                                             */
/**************************************************************************/

#pragma once

#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "zelyn/common.h"
#include "zelyn/native_module.h"

class ZelynLanguageFeatures {
public:
	// Normalizes tokens to support type annotations (: type), let keyword, -> return type, null, elif, pass
	static std::vector<Token> normalize_tokens(const std::vector<Token> &p_tokens);

	// Binds GDScript and Godot engine built-in math and utility functions into Zelyn NativeRegistry
	static void register_godot_builtins(NativeRegistry &reg);

	// Returns all keywords for syntax highlighting and autocomplete
	static Vector<String> get_all_keywords();
	static Vector<String> get_control_flow_keywords();
};
