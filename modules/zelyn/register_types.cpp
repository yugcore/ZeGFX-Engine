/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#include "register_types.h"

#include "runtime/zelyn_script_language.h"
#include "runtime/zelyn_script.h"
#include "runtime/zelyn_resource_format.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"

static ZelynScriptLanguage *zelyn_language = nullptr;
static Ref<ResourceFormatLoaderZelyn> resource_loader_zelyn;
static Ref<ResourceFormatSaverZelyn> resource_saver_zelyn;

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/zelyn_syntax_highlighter.h"
#include "editor/zelyn_bytecode_viewer.h"
#include "editor/zelyn_repl_dock.h"

class ZelynEditorPlugin : public EditorPlugin {
	GDCLASS(ZelynEditorPlugin, EditorPlugin);

	ZelynBytecodeViewer *bytecode_viewer = nullptr;
	ZelynREPLDock *repl_dock = nullptr;

protected:
	static void _bind_methods() {}

public:
	virtual String get_plugin_name() const override { return "Zelyn"; }

	ZelynEditorPlugin() {
		bytecode_viewer = memnew(ZelynBytecodeViewer);
		add_control_to_dock(DOCK_SLOT_RIGHT_UL, bytecode_viewer);

		repl_dock = memnew(ZelynREPLDock);
		add_control_to_bottom_panel(repl_dock, "Zelyn REPL");
	}

	~ZelynEditorPlugin() {
		if (bytecode_viewer) {
			remove_control_from_docks(bytecode_viewer);
			memdelete(bytecode_viewer);
		}
		if (repl_dock) {
			remove_control_from_bottom_panel(repl_dock);
			memdelete(repl_dock);
		}
	}
};

static void _zelyn_editor_init() {
	Ref<ZelynSyntaxHighlighter> highlighter;
	highlighter.instantiate();
	ScriptEditor::get_singleton()->register_syntax_highlighter(highlighter);

	EditorPlugins::add_by_type<ZelynEditorPlugin>();
}
#endif // TOOLS_ENABLED

void initialize_zelyn_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(ZelynScript);

		zelyn_language = memnew(ZelynScriptLanguage);
		ScriptServer::register_language(zelyn_language);

		resource_loader_zelyn.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_zelyn);

		resource_saver_zelyn.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_zelyn);
	}
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorNode::add_init_callback(_zelyn_editor_init);
	}
#endif // TOOLS_ENABLED
}

void uninitialize_zelyn_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (zelyn_language) {
			ScriptServer::unregister_language(zelyn_language);
			memdelete(zelyn_language);
			zelyn_language = nullptr;
		}

		ResourceLoader::remove_resource_format_loader(resource_loader_zelyn);
		resource_loader_zelyn.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_zelyn);
		resource_saver_zelyn.unref();
	}
}
