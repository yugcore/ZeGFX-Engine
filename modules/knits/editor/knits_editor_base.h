/**************************************************************************/
/*  knits_editor_base.h                                                   */
/**************************************************************************/

#pragma once

#ifdef TOOLS_ENABLED

#include "../knits_node.h"
#include "../knits_script.h"

#include "editor/script/script_editor_base.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/graph_frame.h"
#include "scene/gui/graph_node.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"
#include "scene/resources/style_box_flat.h"

class KnitsEditorBase : public ScriptEditorBase {
	GDCLASS(KnitsEditorBase, ScriptEditorBase);

private:
	Ref<KnitsScript> script;
	Ref<KnitsGraph> graph;

	HBoxContainer *toolbar = nullptr;
	Button *add_node_btn = nullptr;
	Button *add_frame_btn = nullptr;
	Button *compile_btn = nullptr;
	Label *status_label = nullptr;

	GraphEdit *graph_edit = nullptr;

	// Quick-Spawn Node Palette Dialog
	ConfirmationDialog *quick_spawn_dialog = nullptr;
	LineEdit *search_box = nullptr;
	Tree *palette_tree = nullptr;
	Vector2 spawn_position;

	HashMap<KnitNodeID, GraphNode *> visual_nodes;
	HashMap<GraphNode *, KnitNodeID> node_id_lookup;
	HashMap<KnitNodeID, Vector<KnitPinID>> input_slot_to_pin;
	HashMap<KnitNodeID, Vector<KnitPinID>> output_slot_to_pin;

	// Inline editing widgets on unconnected input pins
	HashMap<KnitPinID, Control *> pin_inline_widgets;
	HashMap<KnitPinID, KnitNodeID> pin_owner_lookup;
	HashMap<uint64_t, GraphFrame *> visual_frames;

	void _update_graph_view();
	void _create_visual_node(const Ref<KnitNode> &p_node);
	void _create_visual_frame(const KnitCommentBox &p_comment);

	Control *_create_pin_inline_widget(KnitNodeID p_node_id, const KnitPin &p_pin);
	void _update_pin_widget_visibility(KnitPinID p_pin_id);

	Color _get_category_color(KnitNodeCategory p_category) const;
	String _get_category_badge(KnitNodeCategory p_category) const;
	Color _get_pin_color(KnitPinKind p_kind, KnitDataType p_type) const;

	void _on_pin_bool_changed(bool p_val, KnitNodeID p_node_id, KnitPinID p_pin_id);
	void _on_pin_float_changed(double p_val, KnitNodeID p_node_id, KnitPinID p_pin_id);
	void _on_pin_string_changed(const String &p_val, KnitNodeID p_node_id, KnitPinID p_pin_id);
	void _on_pin_color_changed(const Color &p_val, KnitNodeID p_node_id, KnitPinID p_pin_id);

	void _on_add_node_pressed();
	void _on_add_frame_pressed();
	void _on_compile_pressed();

	void _on_connection_request(const StringName &p_from, int p_from_slot, const StringName &p_to, int p_to_slot);
	void _on_disconnection_request(const StringName &p_from, int p_from_slot, const StringName &p_to, int p_to_slot);
	void _on_delete_nodes_request(const TypedArray<StringName> &p_nodes);
	void _on_node_selected(Node *p_node);
	void _on_node_deselected(Node *p_node);
	void _on_popup_request(const Vector2 &p_position);

	void _on_palette_item_activated();
	void _populate_palette_tree(const String &p_filter = "");
	void _on_search_text_changed(const String &p_text);
	void _spawn_selected_palette_node();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	static ScriptEditorBase *create_editor(const Ref<Resource> &p_resource);

	virtual void set_toggle_list_control(Control *p_toggle_list_control) override {}
	virtual void update_toggle_files_button() override {}
	virtual void ensure_focus() override;

	virtual void set_edited_resource(const Ref<Resource> &p_res) override;
	virtual Ref<Resource> get_edited_resource() const override { return edited_res; }

	virtual Control *get_base_editor() const override { return (Control *)graph_edit; }
	virtual Ref<Texture2D> get_theme_icon() override;

	virtual void apply_code() override;
	virtual void validate_script() override;
	virtual bool is_unsaved() override;

	KnitsEditorBase();
	virtual ~KnitsEditorBase();
};

#endif // TOOLS_ENABLED
