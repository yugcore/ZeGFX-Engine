/**************************************************************************/
/*  foliage_3d_editor_plugin.h                                            */
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
#include "scene/3d/foliage_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

class Foliage3DEditorPlugin : public EditorPlugin {
	GDCLASS(Foliage3DEditorPlugin, EditorPlugin);

public:
	enum BrushMode {
		MODE_PAINT,
		MODE_ERASE,
	};

	enum MenuOption {
		MENU_CONFORM_TO_TERRAIN,
		MENU_PROCEDURAL_SCATTER,
		MENU_CLEAR_ALL,
		MENU_FOLIAGE_STATS,
	};

private:
	Foliage3D *foliage = nullptr;

	HBoxContainer *toolbar_hb = nullptr;
	MenuButton *options_menu = nullptr;
	Button *paint_mode_btn = nullptr;
	OptionButton *brush_mode_opt = nullptr;

	HBoxContainer *size_hb = nullptr;
	SpinBox *brush_size_sb = nullptr;

	HBoxContainer *density_hb = nullptr;
	SpinBox *brush_density_sb = nullptr;

	HBoxContainer *erase_filter_hb = nullptr;
	OptionButton *erase_filter_opt = nullptr;

	AcceptDialog *stats_dialog = nullptr;

	// Palette Panel (Bottom Dock)
	PanelContainer *palette_panel = nullptr;
	ItemList *palette_list = nullptr;
	Button *palette_add_btn = nullptr;
	Button *palette_remove_btn = nullptr;
	Button *palette_select_all_btn = nullptr;
	Button *palette_deselect_all_btn = nullptr;
	Label *palette_summary_lbl = nullptr;

	// Brush State
	bool brush_active = false;
	bool is_stroking = false;
	bool has_mouse_hit = false;
	Vector3 brush_pos;
	Vector2 mouse_screen_pos;
	Camera3D *last_camera = nullptr;
	PackedFloat32Array initial_foliage_data;

	void _menu_option(int p_option);
	void _on_paint_toggled(bool p_pressed);
	void _on_brush_mode_changed(int p_mode);
	void _update_palette_ui();
	void _on_palette_item_selected(int p_index);
	void _on_palette_add_pressed();
	void _on_palette_remove_pressed();
	void _on_palette_select_all_pressed(bool p_select);
	bool _raycast_surface(Camera3D *p_camera, const Vector2 &p_mouse_pos, Vector3 &r_hit_pos);
	Vector<int> _get_active_type_filter() const;

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "Foliage3D"; }
	bool has_main_screen() const override { return false; }
	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void make_visible(bool p_visible) override;

	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;

	Foliage3DEditorPlugin();
	~Foliage3DEditorPlugin();
};
