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
	Vector2i,
	Rect2,
	Rect2i,
	Vector3,
	Vector3i,
	Transform2D,
	Vector4,
	Vector4i,
	Plane,
	Quaternion,
	AABB,
	Basis,
	Transform3D,
	Projection,
	Color,
	NodePath,
	RID,
	ObjectRef,   // Engine Object* / Entity handle
	Callable,
	Signal,
	Dictionary,  // Key-value map: Dict<K, V>
	Array,       // Generic typed array: Array<T>
	PackedByteArray,
	PackedInt32Array,
	PackedInt64Array,
	PackedFloat32Array,
	PackedFloat64Array,
	PackedStringArray,
	PackedVector2Array,
	PackedVector3Array,
	PackedColorArray,
	PackedVector4Array,
	Enum,        // Strongly typed enum
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
			case Variant::VECTOR2I: return KnitDataType::Vector2i;
			case Variant::RECT2: return KnitDataType::Rect2;
			case Variant::RECT2I: return KnitDataType::Rect2i;
			case Variant::VECTOR3: return KnitDataType::Vector3;
			case Variant::VECTOR3I: return KnitDataType::Vector3i;
			case Variant::TRANSFORM2D: return KnitDataType::Transform2D;
			case Variant::VECTOR4: return KnitDataType::Vector4;
			case Variant::VECTOR4I: return KnitDataType::Vector4i;
			case Variant::PLANE: return KnitDataType::Plane;
			case Variant::QUATERNION: return KnitDataType::Quaternion;
			case Variant::AABB: return KnitDataType::AABB;
			case Variant::BASIS: return KnitDataType::Basis;
			case Variant::TRANSFORM3D: return KnitDataType::Transform3D;
			case Variant::PROJECTION: return KnitDataType::Projection;
			case Variant::COLOR: return KnitDataType::Color;
			case Variant::NODE_PATH: return KnitDataType::NodePath;
			case Variant::RID: return KnitDataType::RID;
			case Variant::OBJECT: return KnitDataType::ObjectRef;
			case Variant::CALLABLE: return KnitDataType::Callable;
			case Variant::SIGNAL: return KnitDataType::Signal;
			case Variant::DICTIONARY: return KnitDataType::Dictionary;
			case Variant::ARRAY: return KnitDataType::Array;
			case Variant::PACKED_BYTE_ARRAY: return KnitDataType::PackedByteArray;
			case Variant::PACKED_INT32_ARRAY: return KnitDataType::PackedInt32Array;
			case Variant::PACKED_INT64_ARRAY: return KnitDataType::PackedInt64Array;
			case Variant::PACKED_FLOAT32_ARRAY: return KnitDataType::PackedFloat32Array;
			case Variant::PACKED_FLOAT64_ARRAY: return KnitDataType::PackedFloat64Array;
			case Variant::PACKED_STRING_ARRAY: return KnitDataType::PackedStringArray;
			case Variant::PACKED_VECTOR2_ARRAY: return KnitDataType::PackedVector2Array;
			case Variant::PACKED_VECTOR3_ARRAY: return KnitDataType::PackedVector3Array;
			case Variant::PACKED_COLOR_ARRAY: return KnitDataType::PackedColorArray;
			case Variant::PACKED_VECTOR4_ARRAY: return KnitDataType::PackedVector4Array;
			default: return KnitDataType::Void;
		}
	}

	_FORCE_INLINE_ static Variant::Type to_variant_type(KnitDataType p_type) {
		switch (p_type) {
			case KnitDataType::Bool: return Variant::BOOL;
			case KnitDataType::Int32:
			case KnitDataType::Int64: return Variant::INT;
			case KnitDataType::Float:
			case KnitDataType::Double: return Variant::FLOAT;
			case KnitDataType::String: return Variant::STRING;
			case KnitDataType::StringName: return Variant::STRING_NAME;
			case KnitDataType::Vector2: return Variant::VECTOR2;
			case KnitDataType::Vector2i: return Variant::VECTOR2I;
			case KnitDataType::Rect2: return Variant::RECT2;
			case KnitDataType::Rect2i: return Variant::RECT2I;
			case KnitDataType::Vector3: return Variant::VECTOR3;
			case KnitDataType::Vector3i: return Variant::VECTOR3I;
			case KnitDataType::Transform2D: return Variant::TRANSFORM2D;
			case KnitDataType::Vector4: return Variant::VECTOR4;
			case KnitDataType::Vector4i: return Variant::VECTOR4I;
			case KnitDataType::Plane: return Variant::PLANE;
			case KnitDataType::Quaternion: return Variant::QUATERNION;
			case KnitDataType::AABB: return Variant::AABB;
			case KnitDataType::Basis: return Variant::BASIS;
			case KnitDataType::Transform3D: return Variant::TRANSFORM3D;
			case KnitDataType::Projection: return Variant::PROJECTION;
			case KnitDataType::Color: return Variant::COLOR;
			case KnitDataType::NodePath: return Variant::NODE_PATH;
			case KnitDataType::RID: return Variant::RID;
			case KnitDataType::ObjectRef: return Variant::OBJECT;
			case KnitDataType::Callable: return Variant::CALLABLE;
			case KnitDataType::Signal: return Variant::SIGNAL;
			case KnitDataType::Dictionary: return Variant::DICTIONARY;
			case KnitDataType::Array: return Variant::ARRAY;
			case KnitDataType::PackedByteArray: return Variant::PACKED_BYTE_ARRAY;
			case KnitDataType::PackedInt32Array: return Variant::PACKED_INT32_ARRAY;
			case KnitDataType::PackedInt64Array: return Variant::PACKED_INT64_ARRAY;
			case KnitDataType::PackedFloat32Array: return Variant::PACKED_FLOAT32_ARRAY;
			case KnitDataType::PackedFloat64Array: return Variant::PACKED_FLOAT64_ARRAY;
			case KnitDataType::PackedStringArray: return Variant::PACKED_STRING_ARRAY;
			case KnitDataType::PackedVector2Array: return Variant::PACKED_VECTOR2_ARRAY;
			case KnitDataType::PackedVector3Array: return Variant::PACKED_VECTOR3_ARRAY;
			case KnitDataType::PackedColorArray: return Variant::PACKED_COLOR_ARRAY;
			case KnitDataType::PackedVector4Array: return Variant::PACKED_VECTOR4_ARRAY;
			default: return Variant::NIL;
		}
	}

	_FORCE_INLINE_ static KnitTypeSignature from_property_info(const PropertyInfo &p_info) {
		KnitTypeSignature sig;
		sig.kind = from_variant_type(p_info.type);
		if (p_info.type == Variant::OBJECT && p_info.class_name != StringName()) {
			sig.custom_type_name = p_info.class_name;
		}
		return sig;
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
