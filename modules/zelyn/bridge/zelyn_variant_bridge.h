/**************************************************************************/
/*  zelyn_variant_bridge.h                                                */
/**************************************************************************/

#pragma once

#include "zelyn_user_data.h"
#include "core/variant/variant.h"
#include "zelyn/common.h"

class ZelynVariantBridge {
public:
	static Value variant_to_zelyn(const Variant &p_variant);
	static Variant zelyn_to_variant(const Value &p_value);

	static Value make_object_handle(Object *p_object);
	static Object *get_object_from_handle(const Value &p_value);

	static Value make_math_variant_handle(const Variant &p_variant);
	static const Variant *get_math_variant_from_handle(const Value &p_value);
};
