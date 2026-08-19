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

void KnitsEditorBase::_create_visual_node(const Ref<KnitNode> &p_node) {
	if (p_node.is_null()) return;

	GraphNode *gn = memnew(GraphNode);
	gn->set_name("node_" + itos((uint64_t)p_node->id));
	gn->set_title(p_node->title);
	gn->set_position_offset(p_node->position);
	gn->set_resizable(false);

	visual_nodes[p_node->id] = gn;
	node_id_lookup[gn] = p_node->id;

	Vector<KnitPinID> in_pins;
	Vector<KnitPinID> out_pins;

	int max_slots = MAX(p_node->input_pins.size(), p_node->output_pins.size());
	if (max_slots == 0) max_slots = 1;

	for (int i = 0; i < max_slots; i++) {
		Label *slot_label = memnew(Label);
		gn->add_child(slot_label);

		bool has_left = i < p_node->input_pins.size();
		bool has_right = i < p_node->output_pins.size();

		int left_type = 0;
		Color left_col = Color(1, 1, 1);
		if (has_left) {
			const KnitPin &pin = p_node->input_pins[i];
			left_type = (pin.kind == KnitPinKind::Execution) ? 0 : (int)pin.type.kind;
			left_col = _get_pin_color(pin.kind, pin.type.kind);
			slot_label->set_text(pin.display_label);
			in_pins.push_back(pin.id);
		}

		int right_type = 0;
		Color right_col = Color(1, 1, 1);
		if (has_right) {
			const KnitPin &pin = p_node->output_pins[i];
			right_type = (pin.kind == KnitPinKind::Execution) ? 0 : (int)pin.type.kind;
			right_col = _get_pin_color(pin.kind, pin.type.kind);
			if (has_left) {
				slot_label->set_text(slot_label->get_text() + "        " + pin.display_label);
			} else {
				slot_label->set_text(pin.display_label);
				slot_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
			}
			out_pins.push_back(pin.id);
		}

		gn->set_slot(i, has_left, left_type, left_col, has_right, right_type, right_col);
	}

	input_slot_to_pin[p_node->id] = in_pins;
	output_slot_to_pin[p_node->id] = out_pins;

	graph_edit->add_child(gn);
}

void KnitsEditorBase::_update_graph_view() {
	graph_edit->clear_connections();

	for (KeyValue<KnitNodeID, GraphNode *> &E : visual_nodes) {
		if (E.value && E.value->get_parent() == graph_edit) {
			memdelete(E.value);
		}
	}
	visual_nodes.clear();
	node_id_lookup.clear();
	input_slot_to_pin.clear();
	output_slot_to_pin.clear();

	if (graph.is_null()) return;

	for (const KeyValue<KnitNodeID, Ref<KnitNode>> &E : graph->nodes) {
		_create_visual_node(E.value);
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

void KnitsEditorBase::_on_compile_pressed() {
	apply_code();
	validate_script();
}

void KnitsEditorBase::_populate_palette_tree(const String &p_filter) {
	palette_tree->clear();
	TreeItem *root = palette_tree->create_item();

	TreeItem *cat_events = palette_tree->create_item(root);
	cat_events->set_text(0, "Events");

	TreeItem *cat_actions = palette_tree->create_item(root);
	cat_actions->set_text(0, "Actions & Methods");

	TreeItem *cat_math = palette_tree->create_item(root);
	cat_math->set_text(0, "Math & Expressions");

	TreeItem *cat_flow = palette_tree->create_item(root);
	cat_flow->set_text(0, "Flow Control");

	auto add_item = [&](TreeItem *p_parent, const String &p_name, const String &p_desc, const String &p_tag) {
		if (!p_filter.is_empty() && p_name.findn(p_filter) == -1 && p_desc.findn(p_filter) == -1) return;
		TreeItem *item = palette_tree->create_item(p_parent);
		item->set_text(0, p_name);
		item->set_metadata(0, p_tag);
	};

	add_item(cat_events, "Event: _ready", "Called when node enters scene tree", "event_ready");
	add_item(cat_events, "Event: _process", "Called every render frame", "event_process");
	add_item(cat_events, "Event: _physics_process", "Called every physics frame tick", "event_physics_process");

	add_item(cat_math, "Add (+)", "Adds two numbers or vectors", "math_add");
	add_item(cat_math, "Subtract (-)", "Subtracts value B from A", "math_sub");
	add_item(cat_math, "Multiply (*)", "Multiplies two numbers", "math_mul");
	add_item(cat_math, "Divide (/)", "Divides value A by B", "math_div");

	add_item(cat_flow, "Branch (If)", "Branches execution based on condition", "flow_branch");
	add_item(cat_flow, "Delay (Seconds)", "Suspends coroutine execution for duration", "flow_yield");
	add_item(cat_flow, "Return", "Returns from script function", "flow_return");

	add_item(cat_actions, "Print Message", "Prints value to console", "action_print");
	add_item(cat_actions, "Set Velocity", "Sets 3D velocity vector", "action_velocity");
	add_item(cat_actions, "Move and Slide", "Performs character body movement with collision", "action_move_and_slide");
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

	KnitTypeSignature sig_bool;
	sig_bool.kind = KnitDataType::Bool;

	KnitTypeSignature sig_string;
	sig_string.kind = KnitDataType::String;

	Ref<KnitNode> new_node;

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
	} else if (tag == "math_add") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "add", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "math_mul") {
		new_node = graph->create_node(KnitNodeCategory::PureFunction, "mul", spawn_position);
		new_node->add_input_pin("a", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_input_pin("b", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("res", KnitPinKind::Data, sig_float);
	} else if (tag == "flow_branch") {
		new_node = graph->create_node(KnitNodeCategory::FlowControl, "Branch", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("Condition", KnitPinKind::Data, sig_bool, false);
		new_node->add_output_pin("True", KnitPinKind::Execution, sig_exec);
		new_node->add_output_pin("False", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_yield") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "yield_seconds", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("seconds", KnitPinKind::Data, sig_float, 1.0);
		new_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	} else if (tag == "flow_return") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("val", KnitPinKind::Data, sig_float, 0.0);
	} else if (tag == "action_print") {
		new_node = graph->create_node(KnitNodeCategory::ImpureAction, "print", spawn_position);
		new_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
		new_node->add_input_pin("message", KnitPinKind::Data, sig_string, "Hello from Knits!");
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
		status_label->set_text("✅ Graph Compiled Successfully!");
	} else {
		status_label->set_text(vformat("❌ Error: %s", error));
	}
}

bool KnitsEditorBase::is_unsaved() {
	return false;
}

void KnitsEditorBase::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_update_graph_view();
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

	compile_btn = memnew(Button);
	compile_btn->set_text("⚡ Compile");
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
