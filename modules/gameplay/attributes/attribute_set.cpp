/**************************************************************************/
/*  attribute_set.cpp                                                     */
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

#include "attribute_set.h"

// -----------------------------------------------------------------------------
// AttributeModifier
// -----------------------------------------------------------------------------

void AttributeModifier::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_attribute", "name"), &AttributeModifier::set_target_attribute);
	ClassDB::bind_method(D_METHOD("get_target_attribute"), &AttributeModifier::get_target_attribute);

	ClassDB::bind_method(D_METHOD("set_op", "op"), &AttributeModifier::set_op);
	ClassDB::bind_method(D_METHOD("get_op"), &AttributeModifier::get_op);

	ClassDB::bind_method(D_METHOD("set_value", "value"), &AttributeModifier::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &AttributeModifier::get_value);

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AttributeModifier::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &AttributeModifier::get_priority);

	ClassDB::bind_method(D_METHOD("set_source_id", "source_id"), &AttributeModifier::set_source_id);
	ClassDB::bind_method(D_METHOD("get_source_id"), &AttributeModifier::get_source_id);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_attribute"), "set_target_attribute", "get_target_attribute");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "op", PROPERTY_HINT_ENUM, "Additive Base,Additive Bonus,Multiplicative,Override"), "set_op", "get_op");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value"), "set_value", "get_value");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "source_id"), "set_source_id", "get_source_id");

	BIND_ENUM_CONSTANT(OP_ADDITIVE_BASE);
	BIND_ENUM_CONSTANT(OP_ADDITIVE_BONUS);
	BIND_ENUM_CONSTANT(OP_MULTIPLICATIVE);
	BIND_ENUM_CONSTANT(OP_OVERRIDE);
}

// -----------------------------------------------------------------------------
// AttributeSet
// -----------------------------------------------------------------------------

void AttributeSet::_recalculate_attribute_max(const StringName &p_name) {
	if (!attributes.has(p_name)) {
		return;
	}
	AttributeData &data = attributes[p_name];

	real_t base_accum = data.base_value;
	real_t bonus_accum = 0.0;
	real_t mult_accum = 0.0;
	bool has_override = false;
	real_t override_val = 0.0;
	int highest_prio = -999999;

	for (int i = 0; i < active_modifiers.size(); i++) {
		Ref<AttributeModifier> mod = active_modifiers[i];
		if (mod.is_valid() && mod->get_target_attribute() == p_name) {
			switch (mod->get_op()) {
				case AttributeModifier::OP_ADDITIVE_BASE:
					base_accum += mod->get_value();
					break;
				case AttributeModifier::OP_ADDITIVE_BONUS:
					bonus_accum += mod->get_value();
					break;
				case AttributeModifier::OP_MULTIPLICATIVE:
					mult_accum += mod->get_value();
					break;
				case AttributeModifier::OP_OVERRIDE:
					if (!has_override || mod->get_priority() >= highest_prio) {
						has_override = true;
						override_val = mod->get_value();
						highest_prio = mod->get_priority();
					}
					break;
			}
		}
	}

	real_t new_max = (base_accum + bonus_accum) * (1.0 + mult_accum);
	if (has_override) {
		new_max = override_val;
	}
	new_max = CLAMP(new_max, data.min_value, 999999999.0);
	data.max_value = new_max;

	if (data.current_value > data.max_value) {
		data.current_value = data.max_value;
	}
}

bool AttributeSet::add_attribute(const StringName &p_name, real_t p_base, real_t p_min, real_t p_max, real_t p_regen, real_t p_delay, Ref<Curve> p_step_curve) {
	if (p_name == StringName() || attributes.has(p_name)) {
		return false;
	}
	AttributeData data;
	data.name = p_name;
	data.base_value = p_base;
	data.min_value = p_min;
	data.max_value = p_max;
	data.current_value = p_max;
	data.regen_rate = p_regen;
	data.regen_delay = p_delay;
	data.regen_timer = 0.0;
	data.regen_step_curve = p_step_curve;
	data.can_regen = true;

	attributes[p_name] = data;
	_recalculate_attribute_max(p_name);
	emit_changed();
	return true;
}

bool AttributeSet::remove_attribute(const StringName &p_name) {
	if (attributes.erase(p_name)) {
		emit_changed();
		return true;
	}
	return false;
}

bool AttributeSet::has_attribute(const StringName &p_name) const {
	return attributes.has(p_name);
}

real_t AttributeSet::get_attribute_base(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].base_value;
	}
	return 0.0;
}

void AttributeSet::set_attribute_base(const StringName &p_name, real_t p_val) {
	if (attributes.has(p_name)) {
		attributes[p_name].base_value = p_val;
		_recalculate_attribute_max(p_name);
		emit_changed();
	}
}

real_t AttributeSet::get_attribute_current(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].current_value;
	}
	return 0.0;
}

void AttributeSet::set_attribute_current(const StringName &p_name, real_t p_val) {
	if (attributes.has(p_name)) {
		AttributeData &data = attributes[p_name];
		real_t old_val = data.current_value;
		data.current_value = CLAMP(p_val, data.min_value, data.max_value);
		if (!Math::is_equal_approx(old_val, data.current_value)) {
			emit_signal(SNAME("attribute_changed"), p_name, old_val, data.current_value);
			if (Math::is_equal_approx(data.current_value, data.min_value)) {
				emit_signal(SNAME("attribute_depleted"), p_name);
			} else if (Math::is_equal_approx(data.current_value, data.max_value)) {
				emit_signal(SNAME("attribute_full"), p_name);
			}
			emit_changed();
		}
	}
}

real_t AttributeSet::get_attribute_max(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].max_value;
	}
	return 0.0;
}

void AttributeSet::set_attribute_max(const StringName &p_name, real_t p_val) {
	if (attributes.has(p_name)) {
		attributes[p_name].max_value = p_val;
		if (attributes[p_name].current_value > p_val) {
			set_attribute_current(p_name, p_val);
		}
		emit_changed();
	}
}

real_t AttributeSet::get_attribute_min(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].min_value;
	}
	return 0.0;
}

void AttributeSet::set_attribute_min(const StringName &p_name, real_t p_val) {
	if (attributes.has(p_name)) {
		attributes[p_name].min_value = p_val;
		if (attributes[p_name].current_value < p_val) {
			set_attribute_current(p_name, p_val);
		}
		emit_changed();
	}
}

real_t AttributeSet::get_attribute_percent(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		const AttributeData &data = attributes[p_name];
		real_t range = data.max_value - data.min_value;
		if (range > 0.0001) {
			return (data.current_value - data.min_value) / range;
		}
	}
	return 0.0;
}

void AttributeSet::set_attribute_regen_rate(const StringName &p_name, real_t p_rate) {
	if (attributes.has(p_name)) {
		attributes[p_name].regen_rate = p_rate;
		emit_changed();
	}
}

real_t AttributeSet::get_attribute_regen_rate(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].regen_rate;
	}
	return 0.0;
}

void AttributeSet::set_attribute_regen_delay(const StringName &p_name, real_t p_delay) {
	if (attributes.has(p_name)) {
		attributes[p_name].regen_delay = p_delay;
		emit_changed();
	}
}

real_t AttributeSet::get_attribute_regen_delay(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].regen_delay;
	}
	return 0.0;
}

void AttributeSet::set_attribute_step_curve(const StringName &p_name, const Ref<Curve> &p_curve) {
	if (attributes.has(p_name)) {
		attributes[p_name].regen_step_curve = p_curve;
		emit_changed();
	}
}

Ref<Curve> AttributeSet::get_attribute_step_curve(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].regen_step_curve;
	}
	return Ref<Curve>();
}

void AttributeSet::set_attribute_can_regen(const StringName &p_name, bool p_can_regen) {
	if (attributes.has(p_name)) {
		attributes[p_name].can_regen = p_can_regen;
	}
}

bool AttributeSet::get_attribute_can_regen(const StringName &p_name) const {
	if (attributes.has(p_name)) {
		return attributes[p_name].can_regen;
	}
	return false;
}

void AttributeSet::add_modifier(const Ref<AttributeModifier> &p_modifier) {
	if (p_modifier.is_null()) {
		return;
	}
	if (!active_modifiers.has(p_modifier)) {
		active_modifiers.push_back(p_modifier);
		_recalculate_attribute_max(p_modifier->get_target_attribute());
		emit_changed();
	}
}

void AttributeSet::remove_modifier(const Ref<AttributeModifier> &p_modifier) {
	if (active_modifiers.has(p_modifier)) {
		StringName target = p_modifier->get_target_attribute();
		active_modifiers.erase(p_modifier);
		_recalculate_attribute_max(target);
		emit_changed();
	}
}

void AttributeSet::remove_modifiers_from_source(const StringName &p_source_id) {
	bool changed = false;
	HashSet<StringName> affected_attrs;

	for (int i = active_modifiers.size() - 1; i >= 0; i--) {
		Ref<AttributeModifier> mod = active_modifiers[i];
		if (mod.is_valid() && mod->get_source_id() == p_source_id) {
			affected_attrs.insert(mod->get_target_attribute());
			active_modifiers.remove_at(i);
			changed = true;
		}
	}

	if (changed) {
		for (const StringName &attr : affected_attrs) {
			_recalculate_attribute_max(attr);
		}
		emit_changed();
	}
}

void AttributeSet::clear_modifiers() {
	active_modifiers.clear();
	for (const KeyValue<StringName, AttributeData> &E : attributes) {
		_recalculate_attribute_max(E.key);
	}
	emit_changed();
}

void AttributeSet::tick_regen(real_t p_delta) {
	for (KeyValue<StringName, AttributeData> &E : attributes) {
		AttributeData &data = E.value;
		if (!data.can_regen || data.regen_rate <= 0.0) {
			continue;
		}

		if (data.regen_timer > 0.0) {
			data.regen_timer -= p_delta;
			if (data.regen_timer > 0.0) {
				continue;
			}
		}

		real_t mult = 1.0;
		if (data.regen_step_curve.is_valid()) {
			real_t pct = get_attribute_percent(E.key);
			mult = data.regen_step_curve->sample(pct);
		}

		if (data.current_value < data.max_value) {
			real_t new_val = data.current_value + (data.regen_rate * mult * p_delta);
			set_attribute_current(E.key, new_val);
		}
	}
}

real_t AttributeSet::apply_damage(const StringName &p_name, real_t p_amount) {
	if (!attributes.has(p_name)) {
		return 0.0;
	}
	AttributeData &data = attributes[p_name];
	data.regen_timer = data.regen_delay; // Reset grace delay
	real_t prev = data.current_value;
	set_attribute_current(p_name, data.current_value - p_amount);
	return prev - data.current_value;
}

real_t AttributeSet::apply_healing(const StringName &p_name, real_t p_amount) {
	if (!attributes.has(p_name)) {
		return 0.0;
	}
	AttributeData &data = attributes[p_name];
	real_t prev = data.current_value;
	set_attribute_current(p_name, data.current_value + p_amount);
	return data.current_value - prev;
}

void AttributeSet::set_cut_rate(const StringName &p_tag, real_t p_multiplier) {
	cut_rates[p_tag] = p_multiplier;
	emit_changed();
}

real_t AttributeSet::get_cut_rate(const StringName &p_tag) const {
	if (cut_rates.has(p_tag)) {
		return cut_rates[p_tag];
	}
	return 1.0; // Default 100% damage (no mitigation)
}

PackedStringArray AttributeSet::get_cut_rate_tags() const {
	PackedStringArray arr;
	arr.resize(cut_rates.size());
	int i = 0;
	for (const KeyValue<StringName, real_t> &E : cut_rates) {
		arr.set(i++, String(E.key));
	}
	return arr;
}

PackedStringArray AttributeSet::get_attribute_names() const {
	PackedStringArray arr;
	arr.resize(attributes.size());
	int i = 0;
	for (const KeyValue<StringName, AttributeData> &E : attributes) {
		arr.set(i++, String(E.key));
	}
	return arr;
}

Ref<AttributeSet> AttributeSet::duplicate_set() const {
	Ref<AttributeSet> copy;
	copy.instantiate();
	copy->attributes = attributes;
	copy->active_modifiers = active_modifiers;
	copy->cut_rates = cut_rates;
	return copy;
}

void AttributeSet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_attribute", "name", "base", "min", "max", "regen", "delay", "step_curve"), &AttributeSet::add_attribute, DEFVAL(0.0), DEFVAL(0.0), DEFVAL(Ref<Curve>()));
	ClassDB::bind_method(D_METHOD("remove_attribute", "name"), &AttributeSet::remove_attribute);
	ClassDB::bind_method(D_METHOD("has_attribute", "name"), &AttributeSet::has_attribute);

	ClassDB::bind_method(D_METHOD("get_attribute_base", "name"), &AttributeSet::get_attribute_base);
	ClassDB::bind_method(D_METHOD("set_attribute_base", "name", "value"), &AttributeSet::set_attribute_base);

	ClassDB::bind_method(D_METHOD("get_attribute_current", "name"), &AttributeSet::get_attribute_current);
	ClassDB::bind_method(D_METHOD("set_attribute_current", "name", "value"), &AttributeSet::set_attribute_current);

	ClassDB::bind_method(D_METHOD("get_attribute_max", "name"), &AttributeSet::get_attribute_max);
	ClassDB::bind_method(D_METHOD("set_attribute_max", "name", "value"), &AttributeSet::set_attribute_max);

	ClassDB::bind_method(D_METHOD("get_attribute_min", "name"), &AttributeSet::get_attribute_min);
	ClassDB::bind_method(D_METHOD("set_attribute_min", "name", "value"), &AttributeSet::set_attribute_min);

	ClassDB::bind_method(D_METHOD("get_attribute_percent", "name"), &AttributeSet::get_attribute_percent);

	ClassDB::bind_method(D_METHOD("set_attribute_regen_rate", "name", "rate"), &AttributeSet::set_attribute_regen_rate);
	ClassDB::bind_method(D_METHOD("get_attribute_regen_rate", "name"), &AttributeSet::get_attribute_regen_rate);

	ClassDB::bind_method(D_METHOD("set_attribute_regen_delay", "name", "delay"), &AttributeSet::set_attribute_regen_delay);
	ClassDB::bind_method(D_METHOD("get_attribute_regen_delay", "name"), &AttributeSet::get_attribute_regen_delay);

	ClassDB::bind_method(D_METHOD("set_attribute_step_curve", "name", "curve"), &AttributeSet::set_attribute_step_curve);
	ClassDB::bind_method(D_METHOD("get_attribute_step_curve", "name"), &AttributeSet::get_attribute_step_curve);

	ClassDB::bind_method(D_METHOD("set_attribute_can_regen", "name", "can_regen"), &AttributeSet::set_attribute_can_regen);
	ClassDB::bind_method(D_METHOD("get_attribute_can_regen", "name"), &AttributeSet::get_attribute_can_regen);

	ClassDB::bind_method(D_METHOD("add_modifier", "modifier"), &AttributeSet::add_modifier);
	ClassDB::bind_method(D_METHOD("remove_modifier", "modifier"), &AttributeSet::remove_modifier);
	ClassDB::bind_method(D_METHOD("remove_modifiers_from_source", "source_id"), &AttributeSet::remove_modifiers_from_source);
	ClassDB::bind_method(D_METHOD("clear_modifiers"), &AttributeSet::clear_modifiers);

	ClassDB::bind_method(D_METHOD("tick_regen", "delta"), &AttributeSet::tick_regen);
	ClassDB::bind_method(D_METHOD("apply_damage", "name", "amount"), &AttributeSet::apply_damage);
	ClassDB::bind_method(D_METHOD("apply_healing", "name", "amount"), &AttributeSet::apply_healing);

	ClassDB::bind_method(D_METHOD("set_cut_rate", "tag", "multiplier"), &AttributeSet::set_cut_rate);
	ClassDB::bind_method(D_METHOD("get_cut_rate", "tag"), &AttributeSet::get_cut_rate);
	ClassDB::bind_method(D_METHOD("get_cut_rate_tags"), &AttributeSet::get_cut_rate_tags);

	ClassDB::bind_method(D_METHOD("get_attribute_names"), &AttributeSet::get_attribute_names);
	ClassDB::bind_method(D_METHOD("duplicate_set"), &AttributeSet::duplicate_set);

	ADD_SIGNAL(MethodInfo("attribute_changed", PropertyInfo(Variant::STRING_NAME, "name"), PropertyInfo(Variant::FLOAT, "old_value"), PropertyInfo(Variant::FLOAT, "new_value")));
	ADD_SIGNAL(MethodInfo("attribute_depleted", PropertyInfo(Variant::STRING_NAME, "name")));
	ADD_SIGNAL(MethodInfo("attribute_full", PropertyInfo(Variant::STRING_NAME, "name")));
}

AttributeSet::AttributeSet() {}
AttributeSet::~AttributeSet() {}
