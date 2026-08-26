/**************************************************************************/
/*  knits_editor_base.cpp                                                 */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "knits_editor_base.h"
#include "../knits_compiler.h"
#include "../knits_serializer.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"

void KnitsEditorBase::_bind_methods() {
}

ScriptEditorBase *KnitsEditorBase::create_editor(const Ref<Resource> &p_resource) {
	if (Object::cast_to<KnitsScript>(p_resource.ptr()) || Object::cast_to<KnitsGraph>(p_resource.ptr())) {
		return memnew(KnitsEditorBase);
	}
	return nullptr;
}

Color KnitsEditorBase::_get_category_color(KnitNodeCategory p_category) const {
	switch (p_category) {
		case KnitNodeCategory::Event:
			return Color(0.68f, 0.15f, 0.22f); // Ruby Crimson
		case KnitNodeCategory::ImpureAction:
			return Color(0.16f, 0.38f, 0.82f); // Velvet Blue
		case KnitNodeCategory::PureFunction:
			return Color(0.10f, 0.52f, 0.34f); // Emerald
		case KnitNodeCategory::FlowControl:
			return Color(0.26f, 0.32f, 0.42f); // Slate Navy
		case KnitNodeCategory::VariableGet:
		case KnitNodeCategory::VariableSet:
			return Color(0.10f, 0.48f, 0.54f); // Teal Cyan
		case KnitNodeCategory::SubGraph:
			return Color(0.50f, 0.22f, 0.72f); // Royal Purple
		case KnitNodeCategory::Reroute:
			return Color(0.35f, 0.35f, 0.35f);
		case KnitNodeCategory::Comment:
			return Color(0.22f, 0.26f, 0.32f);
		default:
			return Color(0.28f, 0.34f, 0.40f);
	}
}

String KnitsEditorBase::_get_category_badge(KnitNodeCategory p_category) const {
	switch (p_category) {
		case KnitNodeCategory::Event:
			return "[Event] ";
		case KnitNodeCategory::ImpureAction:
			return "[Action] ";
		case KnitNodeCategory::PureFunction:
			return "[Math] ";
		case KnitNodeCategory::FlowControl:
			return "[Flow] ";
		case KnitNodeCategory::VariableGet:
			return "[Get] ";
		case KnitNodeCategory::VariableSet:
			return "[Set] ";
		case KnitNodeCategory::SubGraph:
			return "[Macro] ";
		case KnitNodeCategory::Comment:
			return "[Frame] ";
		default:
			return "";
	}
}

Color KnitsEditorBase::_get_pin_color(KnitPinKind p_kind, KnitDataType p_type) const {
	if (p_kind == KnitPinKind::Execution) {
		return Color(1.0f, 1.0f, 1.0f); // White execution wire
	}
	switch (p_type) {
		case KnitDataType::Bool: return Color(0.95f, 0.35f, 0.35f);       // Coral Red
		case KnitDataType::Int32:
		case KnitDataType::Int64: return Color(0.25f, 0.85f, 0.60f);      // Sea Green
		case KnitDataType::Float:
		case KnitDataType::Double: return Color(0.35f, 0.80f, 1.0f);      // Pale Cyan
		case KnitDataType::String:
		case KnitDataType::StringName: return Color(0.90f, 0.45f, 0.90f);  // Magenta
		case KnitDataType::Vector2:
		case KnitDataType::Vector3: return Color(1.0f, 0.85f, 0.25f);     // Gold Yellow
		case KnitDataType::Color: return Color(0.95f, 0.70f, 0.95f);       // Lavender
		case KnitDataType::Transform3D: return Color(0.85f, 0.55f, 0.30f); // Orange
		case KnitDataType::ObjectRef: return Color(0.35f, 0.65f, 1.0f);   // Bright Blue
		case KnitDataType::Array: return Color(0.85f, 0.65f, 0.40f);       // Ochre
		case KnitDataType::Dictionary: return Color(0.40f, 0.85f, 0.85f);  // Turquoise
		default: return Color(0.8f, 0.8f, 0.8f);
	}
}

Control *KnitsEditorBase::_create_pin_inline_widget(KnitNodeID p_node_id, const KnitPin &p_pin) {
	if (p_pin.kind != KnitPinKind::Data) {
		return nullptr;
	}

	switch (p_pin.type.kind) {
		case KnitDataType::Bool: {
			CheckBox *cb = memnew(CheckBox);
			cb->set_pressed(p_pin.default_value.booleanize());
			cb->set_focus_mode(Control::FOCUS_CLICK);
			cb->connect("toggled", callable_mp(this, &KnitsEditorBase::_on_pin_bool_changed).bind(p_node_id, p_pin.id));
			return cb;
		}
		case KnitDataType::Float:
		case KnitDataType::Double: {
			SpinBox *sb = memnew(SpinBox);
			sb->set_step(0.01);
			sb->set_min(-999999.0);
			sb->set_max(999999.0);
			sb->set_allow_greater(true);
			sb->set_allow_lesser(true);
			sb->set_custom_minimum_size(Size2(64, 0));
			double val = (p_pin.default_value.get_type() == Variant::FLOAT || p_pin.default_value.get_type() == Variant::INT) ? (double)p_pin.default_value : 0.0;
			sb->set_value(val);
			sb->connect("value_changed", callable_mp(this, &KnitsEditorBase::_on_pin_float_changed).bind(p_node_id, p_pin.id));
			return sb;
		}
		case KnitDataType::Int32:
		case KnitDataType::Int64: {
			SpinBox *sb = memnew(SpinBox);
			sb->set_step(1.0);
			sb->set_min(-999999);
			sb->set_max(999999);
			sb->set_allow_greater(true);
			sb->set_allow_lesser(true);
			sb->set_custom_minimum_size(Size2(56, 0));
			int64_t val = (p_pin.default_value.get_type() == Variant::INT || p_pin.default_value.get_type() == Variant::FLOAT) ? (int64_t)p_pin.default_value : 0;
			sb->set_value((double)val);
			sb->connect("value_changed", callable_mp(this, &KnitsEditorBase::_on_pin_float_changed).bind(p_node_id, p_pin.id));
			return sb;
		}
		case KnitDataType::String:
		case KnitDataType::StringName:
		case KnitDataType::NodePath: {
			LineEdit *le = memnew(LineEdit);
			le->set_custom_minimum_size(Size2(72, 0));
			le->set_placeholder("value");
			String val = (p_pin.default_value.get_type() == Variant::STRING || p_pin.default_value.get_type() == Variant::STRING_NAME) ? String(p_pin.default_value) : "";
			le->set_text(val);
			le->connect("text_changed", callable_mp(this, &KnitsEditorBase::_on_pin_string_changed).bind(p_node_id, p_pin.id));
			return le;
		}
		case KnitDataType::Color: {
			ColorPickerButton *cpb = memnew(ColorPickerButton);
			cpb->set_custom_minimum_size(Size2(28, 20));
			Color col = p_pin.default_value.get_type() == Variant::COLOR ? (Color)p_pin.default_value : Color(1, 1, 1, 1);
			cpb->set_pick_color(col);
			cpb->connect("color_changed", callable_mp(this, &KnitsEditorBase::_on_pin_color_changed).bind(p_node_id, p_pin.id));
			return cpb;
		}
		default:
			return nullptr;
	}
}

void KnitsEditorBase::_on_pin_bool_changed(bool p_val, KnitNodeID p_node_id, KnitPinID p_pin_id) {
	if (graph.is_null() || !graph->nodes.has(p_node_id)) return;
	Ref<KnitNode> node = graph->nodes[p_node_id];
	if (node.is_null()) return;
	KnitPin *pin = node->find_pin(p_pin_id);
	if (pin) {
		pin->default_value = p_val;
		apply_code();
	}
}

void KnitsEditorBase::_on_pin_float_changed(double p_val, KnitNodeID p_node_id, KnitPinID p_pin_id) {
	if (graph.is_null() || !graph->nodes.has(p_node_id)) return;
	Ref<KnitNode> node = graph->nodes[p_node_id];
	if (node.is_null()) return;
	KnitPin *pin = node->find_pin(p_pin_id);
	if (pin) {
		if (pin->type.kind == KnitDataType::Int32 || pin->type.kind == KnitDataType::Int64) {
			pin->default_value = (int64_t)p_val;
		} else {
			pin->default_value = p_val;
		}
		apply_code();
	}
}

void KnitsEditorBase::_on_pin_string_changed(const String &p_val, KnitNodeID p_node_id, KnitPinID p_pin_id) {
	if (graph.is_null() || !graph->nodes.has(p_node_id)) return;
	Ref<KnitNode> node = graph->nodes[p_node_id];
	if (node.is_null()) return;
	KnitPin *pin = node->find_pin(p_pin_id);
	if (pin) {
		if (pin->type.kind == KnitDataType::StringName) {
			pin->default_value = StringName(p_val);
		} else if (pin->type.kind == KnitDataType::NodePath) {
			pin->default_value = NodePath(p_val);
		} else {
			pin->default_value = p_val;
		}
		apply_code();
	}
}

void KnitsEditorBase::_on_pin_color_changed(const Color &p_val, KnitNodeID p_node_id, KnitPinID p_pin_id) {
	if (graph.is_null() || !graph->nodes.has(p_node_id)) return;
	Ref<KnitNode> node = graph->nodes[p_node_id];
	if (node.is_null()) return;
	KnitPin *pin = node->find_pin(p_pin_id);
	if (pin) {
		pin->default_value = p_val;
		apply_code();
	}
}

void KnitsEditorBase::_update_pin_widget_visibility(KnitPinID p_pin_id) {
	if (!pin_inline_widgets.has(p_pin_id)) return;
	Control *widget = pin_inline_widgets[p_pin_id];
	if (!widget) return;
	bool is_connected = (graph.is_valid() && graph->get_connection_for_input_pin(p_pin_id) != nullptr);
	widget->set_visible(!is_connected);
}

void KnitsEditorBase::_create_visual_frame(const KnitCommentBox &p_comment) {
	GraphFrame *frame = memnew(GraphFrame);
	frame->set_name("frame_" + itos((uint64_t)p_comment.id));
	frame->set_title(p_comment.title);
	frame->set_position_offset(p_comment.bounds.position);
	frame->set_size(p_comment.bounds.size);
	frame->set_tint_color_enabled(true);
	frame->set_tint_color(p_comment.color);
	frame->set_resizable(true);

	Ref<StyleBoxFlat> frame_sb = memnew(StyleBoxFlat);
	frame_sb->set_bg_color(p_comment.color);
	frame_sb->set_border_width_all(2);
	frame_sb->set_border_color(p_comment.color.lightened(0.3f));
	frame_sb->set_corner_radius_all(8);
	frame->add_theme_style_override("panel", frame_sb);

	visual_frames[p_comment.id] = frame;
	graph_edit->add_child(frame);
}

void KnitsEditorBase::_create_visual_node(const Ref<KnitNode> &p_node) {
	if (p_node.is_null()) return;

	GraphNode *gn = memnew(GraphNode);
	gn->set_name("node_" + itos((uint64_t)p_node->id));
	gn->set_title(_get_category_badge(p_node->category) + p_node->title);
	gn->set_position_offset(p_node->position);
	gn->set_resizable(false);

	Color cat_col = _get_category_color(p_node->category);

	// Custom Category Theming StyleBoxes
	Ref<StyleBoxFlat> titlebar_sb = memnew(StyleBoxFlat);
	titlebar_sb->set_bg_color(cat_col);
	titlebar_sb->set_corner_radius(CORNER_TOP_LEFT, 6);
	titlebar_sb->set_corner_radius(CORNER_TOP_RIGHT, 6);
	titlebar_sb->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	titlebar_sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	titlebar_sb->set_content_margin_all(6);
	gn->add_theme_style_override("titlebar", titlebar_sb);

	Ref<StyleBoxFlat> titlebar_sel_sb = memnew(StyleBoxFlat);
	titlebar_sel_sb->set_bg_color(cat_col.lightened(0.15f));
	titlebar_sel_sb->set_corner_radius(CORNER_TOP_LEFT, 6);
	titlebar_sel_sb->set_corner_radius(CORNER_TOP_RIGHT, 6);
	titlebar_sel_sb->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	titlebar_sel_sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	titlebar_sel_sb->set_border_width(SIDE_TOP, 2);
	titlebar_sel_sb->set_border_width(SIDE_LEFT, 2);
	titlebar_sel_sb->set_border_width(SIDE_RIGHT, 2);
	titlebar_sel_sb->set_border_color(Color(1.0f, 0.85f, 0.35f));
	titlebar_sel_sb->set_content_margin_all(6);
	gn->add_theme_style_override("titlebar_selected", titlebar_sel_sb);

	Ref<StyleBoxFlat> panel_sb = memnew(StyleBoxFlat);
	panel_sb->set_bg_color(Color(0.09f, 0.10f, 0.13f, 0.94f));
	panel_sb->set_border_width_all(1);
	panel_sb->set_border_color(cat_col.darkened(0.2f));
	panel_sb->set_corner_radius(CORNER_TOP_LEFT, 0);
	panel_sb->set_corner_radius(CORNER_TOP_RIGHT, 0);
	panel_sb->set_corner_radius(CORNER_BOTTOM_LEFT, 6);
	panel_sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 6);
	panel_sb->set_shadow_color(Color(0.0f, 0.0f, 0.0f, 0.45f));
	panel_sb->set_shadow_size(6);
	panel_sb->set_shadow_offset(Vector2(0, 3));
	panel_sb->set_content_margin_all(6);
	gn->add_theme_style_override("panel", panel_sb);

	Ref<StyleBoxFlat> panel_sel_sb = memnew(StyleBoxFlat);
	panel_sel_sb->set_bg_color(Color(0.11f, 0.12f, 0.16f, 0.96f));
	panel_sel_sb->set_border_width_all(2);
	panel_sel_sb->set_border_color(Color(1.0f, 0.85f, 0.35f));
	panel_sel_sb->set_corner_radius(CORNER_TOP_LEFT, 0);
	panel_sel_sb->set_corner_radius(CORNER_TOP_RIGHT, 0);
	panel_sel_sb->set_corner_radius(CORNER_BOTTOM_LEFT, 6);
	panel_sel_sb->set_corner_radius(CORNER_BOTTOM_RIGHT, 6);
	panel_sel_sb->set_shadow_color(Color(0.0f, 0.0f, 0.0f, 0.6f));
	panel_sel_sb->set_shadow_size(10);
	panel_sel_sb->set_shadow_offset(Vector2(0, 4));
	panel_sel_sb->set_content_margin_all(6);
	gn->add_theme_style_override("panel_selected", panel_sel_sb);

	visual_nodes[p_node->id] = gn;
	node_id_lookup[gn] = p_node->id;

	Vector<KnitPinID> in_pins;
	Vector<KnitPinID> out_pins;

	int max_slots = MAX(p_node->input_pins.size(), p_node->output_pins.size());
	if (max_slots == 0) max_slots = 1;

	for (int i = 0; i < max_slots; i++) {
		HBoxContainer *slot_row = memnew(HBoxContainer);
		slot_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		slot_row->set_custom_minimum_size(Size2(0, 24));
		gn->add_child(slot_row);

		bool has_left = i < p_node->input_pins.size();
		bool has_right = i < p_node->output_pins.size();

		int left_type = 0;
		Color left_col = Color(1, 1, 1);
		if (has_left) {
			const KnitPin &pin = p_node->input_pins[i];
			left_type = (pin.kind == KnitPinKind::Execution) ? 0 : (int)pin.type.kind;
			left_col = _get_pin_color(pin.kind, pin.type.kind);

			HBoxContainer *left_box = memnew(HBoxContainer);
			left_box->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
			slot_row->add_child(left_box);

			Label *slot_label = memnew(Label);
			slot_label->set_text(pin.display_label);
			slot_label->add_theme_color_override("font_color", left_col.lerp(Color(1, 1, 1), 0.35f));
			left_box->add_child(slot_label);

			if (pin.kind == KnitPinKind::Data) {
				Control *widget = _create_pin_inline_widget(p_node->id, pin);
				if (widget) {
					left_box->add_child(widget);
					pin_inline_widgets[pin.id] = widget;
					pin_owner_lookup[pin.id] = p_node->id;
					bool is_conn = (graph.is_valid() && graph->get_connection_for_input_pin(pin.id) != nullptr);
					widget->set_visible(!is_conn);
				}
			}

			in_pins.push_back(pin.id);
		}

		// Center spacer
		Control *spacer = memnew(Control);
		spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		spacer->set_custom_minimum_size(Size2(16, 0));
		slot_row->add_child(spacer);

		int right_type = 0;
		Color right_col = Color(1, 1, 1);
		if (has_right) {
			const KnitPin &pin = p_node->output_pins[i];
			right_type = (pin.kind == KnitPinKind::Execution) ? 0 : (int)pin.type.kind;
			right_col = _get_pin_color(pin.kind, pin.type.kind);

			HBoxContainer *right_box = memnew(HBoxContainer);
			right_box->set_alignment(BoxContainer::ALIGNMENT_END);
			slot_row->add_child(right_box);

			Label *slot_label = memnew(Label);
			slot_label->set_text(pin.display_label);
			slot_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			slot_label->add_theme_color_override("font_color", right_col.lerp(Color(1, 1, 1), 0.35f));
			right_box->add_child(slot_label);

			out_pins.push_back(pin.id);
		}

		gn->set_slot(i, has_left, left_type, left_col, has_right, right_type, right_col);
	}

	input_slot_to_pin[p_node->id] = in_pins;
	output_slot_to_pin[p_node->id] = out_pins;

	graph_edit->add_child(gn);
}

void KnitsEditorBase::_update_graph_view() {
	if (graph_edit == nullptr) return;

	graph_edit->clear_connections();

	for (KeyValue<KnitNodeID, GraphNode *> &E : visual_nodes) {
		if (E.value && E.value->get_parent() == graph_edit) {
			graph_edit->remove_child(E.value);
			memdelete(E.value);
		}
	}
	visual_nodes.clear();
	node_id_lookup.clear();
	input_slot_to_pin.clear();
	output_slot_to_pin.clear();

	for (KeyValue<uint64_t, GraphFrame *> &E : visual_frames) {
		if (E.value && E.value->get_parent() == graph_edit) {
			graph_edit->remove_child(E.value);
			memdelete(E.value);
		}
	}
	visual_frames.clear();
	pin_inline_widgets.clear();
	pin_owner_lookup.clear();

	if (graph.is_null()) return;

	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : graph->nodes) {
		_create_visual_node(E.value);
	}

	for (int i = 0; i < graph->comments.size(); i++) {
		_create_visual_frame(graph->comments[i]);
	}

	for (int i = 0; i < graph->connections.size(); i++) {
		const KnitConnection &c = graph->connections[i];
		if (!visual_nodes.has(c.from_node) || !visual_nodes.has(c.to_node)) continue;

		GraphNode *from_gn = visual_nodes[c.from_node];
		GraphNode *to_gn = visual_nodes[c.to_node];

		int from_slot = -1;
		int to_slot = -1;

		if (output_slot_to_pin.has(c.from_node)) {
			const Vector<KnitPinID> &pins = output_slot_to_pin[c.from_node];
			for (int s = 0; s < pins.size(); s++) {
				if (pins[s] == c.from_pin) { from_slot = s; break; }
			}
		}
		if (input_slot_to_pin.has(c.to_node)) {
			const Vector<KnitPinID> &pins = input_slot_to_pin[c.to_node];
			for (int s = 0; s < pins.size(); s++) {
				if (pins[s] == c.to_pin) { to_slot = s; break; }
			}
		}

		if (from_slot != -1 && to_slot != -1) {
			graph_edit->connect_node(from_gn->get_name(), from_slot, to_gn->get_name(), to_slot);
		}
	}

	// Update inline widget visibility
	for (const KeyValue<KnitPinID, Control *> &E : pin_inline_widgets) {
		_update_pin_widget_visibility(E.key);
	}
}

void KnitsEditorBase::_on_connection_request(const StringName &p_from, int p_from_slot, const StringName &p_to, int p_to_slot) {
	GraphNode *from_gn = Object::cast_to<GraphNode>(graph_edit->get_node_or_null(NodePath(p_from)));
	GraphNode *to_gn = Object::cast_to<GraphNode>(graph_edit->get_node_or_null(NodePath(p_to)));

	if (!from_gn || !to_gn) return;

	KnitNodeID from_id = node_id_lookup[from_gn];
	KnitNodeID to_id = node_id_lookup[to_gn];

	if (!output_slot_to_pin.has(from_id) || !input_slot_to_pin.has(to_id)) return;
	if (p_from_slot >= output_slot_to_pin[from_id].size() || p_to_slot >= input_slot_to_pin[to_id].size()) return;

	KnitPinID from_pin = output_slot_to_pin[from_id][p_from_slot];
	KnitPinID to_pin = input_slot_to_pin[to_id][p_to_slot];

	KnitConnectionID cid = graph->connect_pins(from_id, from_pin, to_id, to_pin);
	if (cid != 0) {
		graph_edit->connect_node(p_from, p_from_slot, p_to, p_to_slot);
		_update_pin_widget_visibility(to_pin);
		apply_code();
		status_label->set_text("Connected pin!");
	} else {
		status_label->set_text("Invalid connection rejected.");
	}
}

void KnitsEditorBase::_on_disconnection_request(const StringName &p_from, int p_from_slot, const StringName &p_to, int p_to_slot) {
	GraphNode *from_gn = Object::cast_to<GraphNode>(graph_edit->get_node_or_null(NodePath(p_from)));
	GraphNode *to_gn = Object::cast_to<GraphNode>(graph_edit->get_node_or_null(NodePath(p_to)));

	if (!from_gn || !to_gn) return;

	KnitNodeID from_id = node_id_lookup[from_gn];
	KnitNodeID to_id = node_id_lookup[to_gn];

	if (!output_slot_to_pin.has(from_id) || !input_slot_to_pin.has(to_id)) return;
	if (p_from_slot >= output_slot_to_pin[from_id].size() || p_to_slot >= input_slot_to_pin[to_id].size()) return;

	KnitPinID from_pin = output_slot_to_pin[from_id][p_from_slot];
	KnitPinID to_pin = input_slot_to_pin[to_id][p_to_slot];

	graph->disconnect_pins(from_pin, to_pin);
	graph_edit->disconnect_node(p_from, p_from_slot, p_to, p_to_slot);
	_update_pin_widget_visibility(to_pin);
	apply_code();
	status_label->set_text("Disconnected pin.");
}

void KnitsEditorBase::_on_delete_nodes_request(const TypedArray<StringName> &p_nodes) {
	for (int i = 0; i < p_nodes.size(); i++) {
		GraphNode *gn = Object::cast_to<GraphNode>(graph_edit->get_node_or_null(NodePath(String(p_nodes[i]))));
		if (gn && node_id_lookup.has(gn)) {
			KnitNodeID nid = node_id_lookup[gn];
			graph->remove_node(nid);
		}
	}
	_update_graph_view();
	apply_code();
}

void KnitsEditorBase::_on_node_selected(Node *p_node) {
}

void KnitsEditorBase::_on_node_deselected(Node *p_node) {
}

void KnitsEditorBase::_on_popup_request(const Vector2 &p_position) {
	spawn_position = p_position;
	search_box->clear();
	_populate_palette_tree();
	quick_spawn_dialog->popup_centered(Size2(480, 520));
	search_box->grab_focus();
}

void KnitsEditorBase::_on_add_node_pressed() {
	_on_popup_request(Vector2(200, 200));
}

void KnitsEditorBase::_on_add_frame_pressed() {
	if (graph.is_null()) return;
	Rect2 bounds(Vector2(200, 200), Size2(320, 200));
	graph->add_comment_box("Comment Frame", bounds, Color(0.18f, 0.22f, 0.28f, 0.45f));
	_update_graph_view();
	apply_code();
	status_label->set_text("Added Comment Frame.");
}

void KnitsEditorBase::_populate_palette_tree(const String &p_filter) {
	palette_tree->clear();
	TreeItem *root = palette_tree->create_item();

	auto add_category = [&](const String &p_name) -> TreeItem * {
		TreeItem *cat = palette_tree->create_item(root);
		cat->set_text(0, p_name);
		return cat;
	};

	auto add_item = [&](TreeItem *p_parent, const String &p_name, const String &p_desc, const String &p_tag) {
		if (!p_filter.is_empty() && p_name.findn(p_filter) == -1 && p_desc.findn(p_filter) == -1) return;
		TreeItem *item = palette_tree->create_item(p_parent);
		item->set_text(0, p_name);
		item->set_metadata(0, p_tag);
	};

	TreeItem *cat_events = add_category("Events & Lifecycle");
	add_item(cat_events, "Event: _ready", "Called when node enters scene tree", "event_ready");
	add_item(cat_events, "Event: _process", "Called every render frame", "event_process");
	add_item(cat_events, "Event: _physics_process", "Called every physics frame tick", "event_physics_process");
	add_item(cat_events, "Event: _input", "Called on unhandled viewport input", "event_input");
	add_item(cat_events, "Event: _gui_input", "Called on GUI control input", "event_gui_input");
	add_item(cat_events, "Event: _draw", "Called on 2D canvas item redraw", "event_draw");
	add_item(cat_events, "Custom Signal Event", "Listens to a custom signal", "event_custom_signal");

	TreeItem *cat_flow = add_category("Flow Control");
	add_item(cat_flow, "Branch (If / Else)", "Branches execution based on condition", "flow_branch");
	add_item(cat_flow, "While Loop", "Executes loop body while condition is true", "flow_while");
	add_item(cat_flow, "For Each Loop", "Iterates over elements in array or range", "flow_foreach");
	add_item(cat_flow, "Delay (Seconds)", "Suspends coroutine execution for duration", "flow_yield_sec");
	add_item(cat_flow, "Delay (Frames)", "Suspends coroutine for N frame ticks", "flow_yield_frame");
	add_item(cat_flow, "Return", "Returns from script function with value", "flow_return");

	TreeItem *cat_ops = add_category("Operators & Arithmetic");
	add_item(cat_ops, "Add (+)", "Adds two numbers or vectors", "math_add");
	add_item(cat_ops, "Subtract (-)", "Subtracts value B from A", "math_sub");
	add_item(cat_ops, "Multiply (*)", "Multiplies two numbers or scales vector", "math_mul");
	add_item(cat_ops, "Divide (/)", "Divides value A by B with zero check", "math_div");
	add_item(cat_ops, "Modulo (%)", "Computes remainder of division", "math_mod");
	add_item(cat_ops, "Power (**)", "Raises base to power exponent", "math_pow");
	add_item(cat_ops, "Negate (-)", "Inverts sign of number", "math_neg");

	TreeItem *cat_bitwise = add_category("Bitwise");
	add_item(cat_bitwise, "Bitwise AND (&)", "Bitwise AND on integers", "bit_and");
	add_item(cat_bitwise, "Bitwise OR (|)", "Bitwise OR on integers", "bit_or");
	add_item(cat_bitwise, "Bitwise XOR (^)", "Bitwise XOR on integers", "bit_xor");
	add_item(cat_bitwise, "Bitwise NOT (~)", "Bitwise NOT on integer", "bit_not");
	add_item(cat_bitwise, "Bit Shift Left (<<)", "Shifts bits left", "bit_shl");
	add_item(cat_bitwise, "Bit Shift Right (>>)", "Shifts bits right", "bit_shr");

	TreeItem *cat_cmp = add_category("Comparisons & Logic");
	add_item(cat_cmp, "Equal (==)", "Tests equality of two values", "cmp_eq");
	add_item(cat_cmp, "Not Equal (!=)", "Tests inequality of two values", "cmp_ne");
	add_item(cat_cmp, "Less Than (<)", "Tests if A is strictly less than B", "cmp_lt");
	add_item(cat_cmp, "Less Equal (<=)", "Tests if A is less than or equal to B", "cmp_le");
	add_item(cat_cmp, "Greater Than (>)", "Tests if A is strictly greater than B", "cmp_gt");
	add_item(cat_cmp, "Greater Equal (>=)", "Tests if A is greater than or equal to B", "cmp_ge");
	add_item(cat_cmp, "Logical NOT (!)", "Inverts boolean value", "logic_not");
	add_item(cat_cmp, "Logical AND (&&)", "Logical AND between two booleans", "logic_and");
	add_item(cat_cmp, "Logical OR (||)", "Logical OR between two booleans", "logic_or");

	TreeItem *cat_math = add_category("Math & Trigonometry");
	add_item(cat_math, "Sin", "Sine of angle in radians", "math_sin");
	add_item(cat_math, "Cos", "Cosine of angle in radians", "math_cos");
	add_item(cat_math, "Tan", "Tangent of angle in radians", "math_tan");
	add_item(cat_math, "ASin", "Arc sine in radians", "math_asin");
	add_item(cat_math, "ACos", "Arc cosine in radians", "math_acos");
	add_item(cat_math, "ATan2", "Four-quadrant arc tangent", "math_atan2");
	add_item(cat_math, "Deg To Rad", "Converts degrees to radians", "math_deg2rad");
	add_item(cat_math, "Rad To Deg", "Converts radians to degrees", "math_rad2deg");
	add_item(cat_math, "Sqrt", "Square root of positive number", "math_sqrt");
	add_item(cat_math, "Abs", "Absolute value", "math_abs");
	add_item(cat_math, "Sign", "Sign of number (-1, 0, 1)", "math_sign");
	add_item(cat_math, "Floor", "Rounds down to nearest integer", "math_floor");
	add_item(cat_math, "Ceil", "Rounds up to nearest integer", "math_ceil");
	add_item(cat_math, "Round", "Rounds to nearest integer", "math_round");
	add_item(cat_math, "Clamp", "Clamps value between min and max", "math_clamp");
	add_item(cat_math, "Min", "Minimum of two values", "math_min");
	add_item(cat_math, "Max", "Maximum of two values", "math_max");
	add_item(cat_math, "Lerp", "Linear interpolation between from and to", "math_lerp");
	add_item(cat_math, "Remap", "Remaps value from input range to output range", "math_remap");
	add_item(cat_math, "SmoothStep", "Hermite smoothstep interpolation", "math_smoothstep");

	TreeItem *cat_rng = add_category("Random Numbers");
	add_item(cat_rng, "Random Int (randi)", "Random unsigned integer", "rng_randi");
	add_item(cat_rng, "Random Float (randf)", "Random float between 0.0 and 1.0", "rng_randf");
	add_item(cat_rng, "Random Int Range", "Random integer between min and max", "rng_randi_range");
	add_item(cat_rng, "Random Float Range", "Random float between min and max", "rng_randf_range");
	add_item(cat_rng, "Randomize", "Re-seeds global random number generator", "rng_randomize");

	TreeItem *cat_containers = add_category("Containers & Strings");
	add_item(cat_containers, "Construct Array", "Creates an array from inputs", "cont_array_create");
	add_item(cat_containers, "Array Append", "Appends item to array", "cont_array_append");
	add_item(cat_containers, "Array Size (len)", "Gets number of elements in array", "cont_array_size");
	add_item(cat_containers, "Construct Dictionary", "Creates a key-value dictionary", "cont_dict_create");
	add_item(cat_containers, "Dict Get", "Gets value by key from dictionary", "cont_dict_get");
	add_item(cat_containers, "Dict Set", "Sets key-value pair in dictionary", "cont_dict_set");
	add_item(cat_containers, "Dict Has", "Checks if key exists in dictionary", "cont_dict_has");
	add_item(cat_containers, "String Format (str)", "Converts inputs to formatted string", "util_str");
	add_item(cat_containers, "Range Generator", "Generates integer sequence array", "util_range");
	add_item(cat_containers, "Load Resource", "Loads resource from path", "util_load");

	TreeItem *cat_signals = add_category("Signals & Callables");
	add_item(cat_signals, "Emit Signal", "Broadcasts signal with parameters", "signal_emit");
	add_item(cat_signals, "Connect Signal", "Subscribes callable to signal", "signal_connect");
	add_item(cat_signals, "Call Callable", "Invokes a callable dynamically", "callable_call");

	TreeItem *cat_type = add_category("Types & Reflection");
	add_item(cat_type, "Type Of", "Returns integer type ID of variant", "type_typeof");
	add_item(cat_type, "Is Instance Valid", "Checks if object reference is alive", "type_is_valid");
	add_item(cat_type, "Type Test (Is)", "Tests if instance matches class or type", "type_test");

	TreeItem *cat_actions = add_category("Actions & Debugging");
	add_item(cat_actions, "Print Message", "Prints value to console", "action_print");
	add_item(cat_actions, "Print Rich", "Prints BBCode formatted text", "action_print_rich");
	add_item(cat_actions, "Print Error", "Prints error highlighted text", "action_printerr");
	add_item(cat_actions, "Push Error", "Pushes engine debugger error", "action_push_error");
	add_item(cat_actions, "Push Warning", "Pushes engine debugger warning", "action_push_warning");
	add_item(cat_actions, "Assert", "Asserts condition is true in debug builds", "action_assert");
	add_item(cat_actions, "Move and Slide", "Performs character body movement with collision", "action_move_and_slide");
}

void KnitsEditorBase::_on_compile_pressed() {
	apply_code();
	validate_script();
}

void KnitsEditorBase::_on_search_text_changed(const String &p_text) {
	_populate_palette_tree(p_text);
}

void KnitsEditorBase::_on_palette_item_activated() {
	_spawn_selected_palette_node();
	quick_spawn_dialog->hide();
}

void KnitsEditorBase::_spawn_selected_palette_node() {
	TreeItem *selected = palette_tree->get_selected();
	if (!selected || selected->get_metadata(0).get_type() == Variant::NIL) return;

	String tag = selected->get_metadata(0);
	if (tag.is_empty() || graph.is_null()) return;

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;

	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	KnitTypeSignature sig_int;
	sig_int.kind = KnitDataType::Int64;

	KnitTypeSignature sig_bool;
	sig_bool.kind = KnitDataType::Bool;

	KnitTypeSignature sig_string;
	sig_string.kind = KnitDataType::String;

	KnitTypeSignature sig_array;
	sig_array.kind = KnitDataType::Array;

	KnitTypeSignature sig_dict;
	sig_dict.kind = KnitDataType::Dictionary;

	KnitTypeSignature sig_obj;
	sig_obj.kind = KnitDataType::ObjectRef;

	KnitTypeSignature sig_callable;
	sig_callable.kind = KnitDataType::Callable;

	KnitTypeSignature sig_wild;
	sig_wild.kind = KnitDataType::Wildcard;

	Ref<KnitNode> new_node;

	// Events
	if (tag == "event_ready") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_ready", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "event_process") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_process", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("delta", KnitPinKind::Data, sig_float);
	} else if (tag == "event_physics_process") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_physics_process", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("delta", KnitPinKind::Data, sig_float);
	} else if (tag == "event_input") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_input", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("event", KnitPinKind::Data, sig_obj);
	} else if (tag == "event_gui_input") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_gui_input", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("event", KnitPinKind::Data, sig_obj);
	} else if (tag == "event_draw") {
		new_node = graph->create_node(KnitNodeCategory::Event, "_draw", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "event_custom_signal") {
		new_node = graph->create_node(KnitNodeCategory::Event, "custom_signal", spawn_position);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// Flow Control
	} else if (tag == "flow_branch") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Branch", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Condition", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("True", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("False", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_while") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "While", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Condition", KnitPinKind::Data, sig_bool, true);
		new_node->add_output_pin("LoopBody", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Completed", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_foreach") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "ForEach", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Collection", KnitPinKind::Data, sig_array);
		new_node->add_output_pin("LoopBody", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Element", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("Completed", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_yield_sec") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "yield_seconds", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("seconds", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_yield_frame") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "yield_frames", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("frames", KnitPinKind::Data, sig_int, 1);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_return") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_float, 0.0);

	// Operators & Math
	} else if (tag == "math_add") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "add", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_sub") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "sub", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_mul") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "mul", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_div") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "div", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_mod") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "mod", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_pow") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "pow", spawn_position);
		new_node->add_input_pin("base", KnitPinKind::Data, sig_float, 2.0);
		new_node->add_input_pin("exp", KnitPinKind::Data, sig_float, 2.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_neg") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "neg", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Bitwise
	} else if (tag == "bit_and") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_and", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_int, 0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_int, 0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "bit_or") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_or", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_int, 0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_int, 0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "bit_xor") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_xor", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_int, 0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_int, 0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "bit_not") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_not", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_int, 0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "bit_shl") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_shl", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_int, 1);
		new_node->add_input_pin("shift", KnitPinKind::Data, sig_int, 1);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "bit_shr") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_shr", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_int, 1);
		new_node->add_input_pin("shift", KnitPinKind::Data, sig_int, 1);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);

	// Comparisons
	} else if (tag == "cmp_eq") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_eq", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "cmp_ne") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_ne", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "cmp_lt") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_lt", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "cmp_le") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_le", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "cmp_gt") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_gt", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "cmp_ge") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "cmp_ge", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "logic_not") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "not", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "logic_and") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "and", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_bool, true);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_bool, true);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);
	} else if (tag == "logic_or") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "or", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_bool, false);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_bool);

	// Math Functions
	} else if (tag == "math_sin" || tag == "math_cos" || tag == "math_tan" || tag == "math_asin" || tag == "math_acos" || tag == "math_sqrt" || tag == "math_abs" || tag == "math_sign" || tag == "math_floor" || tag == "math_ceil" || tag == "math_round" || tag == "math_deg2rad" || tag == "math_rad2deg") {
		String fn = tag.substr(5);
		new_node = graph->create_node(KnitNodeCategory::PureFunction, fn, spawn_position);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_atan2") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "atan2", spawn_position);
		new_node->add_input_pin("y", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_clamp") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "clamp", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("min", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("max", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_min" || tag == "math_max") {
		String fn = tag.substr(5);
		new_node = graph->create_node(KnitNodeCategory::PureFunction, fn, spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_lerp") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "lerp", spawn_position);
		new_node->add_input_pin("from", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("to", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("weight", KnitPinKind::Data, sig_float, 0.5);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_remap") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "remap", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_float, 0.5);
		new_node->add_input_pin("istart", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("istop", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("ostart", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("ostop", KnitPinKind::Data, sig_float, 100.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_smoothstep") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "smoothstep", spawn_position);
		new_node->add_input_pin("from", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("to", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.5);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Random Numbers
	} else if (tag == "rng_randi") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "randi", spawn_position);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "rng_randf") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "randf", spawn_position);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "rng_randi_range") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "randi_range", spawn_position);
		new_node->add_input_pin("from", KnitPinKind::Data, sig_int, 0);
		new_node->add_input_pin("to", KnitPinKind::Data, sig_int, 100);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_int);
	} else if (tag == "rng_randf_range") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "randf_range", spawn_position);
		new_node->add_input_pin("from", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("to", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "rng_randomize") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "randomize", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// Containers & Strings
	} else if (tag == "cont_array_create") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "array_construct", spawn_position);
		new_node->add_input_pin("elem0", KnitPinKind::Data, sig_wild);
		new_node->add_input_pin("elem1", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("array", KnitPinKind::Data, sig_array);
	} else if (tag == "cont_array_append") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "array_append", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("array", KnitPinKind::Data, sig_array);
		new_node->add_input_pin("item", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "cont_array_size") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "array_size", spawn_position);
		new_node->add_input_pin("array", KnitPinKind::Data, sig_array);
		new_node->add_output_pin("size", KnitPinKind::Data, sig_int);
	} else if (tag == "cont_dict_create") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "dict_construct", spawn_position);
		new_node->add_input_pin("key0", KnitPinKind::Data, sig_string, "key");
		new_node->add_input_pin("val0", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("dict", KnitPinKind::Data, sig_dict);
	} else if (tag == "cont_dict_get") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "dict_get", spawn_position);
		new_node->add_input_pin("dict", KnitPinKind::Data, sig_dict);
		new_node->add_input_pin("key", KnitPinKind::Data, sig_string, "key");
		new_node->add_input_pin("default", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("val", KnitPinKind::Data, sig_wild);
	} else if (tag == "cont_dict_set") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "dict_set", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("dict", KnitPinKind::Data, sig_dict);
		new_node->add_input_pin("key", KnitPinKind::Data, sig_string, "key");
		new_node->add_input_pin("val", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "cont_dict_has") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "dict_has", spawn_position);
		new_node->add_input_pin("dict", KnitPinKind::Data, sig_dict);
		new_node->add_input_pin("key", KnitPinKind::Data, sig_string, "key");
		new_node->add_output_pin("has", KnitPinKind::Data, sig_bool);
	} else if (tag == "util_str") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "str", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("str", KnitPinKind::Data, sig_string);
	} else if (tag == "util_range") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "range", spawn_position);
		new_node->add_input_pin("to", KnitPinKind::Data, sig_int, 10);
		new_node->add_output_pin("array", KnitPinKind::Data, sig_array);
	} else if (tag == "util_load") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "load", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("path", KnitPinKind::Data, sig_string, "res://");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_obj);

	// Signals & Callables
	} else if (tag == "signal_emit") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "emit_signal", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("signal_name", KnitPinKind::Data, sig_string, "my_signal");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "signal_connect") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "connect_signal", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("signal_name", KnitPinKind::Data, sig_string, "my_signal");
		new_node->add_input_pin("callable", KnitPinKind::Data, sig_callable);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "callable_call") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "call_callable", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("callable", KnitPinKind::Data, sig_callable);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_wild);

	// Types & Reflection
	} else if (tag == "type_typeof") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "typeof", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("type_id", KnitPinKind::Data, sig_int);
	} else if (tag == "type_is_valid") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "is_instance_valid", spawn_position);
		new_node->add_input_pin("obj", KnitPinKind::Data, sig_obj);
		new_node->add_output_pin("valid", KnitPinKind::Data, sig_bool);
	} else if (tag == "type_test") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "type_test", spawn_position);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("is_type", KnitPinKind::Data, sig_bool);

	// Actions & Debugging
	} else if (tag == "action_print") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "print", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "Hello from Knits!");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_print_rich") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "print_rich", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "[color=green]Success![/color]");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_printerr") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "printerr", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "Error message");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_push_error") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "push_error", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "Fatal error");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_push_warning") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "push_warning", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "Warning message");
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_assert") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "assert", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("condition", KnitPinKind::Data, sig_bool, true);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "action_move_and_slide") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "move_and_slide", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	}

	if (new_node.is_valid()) {
		_create_visual_node(new_node);
		apply_code();
		status_label->set_text(vformat("Spawned node: %s", new_node->title));
	}
}

void KnitsEditorBase::ensure_focus() {
	graph_edit->grab_focus();
}

void KnitsEditorBase::set_edited_resource(const Ref<Resource> &p_res) {
	edited_res = p_res;
	script = p_res;
	if (script.is_valid()) {
		graph = script->get_graph();
	} else {
		graph = p_res;
	}

	if (graph.is_null()) {
		graph.instantiate();
		if (script.is_valid()) {
			script->set_graph(graph);
		}
	}

	_update_graph_view();
}

void KnitsEditorBase::apply_code() {
	if (graph.is_null()) return;

	// Update node positions from visual GraphNodes
	for (KeyValue<KnitNodeID, GraphNode *> &E : visual_nodes) {
		if (E.value && graph->nodes.has(E.key)) {
			graph->nodes[E.key]->position = E.value->get_position_offset();
		}
	}

	if (script.is_valid()) {
		script->set_graph(graph);
		script->reload();
	}
}

void KnitsEditorBase::validate_script() {
	if (graph.is_null()) return;
	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	if (compiler.compile(graph, compiled, error)) {
		status_label->set_text("Graph Compiled Successfully!");
	} else {
		status_label->set_text(vformat("Error: %s", error));
	}
}

bool KnitsEditorBase::is_unsaved() {
	return false;
}

Ref<Texture2D> KnitsEditorBase::get_theme_icon() {
	if (is_inside_tree()) {
		return get_editor_theme_icon(SNAME("VisualScript"));
	}
	return Ref<Texture2D>();
}

void KnitsEditorBase::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		if (visual_nodes.is_empty() && graph.is_valid()) {
			_update_graph_view();
		}
	}
}

KnitsEditorBase::KnitsEditorBase() {
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_h_size_flags(SIZE_EXPAND_FILL);

	toolbar = memnew(HBoxContainer);
	add_child(toolbar);

	add_node_btn = memnew(Button);
	add_node_btn->set_text("+ Add Node");
	add_node_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_add_node_pressed));
	toolbar->add_child(add_node_btn);

	add_frame_btn = memnew(Button);
	add_frame_btn->set_text("+ Frame");
	add_frame_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_add_frame_pressed));
	toolbar->add_child(add_frame_btn);

	compile_btn = memnew(Button);
	compile_btn->set_text("Compile");
	compile_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_compile_pressed));
	toolbar->add_child(compile_btn);

	status_label = memnew(Label);
	status_label->set_text("Ready");
	status_label->set_h_size_flags(SIZE_EXPAND_FILL);
	toolbar->add_child(status_label);

	graph_edit = memnew(GraphEdit);
	graph_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	graph_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	graph_edit->set_snapping_enabled(true);
	graph_edit->set_snapping_distance(16);
	graph_edit->set_show_grid(true);
	graph_edit->set_minimap_enabled(true);
	graph_edit->set_connection_lines_curvature(0.5f);
	graph_edit->set_connection_lines_thickness(3.0f);
	graph_edit->set_connection_lines_antialiased(true);
	graph_edit->connect("connection_request", callable_mp(this, &KnitsEditorBase::_on_connection_request));
	graph_edit->connect("disconnection_request", callable_mp(this, &KnitsEditorBase::_on_disconnection_request));
	graph_edit->connect("delete_nodes_request", callable_mp(this, &KnitsEditorBase::_on_delete_nodes_request));
	graph_edit->connect("node_selected", callable_mp(this, &KnitsEditorBase::_on_node_selected));
	graph_edit->connect("node_deselected", callable_mp(this, &KnitsEditorBase::_on_node_deselected));
	graph_edit->connect("popup_request", callable_mp(this, &KnitsEditorBase::_on_popup_request));
	add_child(graph_edit);

	// Quick Spawn Dialog
	quick_spawn_dialog = memnew(ConfirmationDialog);
	quick_spawn_dialog->set_title("Quick Spawn Node Palette");
	VBoxContainer *vbox = memnew(VBoxContainer);
	quick_spawn_dialog->add_child(vbox);

	search_box = memnew(LineEdit);
	search_box->set_placeholder("Search nodes (e.g. Add, Branch, Print, Velocity)...");
	search_box->connect("text_changed", callable_mp(this, &KnitsEditorBase::_on_search_text_changed));
	vbox->add_child(search_box);

	palette_tree = memnew(Tree);
	palette_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	palette_tree->set_hide_root(true);
	palette_tree->connect("item_activated", callable_mp(this, &KnitsEditorBase::_on_palette_item_activated));
	vbox->add_child(palette_tree);

	quick_spawn_dialog->connect("confirmed", callable_mp(this, &KnitsEditorBase::_spawn_selected_palette_node));
	add_child(quick_spawn_dialog);
}

KnitsEditorBase::~KnitsEditorBase() {
}

#endif // TOOLS_ENABLED
