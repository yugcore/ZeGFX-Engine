/**************************************************************************/
/*  damage_resolver.h                                                     */
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
#include "../attributes/gameplay_effect.h"
#include "../core/gameplay_tags.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

class Hitbox3D;
class Hurtbox3D;

class DamageResolver : public Object {
	GDCLASS(DamageResolver, Object);

public:
	enum HitOutcome {
		OUTCOME_HIT,
		OUTCOME_BLOCK,
		OUTCOME_PARRY,
		OUTCOME_EVADED,
		OUTCOME_IMMUNE,
	};

private:
	static DamageResolver *singleton;

protected:
	static void _bind_methods();

public:
	static DamageResolver *get_singleton() { return singleton; }

	Dictionary resolve_hit(Hitbox3D *p_hitbox, Hurtbox3D *p_hurtbox, const Vector3 &p_hit_point = Vector3(), const Vector3 &p_hit_normal = Vector3(0, 1, 0));
	Dictionary resolve_interaction(const Ref<AttributeSet> &p_attacker_attrs, const Ref<GameplayTagContainer> &p_attacker_tags, const Dictionary &p_damage_channels, const Ref<AttributeSet> &p_defender_attrs, const Ref<GameplayTagContainer> &p_defender_tags, const Vector3 &p_hit_point = Vector3(), const Vector3 &p_hit_normal = Vector3(0, 1, 0));

	DamageResolver();
	~DamageResolver();
};

VARIANT_ENUM_CAST(DamageResolver::HitOutcome);
