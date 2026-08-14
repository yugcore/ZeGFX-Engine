/**************************************************************************/
/*  terrain_3d_editor_plugin.h                                            */
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

#include "editor/plugins/editor_plugin.h"
#include "scene/3d/terrain_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

class Terrain3DEditorPlugin : public EditorPlugin {
	GDCLASS(Terrain3DEditorPlugin, EditorPlugin);

	enum MenuOption {
		MENU_REBUILD_TERRAIN,
		MENU_BAKE_COLLISION,
		MENU_TOGGLE_CHUNK_BOUNDS,
		MENU_TOGGLE_LOD_COLORS,
		MENU_TERRAIN_STATS,
	};

	Terrain3D *terrain = nullptr;

	HBoxContainer *toolbar_hb = nullptr;
	MenuButton *options_menu = nullptr;
	AcceptDialog *stats_dialog = nullptr;

	// Sculpting Toolbar Widgets
	Button *sculpt_btn = nullptr;
	OptionButton *brush_mode_opt = nullptr;

	HBoxContainer *size_hb = nullptr;
	SpinBox *brush_size_sb = nullptr;

	HBoxContainer *strength_hb = nullptr;
	SpinBox *brush_strength_sb = nullptr;

	HBoxContainer *target_h_hb = nullptr;
	SpinBox *brush_height_sb = nullptr;
	Button *pick_h_btn = nullptr;

	bool sculpt_active = false;
	bool is_sculpting = false;
	bool has_mouse_hit = false;
	Vector3 brush_pos;
	Vector2 mouse_screen_pos;
	Camera3D *last_camera = nullptr;
	Vector<float> initial_heights;

	void _menu_option(int p_option);
	void _on_sculpt_toggled(bool p_pressed);
	void _on_brush_mode_changed(int p_mode);
	void _on_pick_height_pressed();
	bool _raycast_terrain(Camera3D *p_camera, const Vector2 &p_mouse_pos, Vector3 &r_hit_pos);

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Terrain3D"; }
	bool has_main_screen() const override { return false; }
	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void make_visible(bool p_visible) override;

	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;
	virtual void forward_3d_force_draw_over_viewport(Control *p_overlay) override { forward_3d_draw_over_viewport(p_overlay); }

	Terrain3DEditorPlugin();
	~Terrain3DEditorPlugin();
};
