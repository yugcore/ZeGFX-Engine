/**************************************************************************/
/*  knits_editor_plugin.cpp                                               */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "knits_editor_plugin.h"
#include "knits_editor_base.h"
#include "../knits_node.h"
#include "../knits_script.h"
#include "editor/script/script_editor_plugin.h"

bool KnitsEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<KnitsScript>(p_object) != nullptr || Object::cast_to<KnitsGraph>(p_object) != nullptr;
}

void KnitsEditorPlugin::edit(Object *p_object) {
	Ref<Resource> res = Object::cast_to<Resource>(p_object);
	if (res.is_valid()) {
		ScriptEditor::get_singleton()->edit(res);
	}
}

void KnitsEditorPlugin::make_visible(bool p_visible) {
}

KnitsEditorPlugin::KnitsEditorPlugin() {
	ScriptEditor::register_create_script_editor_function(KnitsEditorBase::create_editor);
}

KnitsEditorPlugin::~KnitsEditorPlugin() {
}

#endif // TOOLS_ENABLED
