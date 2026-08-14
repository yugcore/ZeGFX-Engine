/**************************************************************************/
/*  virtual_texture_editor_plugin.h                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "editor/inspector/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/resources/virtual_texture_2d.h"

class VirtualTextureDashboard : public VBoxContainer {
	GDCLASS(VirtualTextureDashboard, VBoxContainer);

	Ref<VirtualTexture2D> vtex;
	Label *info_label = nullptr;
	Button *diag_button = nullptr;
	Button *rebuild_button = nullptr;
	AcceptDialog *diag_dialog = nullptr;

	void _show_diagnostics();
	void _rebuild_cache();

protected:
	void _notification(int p_what);

public:
	void set_virtual_texture(const Ref<VirtualTexture2D> &p_vtex);
	VirtualTextureDashboard();
};

class EditorInspectorPluginVirtualTexture : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginVirtualTexture, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
};

class VirtualTextureEditorPlugin : public EditorPlugin {
	GDCLASS(VirtualTextureEditorPlugin, EditorPlugin);

	Ref<EditorInspectorPluginVirtualTexture> inspector_plugin;

public:
	virtual String get_plugin_name() const override { return "VirtualTexture2D"; }
	VirtualTextureEditorPlugin();
};
