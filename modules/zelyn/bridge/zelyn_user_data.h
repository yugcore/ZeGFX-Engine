/**************************************************************************/
/*  zelyn_user_data.h                                                     */
/**************************************************************************/

#pragma once

#include "zelyn/common.h"
#include "core/object/object.h"
#include "core/object/object_id.h"
#include "core/variant/variant.h"

enum ZelynEngineUserDataType : uint32_t {
	ZELYN_USERDATA_OBJECT = 1,
	ZELYN_USERDATA_MATH_VARIANT = 2,
};

struct ZelynEngineUserData {
	ObjectID object_id;
	Object *cached_ptr = nullptr;
	bool is_ref_counted = false;
	Variant variant_payload;

	_FORCE_INLINE_ Object *get_valid_object() {
		if (object_id.is_null()) {
			cached_ptr = nullptr;
			return nullptr;
		}
		Object *live_obj = ObjectDB::get_instance(object_id);
		if (unlikely(live_obj != cached_ptr)) {
			cached_ptr = live_obj;
		}
		return live_obj;
	}

	static void finalizer(void *ptr) {
		if (ptr) {
			delete static_cast<ZelynEngineUserData *>(ptr);
		}
	}
};
