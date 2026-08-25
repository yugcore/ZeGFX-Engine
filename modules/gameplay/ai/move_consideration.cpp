/**************************************************************************/
/*  move_consideration.cpp                                                */
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

#include "move_consideration.h"

real_t MoveConsideration::score(Node3D *p_self, Node3D *p_target, const Ref<AttributeSet> &p_self_attrs, const Ref<AttributeSet> &p_target_attrs, const Ref<GameplayTagContainer> &p_target_tags) const {
	if (metric == METRIC_CUSTOM_SCRIPT) {
		real_t custom_res = 0.0;
		if (GDVIRTUAL_CALL(_score, p_self, p_target, p_self_attrs, p_target_attrs, p_target_tags, custom_res)) {
			return CLAMP(custom_res, 0.0, 1.0) * weight;
		}
		return 0.0;
	}

	if (required_target_tags.is_valid()) {
		if (!required_target_tags->evaluate(p_target_tags)) {
			return 0.0;
		}
	}

	real_t raw_val = 0.0;
	switch (metric) {
		case METRIC_DISTANCE_TO_TARGET: {
			if (p_self && p_target) {
				Vector3 pos_self = p_self->is_inside_tree() ? p_self->get_global_position() : p_self->get_position();
				Vector3 pos_target = p_target->is_inside_tree() ? p_target->get_global_position() : p_target->get_position();
				raw_val = pos_self.distance_to(pos_target);
			}
		} break;
		case METRIC_TARGET_HEALTH_PCT: {
			if (p_target_attrs.is_valid()) {
				raw_val = p_target_attrs->get_attribute_percent("Health");
			}
		} break;
		case METRIC_SELF_HEALTH_PCT: {
			if (p_self_attrs.is_valid()) {
				raw_val = p_self_attrs->get_attribute_percent("Health");
			}
		} break;
		case METRIC_TARGET_POSTURE_PCT: {
			if (p_target_attrs.is_valid()) {
				raw_val = p_target_attrs->get_attribute_percent("Posture");
			}
		} break;
		case METRIC_SELF_POSTURE_PCT: {
			if (p_self_attrs.is_valid()) {
				raw_val = p_self_attrs->get_attribute_percent("Posture");
			}
		} break;
		case METRIC_TARGET_ATTACK_TELEGRAPH: {
			if (p_target_tags.is_valid() && p_target_tags->has_tag("Combat.Telegraph")) {
				raw_val = 1.0;
			} else {
				raw_val = 0.0;
			}
		} break;
		default:
			break;
	}

	// Normalize raw_val between min_range and max_range
	real_t range = max_range - min_range;
	real_t norm_val = 0.0;
	if (range > 0.0001) {
		norm_val = CLAMP((raw_val - min_range) / range, 0.0, 1.0);
	} else {
		norm_val = CLAMP(raw_val, 0.0, 1.0);
	}

	real_t curve_val = norm_val;
	if (response_curve.is_valid()) {
		curve_val = response_curve->sample(norm_val);
	}

	return CLAMP(curve_val, 0.0, 1.0) * weight;
}

void MoveConsideration::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_metric", "metric"), &MoveConsideration::set_metric);
	ClassDB::bind_method(D_METHOD("get_metric"), &MoveConsideration::get_metric);

	ClassDB::bind_method(D_METHOD("set_response_curve", "curve"), &MoveConsideration::set_response_curve);
	ClassDB::bind_method(D_METHOD("get_response_curve"), &MoveConsideration::get_response_curve);

	ClassDB::bind_method(D_METHOD("set_weight", "weight"), &MoveConsideration::set_weight);
	ClassDB::bind_method(D_METHOD("get_weight"), &MoveConsideration::get_weight);

	ClassDB::bind_method(D_METHOD("set_min_range", "val"), &MoveConsideration::set_min_range);
	ClassDB::bind_method(D_METHOD("get_min_range"), &MoveConsideration::get_min_range);

	ClassDB::bind_method(D_METHOD("set_max_range", "val"), &MoveConsideration::set_max_range);
	ClassDB::bind_method(D_METHOD("get_max_range"), &MoveConsideration::get_max_range);

	ClassDB::bind_method(D_METHOD("set_required_target_tags", "query"), &MoveConsideration::set_required_target_tags);
	ClassDB::bind_method(D_METHOD("get_required_target_tags"), &MoveConsideration::get_required_target_tags);

	ClassDB::bind_method(D_METHOD("score", "self", "target", "self_attrs", "target_attrs", "target_tags"), &MoveConsideration::score);
	GDVIRTUAL_BIND(_score, "self", "target", "self_attrs", "target_attrs", "target_tags");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "metric", PROPERTY_HINT_ENUM, "Distance To Target,Target Health Pct,Self Health Pct,Target Posture Pct,Self Posture Pct,Target Attack Telegraph,Custom Script"), "set_metric", "get_metric");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "response_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_response_curve", "get_response_curve");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_range"), "set_min_range", "get_min_range");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_range"), "set_max_range", "get_max_range");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "required_target_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_required_target_tags", "get_required_target_tags");

	BIND_ENUM_CONSTANT(METRIC_DISTANCE_TO_TARGET);
	BIND_ENUM_CONSTANT(METRIC_TARGET_HEALTH_PCT);
	BIND_ENUM_CONSTANT(METRIC_SELF_HEALTH_PCT);
	BIND_ENUM_CONSTANT(METRIC_TARGET_POSTURE_PCT);
	BIND_ENUM_CONSTANT(METRIC_SELF_POSTURE_PCT);
	BIND_ENUM_CONSTANT(METRIC_TARGET_ATTACK_TELEGRAPH);
	BIND_ENUM_CONSTANT(METRIC_CUSTOM_SCRIPT);
}
