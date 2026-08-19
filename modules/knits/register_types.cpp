/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#include "register_types.h"

#include "knits_language.h"
#include "knits_node.h"
#include "knits_resource_format.h"
#include "knits_script.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/knits_editor_base.h"
#include "editor/knits_editor_plugin.h"
#include "editor/script/script_editor_plugin.h"

static void _knits_editor_init() {
	ScriptEditor::register_create_script_editor_function(KnitsEditorBase::create_editor);
	EditorPlugins::add_by_type<KnitsEditorPlugin>();
}
#endif // TOOLS_ENABLED

static KnitsScriptLanguage *knits_language = nullptr;
static Ref<ResourceFormatLoaderKnits> resource_loader_knits;
static Ref<ResourceFormatSaverKnits> resource_saver_knits;

#ifdef TESTS_ENABLED
#include "tests/test_knits.h"
#endif

void initialize_knits_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(KnitNode);
		GDREGISTER_CLASS(KnitsGraph);
		GDREGISTER_CLASS(KnitsScript);

		knits_language = memnew(KnitsScriptLanguage);
		ScriptServer::register_language(knits_language);

		resource_loader_knits.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_knits);

		resource_saver_knits.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_knits);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorNode::add_init_callback(_knits_editor_init);
	}
#endif // TOOLS_ENABLED
}

void uninitialize_knits_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (knits_language) {
			ScriptServer::unregister_language(knits_language);
			memdelete(knits_language);
			knits_language = nullptr;
		}

		ResourceLoader::remove_resource_format_loader(resource_loader_knits);
		resource_loader_knits.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_knits);
		resource_saver_knits.unref();
	}
}
