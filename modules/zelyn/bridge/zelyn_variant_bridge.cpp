/**************************************************************************/
/*  zelyn_variant_bridge.cpp                                              */
/**************************************************************************/

#include "zelyn_variant_bridge.h"

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"

Value ZelynVariantBridge::make_object_handle(Object *p_object) {
	if (!p_object) {
		return Value();
	}
	ZelynEngineUserData *eud = new ZelynEngineUserData();
	eud->object_id = p_object->get_instance_id();
	eud->cached_ptr = p_object;
	eud->is_ref_counted = p_object->is_ref_counted();

	UserDataObject *udo = new UserDataObject(eud, ZELYN_USERDATA_OBJECT, &ZelynEngineUserData::finalizer);
	Value val;
	val.type = ValueType::Object;
	val.object = udo;
	return val;
}

Object *ZelynVariantBridge::get_object_from_handle(const Value &p_value) {
	if (p_value.type != ValueType::Object || !p_value.object) {
		return nullptr;
	}
	if (p_value.object->kind != HeapObject::Kind::UserData) {
		return nullptr;
	}
	UserDataObject *udo = static_cast<UserDataObject *>(p_value.object);
	if (udo->typeId != ZELYN_USERDATA_OBJECT || !udo->nativePtr) {
		return nullptr;
	}
	ZelynEngineUserData *eud = static_cast<ZelynEngineUserData *>(udo->nativePtr);
	return eud->get_valid_object();
}

Value ZelynVariantBridge::make_math_variant_handle(const Variant &p_variant) {
	ZelynEngineUserData *eud = new ZelynEngineUserData();
	eud->variant_payload = p_variant;

	UserDataObject *udo = new UserDataObject(eud, ZELYN_USERDATA_MATH_VARIANT, &ZelynEngineUserData::finalizer);
	Value val;
	val.type = ValueType::Object;
	val.object = udo;
	return val;
}

const Variant *ZelynVariantBridge::get_math_variant_from_handle(const Value &p_value) {
	if (p_value.type != ValueType::Object || !p_value.object) {
		return nullptr;
	}
	if (p_value.object->kind != HeapObject::Kind::UserData) {
		return nullptr;
	}
	UserDataObject *udo = static_cast<UserDataObject *>(p_value.object);
	if (udo->typeId != ZELYN_USERDATA_MATH_VARIANT || !udo->nativePtr) {
		return nullptr;
	}
	ZelynEngineUserData *eud = static_cast<ZelynEngineUserData *>(udo->nativePtr);
	return &eud->variant_payload;
}

Value ZelynVariantBridge::variant_to_zelyn(const Variant &p_variant) {
	switch (p_variant.get_type()) {
		case Variant::NIL:
			return Value();
		case Variant::BOOL:
			return Value(Value::BOOL, (bool)p_variant ? 1.0 : 0.0);
		case Variant::INT: {
			Value v;
			v.type = ValueType::Number;
			v.is_float = false;
			v.number = static_cast<double>((int64_t)p_variant);
			return v;
		}
		case Variant::FLOAT: {
			Value v;
			v.type = ValueType::Number;
			v.is_float = true;
			v.number = static_cast<double>(p_variant);
			return v;
		}
		case Variant::STRING: {
			String s = p_variant;
			CharString utf8 = s.utf8();
			return Value::makeString(std::string(utf8.get_data(), utf8.length()));
		}
		case Variant::STRING_NAME: {
			StringName sn = p_variant;
			String s = sn;
			CharString utf8 = s.utf8();
			uint32_t symId = SymbolTable::get().getOrCreate(std::string(utf8.get_data(), utf8.length()));
			Value v;
			v.type = ValueType::Symbol;
			v.symbolId = symId;
			return v;
		}
		case Variant::OBJECT: {
			Object *obj = p_variant.get_validated_object();
			return make_object_handle(obj);
		}
		case Variant::ARRAY: {
			Array arr = p_variant;
			ListObject *listObj = new ListObject();
			listObj->items.reserve(arr.size());
			for (int i = 0; i < arr.size(); i++) {
				listObj->items.push_back(variant_to_zelyn(arr[i]));
			}
			Value v;
			v.type = ValueType::Object;
			v.object = listObj;
			return v;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_variant;
			DictObject *dictObj = new DictObject();
			Array keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				String key_str = keys[i].stringify();
				CharString utf8 = key_str.utf8();
				uint32_t symId = SymbolTable::get().getOrCreate(std::string(utf8.get_data(), utf8.length()));
				dictObj->set(symId, variant_to_zelyn(dict[keys[i]]));
			}
			Value v;
			v.type = ValueType::Object;
			v.object = dictObj;
			return v;
		}
		default: {
			// Vector2, Vector3, Color, Transform3D, etc.
			return make_math_variant_handle(p_variant);
		}
	}
}

Variant ZelynVariantBridge::zelyn_to_variant(const Value &p_value) {
	switch (p_value.type.rawType) {
		case ValueType::Nil:
			return Variant();
		case ValueType::Bool:
			return Variant(p_value.number != 0.0);
		case ValueType::Number:
			if (p_value.is_float) {
				return Variant(p_value.number);
			} else {
				return Variant((int64_t)p_value.number);
			}
		case ValueType::Symbol: {
			const std::string &sym = SymbolTable::get().getString(p_value.symbolId);
			return StringName(sym.c_str());
		}
		case ValueType::Object: {
			if (!p_value.object) {
				return Variant();
			}
			switch (p_value.object->kind) {
				case HeapObject::Kind::String: {
					StringObject *strObj = static_cast<StringObject *>(p_value.object);
					return String::utf8(strObj->data.c_str(), strObj->data.length());
				}
				case HeapObject::Kind::List: {
					ListObject *listObj = static_cast<ListObject *>(p_value.object);
					Array arr;
					arr.resize(listObj->items.size());
					for (size_t i = 0; i < listObj->items.size(); i++) {
						arr[i] = zelyn_to_variant(listObj->items[i]);
					}
					return arr;
				}
				case HeapObject::Kind::Dict: {
					DictObject *dictObj = static_cast<DictObject *>(p_value.object);
					Dictionary dict;
					for (auto it = dictObj->entries.begin(); it != dictObj->entries.end(); ++it) {
						const std::string &sym = SymbolTable::get().getString(it.ptr->first);
						dict[StringName(sym.c_str())] = zelyn_to_variant(it.ptr->second);
					}
					return dict;
				}
				case HeapObject::Kind::UserData: {
					UserDataObject *udo = static_cast<UserDataObject *>(p_value.object);
					if (udo->typeId == ZELYN_USERDATA_OBJECT && udo->nativePtr) {
						ZelynEngineUserData *eud = static_cast<ZelynEngineUserData *>(udo->nativePtr);
						Object *obj = eud->get_valid_object();
						return obj ? Variant(obj) : Variant();
					} else if (udo->typeId == ZELYN_USERDATA_MATH_VARIANT && udo->nativePtr) {
						ZelynEngineUserData *eud = static_cast<ZelynEngineUserData *>(udo->nativePtr);
						return eud->variant_payload;
					}
					return Variant();
				}
				default:
					return Variant();
			}
		}
		default:
			return Variant();
	}
}
