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
			return Color(0.80f, 0.20f, 0.28f); // Glowing Ruby Crimson
		case KnitNodeCategory::ImpureAction:
			return Color(0.20f, 0.48f, 0.90f); // Electric Cobalt
		case KnitNodeCategory::PureFunction:
			return Color(0.12f, 0.65f, 0.40f); // Vivid Mint Emerald
		case KnitNodeCategory::FlowControl:
			return Color(0.30f, 0.40f, 0.55f); // Slate Navy
		case KnitNodeCategory::VariableGet:
		case KnitNodeCategory::VariableSet:
			return Color(0.12f, 0.60f, 0.68f); // Luminous Teal Cyan
		case KnitNodeCategory::SubGraph:
			return Color(0.60f, 0.28f, 0.88f); // Royal Neon Violet
		case KnitNodeCategory::Reroute:
			return Color(0.32f, 0.35f, 0.42f);
		case KnitNodeCategory::Comment:
			return Color(0.14f, 0.16f, 0.20f);
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

	if (p_node->title == "Math Expression" || p_node->title == "math_expression" || p_node->title == "expression") {
		HBoxContainer *expr_box = memnew(HBoxContainer);
		expr_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		expr_box->set_custom_minimum_size(Size2(140, 24));
		Label *lbl = memnew(Label);
		lbl->set_text("f =");
		lbl->add_theme_color_override("font_color", Color(0.35f, 0.80f, 1.0f));
		expr_box->add_child(lbl);

		LineEdit *expr_edit = memnew(LineEdit);
		expr_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		expr_edit->set_text(p_node->target_symbol.is_empty() ? "x + y" : String(p_node->target_symbol));
		expr_edit->set_tooltip_text("Press Enter to update formula variables (e.g. (a + b) * sin(c))");
		expr_edit->connect("text_submitted", callable_mp(this, &KnitsEditorBase::_on_expression_text_submitted).bind(p_node->id));
		expr_box->add_child(expr_edit);
		gn->add_child(expr_box);
	}

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

	String node_title = p_node->title;
	if (node_title == "array_construct" || node_title == "dict_construct" || node_title == "format_str" || node_title == "str" || node_title == "Sequence") {
		HBoxContainer *btn_row = memnew(HBoxContainer);
		btn_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		btn_row->set_custom_minimum_size(Size2(0, 22));

		Button *add_btn = memnew(Button);
		add_btn->set_text("+ Pin");
		add_btn->set_focus_mode(Control::FOCUS_NONE);
		add_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_add_pin_pressed).bind(p_node->id));
		btn_row->add_child(add_btn);

		Button *rem_btn = memnew(Button);
		rem_btn->set_text("- Pin");
		rem_btn->set_focus_mode(Control::FOCUS_NONE);
		rem_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_remove_pin_pressed).bind(p_node->id));
		btn_row->add_child(rem_btn);

		gn->add_child(btn_row);
	}

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
	add_item(cat_flow, "Sequence", "Executes multiple flow branches in sequence", "flow_sequence");
	add_item(cat_flow, "Do Once", "Executes flow branch only once until reset", "flow_do_once");
	add_item(cat_flow, "Flip Flop", "Alternates execution between outputs A and B", "flow_flip_flop");
	add_item(cat_flow, "Gate", "Controllable flow gate with open/close/toggle", "flow_gate");
	add_item(cat_flow, "Reroute Knot", "Lightweight pass-through routing knot", "flow_reroute");
	add_item(cat_flow, "While Loop", "Executes loop body while condition is true", "flow_while");
	add_item(cat_flow, "For Each Loop", "Iterates over elements in array or range", "flow_foreach");
	add_item(cat_flow, "Delay (Seconds)", "Suspends coroutine execution for duration", "flow_yield_sec");
	add_item(cat_flow, "Delay (Frames)", "Suspends coroutine for N frame ticks", "flow_yield_frame");
	add_item(cat_flow, "Return", "Returns from script function with value", "flow_return");

	TreeItem *cat_structs = add_category("Structs & Vectors (Make / Break)");
	add_item(cat_structs, "Make Vector2", "Constructs Vector2 from (x, y)", "make_vec2");
	add_item(cat_structs, "Break Vector2", "Splits Vector2 into (x, y)", "break_vec2");
	add_item(cat_structs, "Make Vector3", "Constructs Vector3 from (x, y, z)", "make_vec3");
	add_item(cat_structs, "Break Vector3", "Splits Vector3 into (x, y, z)", "break_vec3");
	add_item(cat_structs, "Make Vector4", "Constructs Vector4 from (x, y, z, w)", "make_vec4");
	add_item(cat_structs, "Break Vector4", "Splits Vector4 into (x, y, z, w)", "break_vec4");
	add_item(cat_structs, "Make Color", "Constructs Color from (r, g, b, a)", "make_color");
	add_item(cat_structs, "Break Color", "Splits Color into (r, g, b, a)", "break_color");
	add_item(cat_structs, "Make Rect2", "Constructs Rect2 from (pos, size)", "make_rect2");
	add_item(cat_structs, "Break Rect2", "Splits Rect2 into (pos, size)", "break_rect2");
	add_item(cat_structs, "Make Transform2D", "Constructs Transform2D from rotation & origin", "make_transform2d");
	add_item(cat_structs, "Break Transform2D", "Splits Transform2D into origin & rotation", "break_transform2d");
	add_item(cat_structs, "Make Transform3D", "Constructs Transform3D from rotation & origin", "make_transform3d");
	add_item(cat_structs, "Break Transform3D", "Splits Transform3D into origin, rotation, scale", "break_transform3d");

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
	add_item(cat_cmp, "Select (Ternary ? :)", "Returns A if condition is true, else B", "logic_select");

	TreeItem *cat_math = add_category("Math & Trigonometry");
	add_item(cat_math, "Math Expression", "Inline math formula parser (e.g. (a + b) * sin(c))", "math_expression");
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

	TreeItem *cat_gameplay = add_category("Gameplay & Physics");
	add_item(cat_gameplay, "Character Move & Jump 3D", "Unified kinematic movement & jump for CharacterBody3D", "gameplay_char_move_jump_3d");
	add_item(cat_gameplay, "Raycast Query 3D", "Performs single-line 3D physics raycast", "gameplay_raycast_3d");

	TreeItem *cat_anim = add_category("Animation & Audio");
	add_item(cat_anim, "Tween Property", "Interpolates node property over time", "anim_tween_property");
	add_item(cat_anim, "Play Sound 3D", "Spawns a 3D audio one-shot sound", "audio_play_sound_3d");

	TreeItem *cat_actions = add_category("Actions & Debugging");
	add_item(cat_actions, "Print Message", "Prints value to console", "action_print");
	add_item(cat_actions, "Print Rich", "Prints BBCode formatted text", "action_print_rich");
	add_item(cat_actions, "Print Error", "Prints error highlighted text", "action_printerr");
	add_item(cat_actions, "Push Error", "Pushes engine debugger error", "action_push_error");
	add_item(cat_actions, "Push Warning", "Pushes engine debugger warning", "action_push_warning");
	add_item(cat_actions, "Assert", "Asserts condition is true in debug builds", "action_assert");
	add_item(cat_actions, "Move and Slide", "Performs character body movement with collision", "action_move_and_slide");

	// Dynamic Engine Classes (ClassDB Auto-Discovery)
	TreeItem *cat_classes = add_category("Engine Classes (ClassDB)");
	static const char *common_classes[] = {
		"Node", "Node2D", "Node3D", "Control",
		"CharacterBody2D", "CharacterBody3D", "RigidBody3D", "Area3D", "Area2D",
		"Camera3D", "Camera2D", "AudioStreamPlayer", "AudioStreamPlayer3D",
		"AnimationPlayer", "Timer", "Sprite2D", "Sprite3D", "MeshInstance3D",
		"RayCast3D", "RayCast2D", "Label", "Button", "ProgressBar", "TextureRect",
		"HTTPRequest"
	};

	Vector<StringName> target_classes;
	if (p_filter.is_empty()) {
		for (const char *cls_name : common_classes) {
			target_classes.push_back(StringName(cls_name));
		}
	} else {
		LocalVector<StringName> all_classes;
		ClassDB::get_class_list(all_classes);
		for (uint32_t i = 0; i < all_classes.size(); i++) {
			const StringName &cls = all_classes[i];
			if (String(cls).findn(p_filter) != -1) {
				target_classes.push_back(cls);
			}
		}
		for (const char *cls_name : common_classes) {
			if (!target_classes.has(StringName(cls_name))) {
				target_classes.push_back(StringName(cls_name));
			}
		}
	}

	for (int c = 0; c < target_classes.size(); c++) {
		StringName cls = target_classes[c];
		if (!ClassDB::class_exists(cls)) continue;

		TreeItem *class_cat = nullptr;

		// 1. Methods
		List<MethodInfo> methods;
		ClassDB::get_method_list(cls, &methods, true);
		for (const MethodInfo &m : methods) {
			if (m.name.begins_with("_")) continue;
			if (!p_filter.is_empty() && String(m.name).findn(p_filter) == -1 && String(cls).findn(p_filter) == -1) continue;

			if (!class_cat) {
				class_cat = palette_tree->create_item(cat_classes);
				class_cat->set_text(0, String(cls));
			}

			TreeItem *m_item = palette_tree->create_item(class_cat);
			m_item->set_text(0, vformat("Call: %s.%s()", String(cls), String(m.name)));
			m_item->set_metadata(0, vformat("class_method:%s:%s", String(cls), String(m.name)));
		}

		// 2. Properties (Get / Set)
		List<PropertyInfo> props;
		ClassDB::get_property_list(cls, &props, true);
		for (const PropertyInfo &p : props) {
			if (p.name.begins_with("_")) continue;
			if (p.usage & PROPERTY_USAGE_GROUP || p.usage & PROPERTY_USAGE_CATEGORY || p.usage & PROPERTY_USAGE_SUBGROUP) continue;
			if (!p_filter.is_empty() && String(p.name).findn(p_filter) == -1 && String(cls).findn(p_filter) == -1) continue;

			if (!class_cat) {
				class_cat = palette_tree->create_item(cat_classes);
				class_cat->set_text(0, String(cls));
			}

			TreeItem *get_item = palette_tree->create_item(class_cat);
			get_item->set_text(0, vformat("Get: %s.%s", String(cls), String(p.name)));
			get_item->set_metadata(0, vformat("class_prop_get:%s:%s", String(cls), String(p.name)));

			TreeItem *set_item = palette_tree->create_item(class_cat);
			set_item->set_text(0, vformat("Set: %s.%s", String(cls), String(p.name)));
			set_item->set_metadata(0, vformat("class_prop_set:%s:%s", String(cls), String(p.name)));
		}

		// 3. Signals
		List<MethodInfo> signals;
		ClassDB::get_signal_list(cls, &signals, true);
		for (const MethodInfo &sig : signals) {
			if (!p_filter.is_empty() && String(sig.name).findn(p_filter) == -1 && String(cls).findn(p_filter) == -1) continue;

			if (!class_cat) {
				class_cat = palette_tree->create_item(cat_classes);
				class_cat->set_text(0, String(cls));
			}

			TreeItem *sig_item = palette_tree->create_item(class_cat);
			sig_item->set_text(0, vformat("Signal: %s.%s", String(cls), String(sig.name)));
			sig_item->set_metadata(0, vformat("class_signal:%s:%s", String(cls), String(sig.name)));
		}
	}
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
	} else if (tag == "flow_sequence") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Sequence", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Then 0", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Then 1", KnitPinKind::Execution, sig_exec);
	} else if (tag == "logic_select") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "Select", spawn_position);
		new_node->add_input_pin("Condition", KnitPinKind::Data, sig_bool, true);
		new_node->add_input_pin("True", KnitPinKind::Data, sig_wild);
		new_node->add_input_pin("False", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("Result", KnitPinKind::Data, sig_wild);

	// Make / Break Structs & Components
	} else if (tag == "make_vec2") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_vec2", spawn_position);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("y", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("vec2", KnitPinKind::Data, sig_vec2);
	} else if (tag == "break_vec2") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_vec2", spawn_position);
		new_node->add_input_pin("vec2", KnitPinKind::Data, sig_vec2);
		new_node->add_output_pin("x", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("y", KnitPinKind::Data, sig_float);
	} else if (tag == "make_vec3") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_vec3", spawn_position);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("y", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("z", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("vec3", KnitPinKind::Data, sig_vec3);
	} else if (tag == "break_vec3") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_vec3", spawn_position);
		new_node->add_input_pin("vec3", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("x", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("y", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("z", KnitPinKind::Data, sig_float);
	} else if (tag == "make_vec4") {
		KnitTypeSignature sig_vec4;
		sig_vec4.kind = KnitDataType::Vector4;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_vec4", spawn_position);
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("y", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("z", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("w", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("vec4", KnitPinKind::Data, sig_vec4);
	} else if (tag == "break_vec4") {
		KnitTypeSignature sig_vec4;
		sig_vec4.kind = KnitDataType::Vector4;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_vec4", spawn_position);
		new_node->add_input_pin("vec4", KnitPinKind::Data, sig_vec4);
		new_node->add_output_pin("x", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("y", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("z", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("w", KnitPinKind::Data, sig_float);
	} else if (tag == "make_color") {
		KnitTypeSignature sig_col;
		sig_col.kind = KnitDataType::Color;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_color", spawn_position);
		new_node->add_input_pin("r", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("g", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("color", KnitPinKind::Data, sig_col);
	} else if (tag == "break_color") {
		KnitTypeSignature sig_col;
		sig_col.kind = KnitDataType::Color;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_color", spawn_position);
		new_node->add_input_pin("color", KnitPinKind::Data, sig_col);
		new_node->add_output_pin("r", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("g", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("b", KnitPinKind::Data, sig_float);
		new_node->add_output_pin("a", KnitPinKind::Data, sig_float);
	} else if (tag == "make_rect2") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		KnitTypeSignature sig_rect2;
		sig_rect2.kind = KnitDataType::Rect2;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_rect2", spawn_position);
		new_node->add_input_pin("position", KnitPinKind::Data, sig_vec2);
		new_node->add_input_pin("size", KnitPinKind::Data, sig_vec2);
		new_node->add_output_pin("rect2", KnitPinKind::Data, sig_rect2);
	} else if (tag == "break_rect2") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		KnitTypeSignature sig_rect2;
		sig_rect2.kind = KnitDataType::Rect2;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_rect2", spawn_position);
		new_node->add_input_pin("rect2", KnitPinKind::Data, sig_rect2);
		new_node->add_output_pin("position", KnitPinKind::Data, sig_vec2);
		new_node->add_output_pin("size", KnitPinKind::Data, sig_vec2);
	} else if (tag == "make_transform2d") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		KnitTypeSignature sig_t2d;
		sig_t2d.kind = KnitDataType::Transform2D;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_transform2d", spawn_position);
		new_node->add_input_pin("rotation", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("origin", KnitPinKind::Data, sig_vec2);
		new_node->add_output_pin("transform2d", KnitPinKind::Data, sig_t2d);
	} else if (tag == "break_transform2d") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		KnitTypeSignature sig_t2d;
		sig_t2d.kind = KnitDataType::Transform2D;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_transform2d", spawn_position);
		new_node->add_input_pin("transform2d", KnitPinKind::Data, sig_t2d);
		new_node->add_output_pin("origin", KnitPinKind::Data, sig_vec2);
		new_node->add_output_pin("rotation", KnitPinKind::Data, sig_float);
	} else if (tag == "make_transform3d") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		KnitTypeSignature sig_t3d;
		sig_t3d.kind = KnitDataType::Transform3D;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "make_transform3d", spawn_position);
		new_node->add_input_pin("rotation", KnitPinKind::Data, sig_vec3);
		new_node->add_input_pin("origin", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("transform3d", KnitPinKind::Data, sig_t3d);
	} else if (tag == "break_transform3d") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		KnitTypeSignature sig_t3d;
		sig_t3d.kind = KnitDataType::Transform3D;
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "break_transform3d", spawn_position);
		new_node->add_input_pin("transform3d", KnitPinKind::Data, sig_t3d);
		new_node->add_output_pin("origin", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("rotation", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("scale", KnitPinKind::Data, sig_vec3);

	// Dynamic ClassDB Methods, Properties, Signals
	} else if (tag.begins_with("class_method:")) {
		Vector<String> parts = tag.split(":");
		if (parts.size() >= 3) {
			StringName class_name = parts[1];
			StringName method_name = parts[2];
			MethodInfo mi;
			if (ClassDB::get_method_info(class_name, method_name, &mi)) {
				bool is_const = (mi.flags & METHOD_FLAG_CONST);
				bool has_return = (mi.return_val.type != Variant::NIL);
				KnitNodeCategory cat = (is_const && has_return) ? KnitNodeCategory::PureFunction : KnitNodeCategory::ImpureAction;

				new_node = graph->create_node(cat, vformat("%s", String(method_name)), spawn_position);
				new_node->target_symbol = method_name;

				if (cat == KnitNodeCategory::ImpureAction) {
					new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
				}

				KnitTypeSignature target_sig;
				target_sig.kind = KnitDataType::ObjectRef;
				target_sig.custom_type_name = class_name;
				new_node->add_input_pin("Target", KnitPinKind::Data, target_sig);

				for (int a = 0; a < mi.arguments.size(); a++) {
					const PropertyInfo &arg = mi.arguments[a];
					new_node->add_input_pin(arg.name, KnitPinKind::Data, KnitTypeSignature::from_property_info(arg));
				}

				if (cat == KnitNodeCategory::ImpureAction) {
					new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
				}

				if (has_return) {
					new_node->add_output_pin("Result", KnitPinKind::Data, KnitTypeSignature::from_property_info(mi.return_val));
				}
			}
		}
	} else if (tag.begins_with("class_prop_get:")) {
		Vector<String> parts = tag.split(":");
		if (parts.size() >= 3) {
			StringName class_name = parts[1];
			StringName prop_name = parts[2];
			Variant::Type p_type = ClassDB::get_property_type(class_name, prop_name);

			new_node = graph->create_node(KnitNodeCategory::PureFunction, vformat("Get %s", String(prop_name)), spawn_position);
			new_node->target_symbol = prop_name;

			KnitTypeSignature target_sig;
			target_sig.kind = KnitDataType::ObjectRef;
			target_sig.custom_type_name = class_name;
			new_node->add_input_pin("Target", KnitPinKind::Data, target_sig);

			KnitTypeSignature val_sig;
			val_sig.kind = KnitTypeSignature::from_variant_type(p_type);
			new_node->add_output_pin("Value", KnitPinKind::Data, val_sig);
		}
	} else if (tag.begins_with("class_prop_set:")) {
		Vector<String> parts = tag.split(":");
		if (parts.size() >= 3) {
			StringName class_name = parts[1];
			StringName prop_name = parts[2];
			Variant::Type p_type = ClassDB::get_property_type(class_name, prop_name);

			new_node = graph->create_node(KnitNodeCategory::ImpureAction, vformat("Set %s", String(prop_name)), spawn_position);
			new_node->target_symbol = prop_name;

			new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);

			KnitTypeSignature target_sig;
			target_sig.kind = KnitDataType::ObjectRef;
			target_sig.custom_type_name = class_name;
			new_node->add_input_pin("Target", KnitPinKind::Data, target_sig);

			KnitTypeSignature val_sig;
			val_sig.kind = KnitTypeSignature::from_variant_type(p_type);
			new_node->add_input_pin("Value", KnitPinKind::Data, val_sig);

			new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		}
	} else if (tag.begins_with("class_signal:")) {
		Vector<String> parts = tag.split(":");
		if (parts.size() >= 3) {
			StringName class_name = parts[1];
			StringName signal_name = parts[2];
			MethodInfo sig_info;
			ClassDB::get_signal(class_name, signal_name, &sig_info);

			new_node = graph->create_node(KnitNodeCategory::Event, vformat("On %s", String(signal_name)), spawn_position);
			new_node->target_symbol = signal_name;
			new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

			for (int a = 0; a < sig_info.arguments.size(); a++) {
				const PropertyInfo &arg = sig_info.arguments[a];
				new_node->add_output_pin(arg.name, KnitPinKind::Data, KnitTypeSignature::from_property_info(arg));
			}
		}
	} else if (tag == "flow_do_once") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Do Once", spawn_position);
		new_node->add_input_pin("In", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Reset", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Start Closed", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("Out", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_flip_flop") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Flip Flop", spawn_position);
		new_node->add_input_pin("In", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("A", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("B", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Is A", KnitPinKind::Data, sig_bool);
	} else if (tag == "flow_gate") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Gate", spawn_position);
		new_node->add_input_pin("In", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Open", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Close", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Toggle", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Start Closed", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("Out", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Is Open", KnitPinKind::Data, sig_bool);
	} else if (tag == "gameplay_char_move_jump_3d") {
		KnitTypeSignature sig_vec2;
		sig_vec2.kind = KnitDataType::Vector2;
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		KnitTypeSignature sig_body;
		sig_body.kind = KnitDataType::ObjectRef;
		sig_body.custom_type_name = "CharacterBody3D";

		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "Character Move & Jump 3D", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Character", KnitPinKind::Data, sig_body);
		new_node->add_input_pin("Input Dir", KnitPinKind::Data, sig_vec2, Vector2(0, 0));
		new_node->add_input_pin("Speed", KnitPinKind::Data, sig_float, 5.0);
		new_node->add_input_pin("Jump", KnitPinKind::Data, sig_bool, false);
		new_node->add_input_pin("Jump Velocity", KnitPinKind::Data, sig_float, 4.5);
		new_node->add_input_pin("Gravity", KnitPinKind::Data, sig_float, 9.8);
		new_node->add_input_pin("Delta", KnitPinKind::Data, sig_float, 0.0166);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Is Grounded", KnitPinKind::Data, sig_bool);
		new_node->add_output_pin("Velocity", KnitPinKind::Data, sig_vec3);
	} else if (tag == "gameplay_raycast_3d") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		KnitTypeSignature sig_obj;
		sig_obj.kind = KnitDataType::ObjectRef;

		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "Raycast Query 3D", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Origin", KnitPinKind::Data, sig_vec3, Vector3(0, 0, 0));
		new_node->add_input_pin("Target", KnitPinKind::Data, sig_vec3, Vector3(0, -10, 0));
		new_node->add_input_pin("Collision Mask", KnitPinKind::Data, sig_int, 1);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Hit", KnitPinKind::Data, sig_bool);
		new_node->add_output_pin("Hit Position", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("Hit Normal", KnitPinKind::Data, sig_vec3);
		new_node->add_output_pin("Hit Collider", KnitPinKind::Data, sig_obj);
	} else if (tag == "anim_tween_property") {
		KnitTypeSignature sig_obj;
		sig_obj.kind = KnitDataType::ObjectRef;

		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "Tween Property", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Target", KnitPinKind::Data, sig_obj);
		new_node->add_input_pin("Property", KnitPinKind::Data, sig_string, "position");
		new_node->add_input_pin("Final Value", KnitPinKind::Data, sig_wild);
		new_node->add_input_pin("Duration", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("Tween", KnitPinKind::Data, sig_obj);
	} else if (tag == "audio_play_sound_3d") {
		KnitTypeSignature sig_vec3;
		sig_vec3.kind = KnitDataType::Vector3;
		KnitTypeSignature sig_res;
		sig_res.kind = KnitDataType::ObjectRef;
		sig_res.custom_type_name = "AudioStream";

		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "Play Sound 3D", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Stream", KnitPinKind::Data, sig_res);
		new_node->add_input_pin("Position", KnitPinKind::Data, sig_vec3, Vector3(0, 0, 0));
		new_node->add_input_pin("Volume dB", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("Pitch Scale", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_reroute") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "Reroute", spawn_position);
		new_node->add_input_pin("In", KnitPinKind::Data, sig_wild);
		new_node->add_output_pin("Out", KnitPinKind::Data, sig_wild);
	} else if (tag == "math_expression") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "Math Expression", spawn_position);
		new_node->target_symbol = "x + y";
		new_node->add_input_pin("x", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("y", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("Result", KnitPinKind::Data, sig_float);
	} else if (tag == "action_move_and_slide") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "move_and_slide", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	}

	if (new_node.is_valid()) {
		if (wire_drag_source_node != 0 && wire_drag_source_pin != 0) {
			if (wire_drag_is_output) {
				for (int i = 0; i < new_node->input_pins.size(); i++) {
					const KnitPin &in_pin = new_node->input_pins[i];
					if (wire_drag_pin_kind == KnitPinKind::Execution && in_pin.kind == KnitPinKind::Execution) {
						graph->connect_pins(wire_drag_source_node, wire_drag_source_pin, new_node->id, in_pin.id);
						break;
					} else if (wire_drag_pin_kind == KnitPinKind::Data && in_pin.kind == KnitPinKind::Data) {
						if (in_pin.type.kind == KnitDataType::Wildcard || wire_drag_data_type == KnitDataType::Wildcard || in_pin.type.kind == wire_drag_data_type) {
							graph->connect_pins(wire_drag_source_node, wire_drag_source_pin, new_node->id, in_pin.id);
							break;
						}
					}
				}
			} else {
				for (int i = 0; i < new_node->output_pins.size(); i++) {
					const KnitPin &out_pin = new_node->output_pins[i];
					if (wire_drag_pin_kind == KnitPinKind::Execution && out_pin.kind == KnitPinKind::Execution) {
						graph->connect_pins(new_node->id, out_pin.id, wire_drag_source_node, wire_drag_source_pin);
						break;
					} else if (wire_drag_pin_kind == KnitPinKind::Data && out_pin.kind == KnitPinKind::Data) {
						if (out_pin.type.kind == KnitDataType::Wildcard || wire_drag_data_type == KnitDataType::Wildcard || out_pin.type.kind == wire_drag_data_type) {
							graph->connect_pins(new_node->id, out_pin.id, wire_drag_source_node, wire_drag_source_pin);
							break;
						}
					}
				}
			}
			wire_drag_source_node = 0;
			wire_drag_source_pin = 0;
		}

		_create_visual_node(new_node);
		apply_code();
		status_label->set_text(vformat("Spawned node: %s", new_node->title));
	}
}

void KnitsEditorBase::_on_connection_to_empty(const StringName &p_from, int p_from_slot, const Vector2 &p_release_position) {
	for (const KeyValue<KnitNodeID, GraphNode *> &E : visual_nodes) {
		if (E.value && E.value->get_name() == p_from) {
			wire_drag_source_node = E.key;
			if (output_slot_to_pin.has(E.key) && p_from_slot >= 0 && p_from_slot < output_slot_to_pin[E.key].size()) {
				wire_drag_source_pin = output_slot_to_pin[E.key][p_from_slot];
				Ref<KnitNode> node = graph->get_node(E.key);
				if (node.is_valid()) {
					for (int i = 0; i < node->output_pins.size(); i++) {
						if (node->output_pins[i].id == wire_drag_source_pin) {
							wire_drag_pin_kind = node->output_pins[i].kind;
							wire_drag_data_type = node->output_pins[i].type.kind;
							break;
						}
					}
				}
			}
			wire_drag_is_output = true;
			break;
		}
	}

	spawn_position = (p_release_position + graph_edit->get_scroll_offset()) / graph_edit->get_zoom();
	_populate_palette_tree("");
	search_box->clear();
	quick_spawn_dialog->popup_centered_ratio(0.4);
	search_box->grab_focus();
}

void KnitsEditorBase::_on_connection_from_empty(const StringName &p_to, int p_to_slot, const Vector2 &p_release_position) {
	for (const KeyValue<KnitNodeID, GraphNode *> &E : visual_nodes) {
		if (E.value && E.value->get_name() == p_to) {
			wire_drag_source_node = E.key;
			if (input_slot_to_pin.has(E.key) && p_to_slot >= 0 && p_to_slot < input_slot_to_pin[E.key].size()) {
				wire_drag_source_pin = input_slot_to_pin[E.key][p_to_slot];
				Ref<KnitNode> node = graph->get_node(E.key);
				if (node.is_valid()) {
					for (int i = 0; i < node->input_pins.size(); i++) {
						if (node->input_pins[i].id == wire_drag_source_pin) {
							wire_drag_pin_kind = node->input_pins[i].kind;
							wire_drag_data_type = node->input_pins[i].type.kind;
							break;
						}
					}
				}
			}
			wire_drag_is_output = false;
			break;
		}
	}

	spawn_position = (p_release_position + graph_edit->get_scroll_offset()) / graph_edit->get_zoom();
	_populate_palette_tree("");
	search_box->clear();
	quick_spawn_dialog->popup_centered_ratio(0.4);
	search_box->grab_focus();
}

void KnitsEditorBase::_on_expression_text_submitted(const String &p_text, KnitNodeID p_node_id) {
	if (graph.is_null()) return;
	Ref<KnitNode> node = graph->get_node(p_node_id);
	if (node.is_null()) return;

	node->target_symbol = p_text;

	// Extract unique variable identifiers from formula
	HashSet<String> math_funcs;
	math_funcs.insert("sin"); math_funcs.insert("cos"); math_funcs.insert("tan");
	math_funcs.insert("asin"); math_funcs.insert("acos"); math_funcs.insert("atan"); math_funcs.insert("atan2");
	math_funcs.insert("sqrt"); math_funcs.insert("abs"); math_funcs.insert("sign");
	math_funcs.insert("floor"); math_funcs.insert("ceil"); math_funcs.insert("round");
	math_funcs.insert("min"); math_funcs.insert("max"); math_funcs.insert("clamp");
	math_funcs.insert("lerp"); math_funcs.insert("remap"); math_funcs.insert("smoothstep");
	math_funcs.insert("pow"); math_funcs.insert("pi"); math_funcs.insert("tau"); math_funcs.insert("e");

	Vector<String> vars;
	int pos = 0;
	while (pos < p_text.length()) {
		char32_t c = p_text[pos];
		if (is_unicode_identifier_start(c) || c == '_') {
			int start = pos;
			while (pos < p_text.length() && (is_unicode_identifier_continue(p_text[pos]) || p_text[pos] == '_')) {
				pos++;
			}
			String ident = p_text.substr(start, pos - start);
			if (!math_funcs.has(ident.to_lower()) && !vars.has(ident)) {
				vars.push_back(ident);
			}
		} else {
			pos++;
		}
	}

	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	// Remove old pins not in vars
	for (int i = node->input_pins.size() - 1; i >= 0; i--) {
		if (!vars.has(node->input_pins[i].name)) {
			KnitPinID pin_id = node->input_pins[i].id;
			graph->disconnect_pins(0, pin_id);
			node->remove_pin(pin_id);
		}
	}

	// Add new pins for new vars
	for (int i = 0; i < vars.size(); i++) {
		bool exists = false;
		for (int j = 0; j < node->input_pins.size(); j++) {
			if (node->input_pins[j].name == vars[i]) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			node->add_input_pin(vars[i], KnitPinKind::Data, sig_float, 0.0);
		}
	}

	_update_graph_view();
	apply_code();
	status_label->set_text(vformat("Expression formula updated: %s", p_text));
}

void KnitsEditorBase::_on_add_pin_pressed(KnitNodeID p_node_id) {
	if (graph.is_null()) return;
	Ref<KnitNode> node = graph->get_node(p_node_id);
	if (node.is_null()) return;

	String title = node->title;
	if (title == "array_construct" || title == "format_str" || title == "str") {
		KnitTypeSignature sig_wild;
		sig_wild.kind = KnitDataType::Wildcard;
		int idx = node->input_pins.size();
		node->add_input_pin(vformat("elem%d", idx), KnitPinKind::Data, sig_wild);
	} else if (title == "dict_construct") {
		KnitTypeSignature sig_str;
		sig_str.kind = KnitDataType::String;
		KnitTypeSignature sig_wild;
		sig_wild.kind = KnitDataType::Wildcard;
		int idx = node->input_pins.size() / 2;
		node->add_input_pin(vformat("key%d", idx), KnitPinKind::Data, sig_str, vformat("key%d", idx));
		node->add_input_pin(vformat("val%d", idx), KnitPinKind::Data, sig_wild);
	} else if (title == "Sequence") {
		KnitTypeSignature sig_exec;
		sig_exec.kind = KnitDataType::Execution;
		int idx = node->output_pins.size();
		node->add_output_pin(vformat("Then %d", idx), KnitPinKind::Execution, sig_exec);
	}

	_update_graph_view();
	apply_code();
}

void KnitsEditorBase::_on_remove_pin_pressed(KnitNodeID p_node_id) {
	if (graph.is_null()) return;
	Ref<KnitNode> node = graph->get_node(p_node_id);
	if (node.is_null()) return;

	String title = node->title;
	if (title == "array_construct" || title == "format_str" || title == "str") {
		if (node->input_pins.size() > 1) {
			KnitPinID last_pin = node->input_pins[node->input_pins.size() - 1].id;
			graph->disconnect_pins(0, last_pin);
			node->remove_pin(last_pin);
		}
	} else if (title == "dict_construct") {
		if (node->input_pins.size() > 2) {
			KnitPinID val_pin = node->input_pins[node->input_pins.size() - 1].id;
			KnitPinID key_pin = node->input_pins[node->input_pins.size() - 2].id;
			graph->disconnect_pins(0, val_pin);
			graph->disconnect_pins(0, key_pin);
			node->remove_pin(val_pin);
			node->remove_pin(key_pin);
		}
	} else if (title == "Sequence") {
		if (node->output_pins.size() > 1) {
			KnitPinID last_pin = node->output_pins[node->output_pins.size() - 1].id;
			graph->disconnect_pins(last_pin, 0);
			node->remove_pin(last_pin);
		}
	}

	_update_graph_view();
	apply_code();
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
	add_node_btn->set_flat(true);
	add_node_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_add_node_pressed));
	toolbar->add_child(add_node_btn);

	add_frame_btn = memnew(Button);
	add_frame_btn->set_text("+ Frame");
	add_frame_btn->set_flat(true);
	add_frame_btn->connect("pressed", callable_mp(this, &KnitsEditorBase::_on_add_frame_pressed));
	toolbar->add_child(add_frame_btn);

	compile_btn = memnew(Button);
	compile_btn->set_text("Compile");
	compile_btn->set_flat(true);
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
	graph_edit->connect("connection_to_empty", callable_mp(this, &KnitsEditorBase::_on_connection_to_empty));
	graph_edit->connect("connection_from_empty", callable_mp(this, &KnitsEditorBase::_on_connection_from_empty));
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
