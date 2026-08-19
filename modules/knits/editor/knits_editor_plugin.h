/**************************************************************************/
/*  knits_editor_plugin.h                                                 */
/**************************************************************************/

#pragma once

#ifdef TOOLS_ENABLED

#include "editor/plugins/editor_plugin.h"

class KnitsEditorPlugin : public EditorPlugin {
	GDCLASS(KnitsEditorPlugin, EditorPlugin);

public:
	virtual String get_plugin_name() const override { return "Knits"; }
	virtual bool handles(Object *p_object) const override;
	virtual void edit(Object *p_object) override;
	virtual void make_visible(bool p_visible) override;

	KnitsEditorPlugin();
	~KnitsEditorPlugin();
};

#endif // TOOLS_ENABLED
