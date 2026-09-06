/**************************************************************************/
/*  zelyn_editor_language.h                                               */
/**************************************************************************/

#pragma once

#ifdef TOOLS_ENABLED

#include "core/object/editor_language.h"

class ZelynEditorLanguage final : public EditorLanguage {
	static ZelynEditorLanguage *singleton;

public:
	_FORCE_INLINE_ static ZelynEditorLanguage *get_singleton() { return singleton; }

	virtual Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptLanguage::CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) override;
	virtual Error lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, LookupResult &r_result) override;
	virtual int32_t find_function(const String &p_function, const String &p_code) const override;
	virtual void format_code(String &r_code, uint32_t p_from_line, uint32_t p_to_line) const override;
	virtual bool validate(const String &p_code, const String &p_path, List<ScriptError> *r_errors, List<Warning> *r_warnings, List<String> *r_functions, HashSet<int> *r_safe_lines) const override;

	ZelynEditorLanguage();
	~ZelynEditorLanguage();
};

#endif // TOOLS_ENABLED
