/**************************************************************************/
/*  knits_node.h                                                          */
/**************************************************************************/

#pragma once

#include "knits_types.h"
#include "core/io/resource.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"

struct KnitPin {
	KnitPinID id = 0;
	KnitNodeID owner_node = 0;
	StringName name;
	String display_label;
	KnitPinKind kind = KnitPinKind::Data;
	KnitPinDirection direction = KnitPinDirection::Input;
	KnitTypeSignature type;

	// Default literal value when no wire is connected
	Variant default_value;

	bool is_connected = false;
	bool is_hidden = false;
	bool is_orphaned = false; // Marked true if signature drift occurs on load
};

enum class KnitNodeCategory : uint8_t {
	Event,        // Entry points (_ready, _physics_process, custom signals)
	ImpureAction, // Functions with side effects (set_position, play_sound)
	PureFunction, // Deterministic pure math/expression nodes (distance, add)
	FlowControl,  // Branch (If), ForLoop, WhileLoop, Sequence, Merge
	VariableGet,  // Fast member/local variable read
	VariableSet,  // Variable write with pass-through flow
	SubGraph,     // Referenced external .knit_macro resource
	Reroute,      // Zero-cost visual wire routing knot
	Comment       // Visual grouping box
};

class KnitNode : public RefCounted {
	GDCLASS(KnitNode, RefCounted);

protected:
	static void _bind_methods();

public:
	KnitNodeID id = 0;
	String title;
	String comments;
	KnitNodeCategory category = KnitNodeCategory::ImpureAction;

	// Canvas layout coordinates
	Vector2 position;
	Vector2 size;

	// Binding information & Signature Versioning
	StringName target_symbol;    // ClassDB method name, variable name, or signal name
	uint32_t signature_hash = 0; // Hash of param names + types at save time to detect drift
	String macro_resource_path;  // Used when category == SubGraph ("res://macros/apply_damage.knit_macro")

	// Generic resolution table for wildcard pins
	HashMap<StringName, KnitTypeSignature> generic_bindings; // e.g. {"T": TypeSignature(Float)}

	Vector<KnitPin> input_pins;
	Vector<KnitPin> output_pins;

	KnitPin *find_pin(KnitPinID p_pin_id);
	const KnitPin *find_pin(KnitPinID p_pin_id) const;

	KnitPinID add_input_pin(const StringName &p_name, KnitPinKind p_kind, const KnitTypeSignature &p_type, const Variant &p_default_val = Variant());
	KnitPinID add_output_pin(const StringName &p_name, KnitPinKind p_kind, const KnitTypeSignature &p_type);

	void remove_pin(KnitPinID p_pin_id);

	KnitNode();
	~KnitNode();
};

struct KnitConnection {
	KnitConnectionID id = 0;
	KnitNodeID from_node = 0;
	KnitPinID from_pin = 0; // Output pin
	KnitNodeID to_node = 0;
	KnitPinID to_pin = 0;   // Input pin
};

struct KnitCommentBox {
	KnitNodeID id = 0;
	String title = "Comment";
	Color color = Color(0.2f, 0.2f, 0.2f, 0.8f);
	Rect2 bounds;
	Vector<KnitNodeID> enclosed_nodes;
};

struct KnitVariable {
	StringName name;
	KnitTypeSignature type;
	Variant default_value;
	bool is_exported = false; // Exposed to Inspector
};

class KnitsGraph : public Resource {
	GDCLASS(KnitsGraph, Resource);

protected:
	static void _bind_methods();

public:
	KnitGraphID id = 0;
	String graph_name = "MainGraph";
	bool is_function = false;
	KnitTypeSignature return_type;

	HashMap<KnitNodeID, Ref<KnitNode>> nodes;
	Vector<KnitConnection> connections;
	HashMap<StringName, KnitVariable> variables;
	Vector<KnitCommentBox> comments;

	Ref<KnitNode> create_node(KnitNodeCategory p_category, const String &p_title, const Vector2 &p_pos = Vector2());
	bool remove_node(KnitNodeID p_node_id);
	Ref<KnitNode> get_node(KnitNodeID p_node_id) const;

	KnitConnectionID connect_pins(KnitNodeID p_from_node, KnitPinID p_from_pin, KnitNodeID p_to_node, KnitPinID p_to_pin);
	bool disconnect_pins(KnitPinID p_from_pin, KnitPinID p_to_pin);
	bool disconnect_connection(KnitConnectionID p_connection_id);

	const KnitConnection *get_connection_for_input_pin(KnitPinID p_input_pin) const;
	Vector<const KnitConnection *> get_connections_for_output_pin(KnitPinID p_output_pin) const;

	void add_variable(const StringName &p_name, const KnitTypeSignature &p_type, const Variant &p_default_val = Variant(), bool p_exported = false);
	void remove_variable(const StringName &p_name);
	bool has_variable(const StringName &p_name) const;

	void add_comment_box(const String &p_title, const Rect2 &p_bounds, const Color &p_color = Color(0.2f, 0.2f, 0.2f, 0.8f));

	KnitsGraph();
	~KnitsGraph();
};
