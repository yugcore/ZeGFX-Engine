/**************************************************************************/
/*  attribute_set.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "scene/resources/curve.h"

class AttributeModifier : public RefCounted {
	GDCLASS(AttributeModifier, RefCounted);

public:
	enum ModifierOp {
		OP_ADDITIVE_BASE,
		OP_ADDITIVE_BONUS,
		OP_MULTIPLICATIVE,
		OP_OVERRIDE,
	};

private:
	StringName target_attribute;
	ModifierOp op = OP_ADDITIVE_BONUS;
	real_t value = 0.0;
	int priority = 0;
	StringName source_id;

protected:
	static void _bind_methods();

public:
	void set_target_attribute(const StringName &p_name) { target_attribute = p_name; }
	StringName get_target_attribute() const { return target_attribute; }

	void set_op(ModifierOp p_op) { op = p_op; }
	ModifierOp get_op() const { return op; }

	void set_value(real_t p_val) { value = p_val; }
	real_t get_value() const { return value; }

	void set_priority(int p_prio) { priority = p_prio; }
	int get_priority() const { return priority; }

	void set_source_id(const StringName &p_id) { source_id = p_id; }
	StringName get_source_id() const { return source_id; }

	AttributeModifier() {}
	~AttributeModifier() {}
};

VARIANT_ENUM_CAST(AttributeModifier::ModifierOp);

struct AttributeData {
	StringName name;
	real_t base_value = 100.0;
	real_t min_value = 0.0;
	real_t max_value = 100.0;
	real_t current_value = 100.0;
	real_t regen_rate = 0.0; // per second
	real_t regen_delay = 0.0; // grace timer before regen starts
	real_t regen_timer = 0.0; // counts down to 0 after taking damage
	Ref<Curve> regen_step_curve;
	bool can_regen = true;
};

class AttributeSet : public Resource {
	GDCLASS(AttributeSet, Resource);

private:
	HashMap<StringName, AttributeData> attributes;
	Vector<Ref<AttributeModifier>> active_modifiers;
	HashMap<StringName, real_t> cut_rates; // Tag name -> damage multiplier (e.g. Damage.Slash -> 0.8)

	void _recalculate_attribute_max(const StringName &p_name);

protected:
	static void _bind_methods();

public:
	bool add_attribute(const StringName &p_name, real_t p_base, real_t p_min, real_t p_max, real_t p_regen = 0.0, real_t p_delay = 0.0, Ref<Curve> p_step_curve = Ref<Curve>());
	bool remove_attribute(const StringName &p_name);
	bool has_attribute(const StringName &p_name) const;

	real_t get_attribute_base(const StringName &p_name) const;
	void set_attribute_base(const StringName &p_name, real_t p_val);

	real_t get_attribute_current(const StringName &p_name) const;
	void set_attribute_current(const StringName &p_name, real_t p_val);

	real_t get_attribute_max(const StringName &p_name) const;
	void set_attribute_max(const StringName &p_name, real_t p_val);

	real_t get_attribute_min(const StringName &p_name) const;
	void set_attribute_min(const StringName &p_name, real_t p_val);

	real_t get_attribute_percent(const StringName &p_name) const;

	void set_attribute_regen_rate(const StringName &p_name, real_t p_rate);
	real_t get_attribute_regen_rate(const StringName &p_name) const;

	void set_attribute_regen_delay(const StringName &p_name, real_t p_delay);
	real_t get_attribute_regen_delay(const StringName &p_name) const;

	void set_attribute_step_curve(const StringName &p_name, const Ref<Curve> &p_curve);
	Ref<Curve> get_attribute_step_curve(const StringName &p_name) const;

	void set_attribute_can_regen(const StringName &p_name, bool p_can_regen);
	bool get_attribute_can_regen(const StringName &p_name) const;

	// Modifiers
	void add_modifier(const Ref<AttributeModifier> &p_modifier);
	void remove_modifier(const Ref<AttributeModifier> &p_modifier);
	void remove_modifiers_from_source(const StringName &p_source_id);
	void clear_modifiers();

	// Gameplay Math
	void tick_regen(real_t p_delta);
	real_t apply_damage(const StringName &p_name, real_t p_amount);
	real_t apply_healing(const StringName &p_name, real_t p_amount);

	// Damage Cut-Rates
	void set_cut_rate(const StringName &p_tag, real_t p_multiplier);
	real_t get_cut_rate(const StringName &p_tag) const;
	PackedStringArray get_cut_rate_tags() const;

	PackedStringArray get_attribute_names() const;

	Ref<AttributeSet> duplicate_set() const;

	AttributeSet();
	~AttributeSet();
};
