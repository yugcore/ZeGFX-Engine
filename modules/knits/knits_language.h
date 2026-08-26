/**************************************************************************/
/*  knits_language.h                                                      */
/**************************************************************************/

#pragma once

#include "knits_vm.h"
#include "core/object/script_language.h"

class KnitsScriptLanguage : public ScriptLanguage {
	GDCLASS(KnitsScriptLanguage, ScriptLanguage);

private:
	static KnitsScriptLanguage *singleton;
	KnitsBytecodeVM shared_vm;

protected:
	static void _bind_methods();

public:
	_FORCE_INLINE_ static KnitsScriptLanguage *get_singleton() { return singleton; }

	virtual String get_name() const override { return "Knits"; }
	virtual void init() override;
	virtual String get_type() const override { return "KnitsScript"; }
	virtual String get_extension() const override { return "knit"; }
	virtual void finish() override;

	virtual Vector<String> get_reserved_words() const override { return Vector<String>(); }
	virtual bool is_control_flow_keyword(const String &p_string) const override { return false; }
	virtual Vector<String> get_comment_delimiters() const override { return Vector<String>(); }
	virtual Vector<String> get_doc_comment_delimiters() const override { return Vector<String>(); }
	virtual Vector<String> get_string_delimiters() const override { return Vector<String>(); }

	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	virtual Vector<ScriptTemplate> get_built_in_templates(const StringName &p_object) override { return Vector<ScriptTemplate>(); }
	virtual bool is_using_templates() override { return false; }

	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override { return ""; }
	virtual bool supports_builtin_mode() const override { return false; }
	virtual bool supports_documentation() const override { return true; }
	virtual bool can_inherit_from_file() const override { return false; }

	virtual bool handles_global_class_type(const String &p_type) const override { return p_type == "KnitsScript"; }
	virtual String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr, bool *r_is_abstract = nullptr, bool *r_is_tool = nullptr) const override { return String(); }

	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override {}

	/* DEBUGGER FUNCTIONS */
	virtual String debug_get_error() const override { return ""; }
	virtual int debug_get_stack_level_count() const override { return 0; }
	virtual int debug_get_stack_level_line(int p_level) const override { return 0; }
	virtual String debug_get_stack_level_function(int p_level) const override { return ""; }
	virtual String debug_get_stack_level_source(int p_level) const override { return ""; }
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) override { return ""; }

	virtual void reload_all_scripts() override {}
	virtual void reload_scripts(const Array &p_scripts) override {}
	virtual void reload_tool_script(const Ref<Script> &p_script) override {}

	/* LOADER FUNCTIONS */
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override {}
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override {}
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const override {}

	/* PROFILING FUNCTIONS */
	virtual void profiling_start() override {}
	virtual void profiling_stop() override {}
	virtual void profiling_set_save_native_calls(bool p_enable) override {}
	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }

	virtual void frame() override;

#ifdef TOOLS_ENABLED
	virtual EditorLanguage *get_editor_language() override { return nullptr; }
#endif

	KnitsBytecodeVM &get_shared_vm() { return shared_vm; }

	KnitsScriptLanguage();
	~KnitsScriptLanguage();
};
