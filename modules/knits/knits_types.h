/**************************************************************************/
/*  knits_types.h                                                         */
/**************************************************************************/

#pragma once

#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "core/object/object_id.h"
#include "core/object/ref_counted.h"
#include "core/os/os.h"
#include "core/string/string_name.h"
#include "core/variant/variant.h"
#include <cstdint>

// 64-bit random unique identifiers (zero merge conflicts across git branches)
using KnitNodeID       = uint64_t;
using KnitPinID        = uint64_t;
using KnitConnectionID = uint64_t;
using KnitGraphID      = uint64_t;

class KnitIDGenerator {
public:
	_FORCE_INLINE_ static uint64_t generate() {
		static thread_local RandomPCG rng(OS::get_singleton() ? OS::get_singleton()->get_ticks_usec() : 123456789ULL);
		uint64_t high = uint64_t(rng.rand());
		uint64_t low = uint64_t(rng.rand());
		uint64_t id = (high << 32) | low;
		if (id == 0) {
			id = 1; // Ensure non-zero ID
		}
		return id;
	}
};

// Visual & Semantic category of the pin
enum class KnitPinKind : uint8_t {
	Execution, // Control flow (Chevron/arrow)
	Data       // Typed data payload (Color-coded circle)
};

enum class KnitPinDirection : uint8_t {
	Input,
	Output
};

// Strongly-typed data types mapped to Engine Variant types
enum class KnitDataType : uint16_t {
	Void = 0,
	Execution,
	Bool,
	Int32,
	Int64,
	Float,
	Double,
	String,
	StringName,
	Vector2,
	Vector3,
	Color,
	Transform3D,
	ObjectRef,   // Engine Object* / Entity handle
	Enum,        // Strongly typed enum
	Array,       // Generic typed array: Array<T>
	Dictionary,  // Key-value map: Dict<K, V>
	Wildcard     // Generic unconstrained pin (Resolved via propagation pass)
};

struct KnitTypeSignature {
	KnitDataType kind = KnitDataType::Void;
	StringName custom_type_name; // Native class name for ObjectRef (e.g. "CharacterBody3D"), Enum name, or Struct name
	StringName generic_symbol;   // E.g. "T", "TKey", "TValue" when kind == Wildcard
	Ref<RefCounted> inner_type;  // For Array<T>
	Ref<RefCounted> key_type;    // For Dict Key

	_FORCE_INLINE_ bool is_compatible_with(const KnitTypeSignature &p_other) const {
		if (kind == KnitDataType::Wildcard || p_other.kind == KnitDataType::Wildcard) {
			return true;
		}
		if (kind != p_other.kind) {
			// Strict type promotion rules
			if ((kind == KnitDataType::Int32 && p_other.kind == KnitDataType::Float) ||
				(kind == KnitDataType::Float && p_other.kind == KnitDataType::Double) ||
				(kind == KnitDataType::Int32 && p_other.kind == KnitDataType::Int64) ||
				(kind == KnitDataType::String && p_other.kind == KnitDataType::StringName)) {
				return true;
			}
			return false;
		}
		if (kind == KnitDataType::ObjectRef) {
			if (custom_type_name == StringName() || p_other.custom_type_name == StringName()) {
				return true;
			}
			if (custom_type_name == p_other.custom_type_name) {
				return true;
			}
			// Native ClassDB hierarchy reflection check
			return ClassDB::is_parent_class(p_other.custom_type_name, custom_type_name);
		}
		return true;
	}

	_FORCE_INLINE_ static KnitDataType from_variant_type(Variant::Type p_type) {
		switch (p_type) {
			case Variant::NIL: return KnitDataType::Void;
			case Variant::BOOL: return KnitDataType::Bool;
			case Variant::INT: return KnitDataType::Int64;
			case Variant::FLOAT: return KnitDataType::Double;
			case Variant::STRING: return KnitDataType::String;
			case Variant::STRING_NAME: return KnitDataType::StringName;
			case Variant::VECTOR2: return KnitDataType::Vector2;
			case Variant::VECTOR3: return KnitDataType::Vector3;
			case Variant::COLOR: return KnitDataType::Color;
			case Variant::TRANSFORM3D: return KnitDataType::Transform3D;
			case Variant::OBJECT: return KnitDataType::ObjectRef;
			case Variant::ARRAY: return KnitDataType::Array;
			case Variant::DICTIONARY: return KnitDataType::Dictionary;
			default: return KnitDataType::Void;
		}
	}
};

// Resumable Coroutine State-Machine Frame Data (Tier 1 Architecture)
struct KnitFrameKey {
	ObjectID instance_id;
	KnitGraphID graph_id = 0;
	uint32_t coroutine_id = 0;

	_FORCE_INLINE_ bool operator==(const KnitFrameKey &p_other) const {
		return instance_id == p_other.instance_id && graph_id == p_other.graph_id && coroutine_id == p_other.coroutine_id;
	}

	_FORCE_INLINE_ uint32_t hash() const {
		uint32_t h = hash_murmur3_one_64(instance_id);
		h = hash_murmur3_one_64(graph_id, h);
		h = hash_murmur3_one_32(coroutine_id, h);
		return h;
	}
};

struct KnitExecutionFrame {
	KnitNodeID owner_node = 0;
	uint32_t pc = 0;                     // Bytecode resume offset
	Variant registers[16];               // Snapshot of active registers at suspension point
	float yield_timer_remaining = 0.0f;  // Seconds remaining (for YIELD_SECONDS)
	uint32_t yield_frames_remaining = 0; // Frames remaining (for YIELD_FRAMES)
};
