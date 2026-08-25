/**************************************************************************/
/*  move_consideration.h                                                  */
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

#include "../attributes/attribute_set.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/curve.h"

class MoveConsideration : public Resource {
	GDCLASS(MoveConsideration, Resource);

public:
	enum MetricType {
		METRIC_DISTANCE_TO_TARGET,
		METRIC_TARGET_HEALTH_PCT,
		METRIC_SELF_HEALTH_PCT,
		METRIC_TARGET_POSTURE_PCT,
		METRIC_SELF_POSTURE_PCT,
		METRIC_TARGET_ATTACK_TELEGRAPH,
		METRIC_CUSTOM_SCRIPT,
	};

private:
	MetricType metric = METRIC_DISTANCE_TO_TARGET;
	Ref<Curve> response_curve;
	real_t weight = 1.0;
	real_t min_range = 0.0;
	real_t max_range = 10.0;
	Ref<GameplayTagQuery> required_target_tags;

protected:
	static void _bind_methods();
	GDVIRTUAL5RC(real_t, _score, Node3D *, Node3D *, Ref<AttributeSet>, Ref<AttributeSet>, Ref<GameplayTagContainer>);

public:
	void set_metric(MetricType p_metric) { metric = p_metric; emit_changed(); }
	MetricType get_metric() const { return metric; }

	void set_response_curve(const Ref<Curve> &p_curve) { response_curve = p_curve; emit_changed(); }
	Ref<Curve> get_response_curve() const { return response_curve; }

	void set_weight(real_t p_weight) { weight = MAX(0.0, p_weight); emit_changed(); }
	real_t get_weight() const { return weight; }

	void set_min_range(real_t p_val) { min_range = p_val; emit_changed(); }
	real_t get_min_range() const { return min_range; }

	void set_max_range(real_t p_val) { max_range = p_val; emit_changed(); }
	real_t get_max_range() const { return max_range; }

	void set_required_target_tags(const Ref<GameplayTagQuery> &p_query) { required_target_tags = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_required_target_tags() const { return required_target_tags; }

	real_t score(Node3D *p_self, Node3D *p_target, const Ref<AttributeSet> &p_self_attrs, const Ref<AttributeSet> &p_target_attrs, const Ref<GameplayTagContainer> &p_target_tags) const;

	MoveConsideration() {}
	~MoveConsideration() {}
};

VARIANT_ENUM_CAST(MoveConsideration::MetricType);
