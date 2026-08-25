/**************************************************************************/
/*  damage_resolver.cpp                                                   */
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

#include "damage_resolver.h"
#include "hitbox_3d.h"
#include "hurtbox_3d.h"

DamageResolver *DamageResolver::singleton = nullptr;

Dictionary DamageResolver::resolve_hit(Hitbox3D *p_hitbox, Hurtbox3D *p_hurtbox, const Vector3 &p_hit_point, const Vector3 &p_hit_normal) {
	if (!p_hitbox || !p_hurtbox) {
		Dictionary result;
		result["outcome"] = OUTCOME_IMMUNE;
		return result;
	}

	return resolve_interaction(
			p_hitbox->get_instigator_attributes(),
			p_hitbox->get_attack_tags(),
			p_hitbox->get_damage_channels(),
			p_hurtbox->get_attribute_set(),
			p_hurtbox->get_state_tags(),
			p_hit_point,
			p_hit_normal);
}

Dictionary DamageResolver::resolve_interaction(const Ref<AttributeSet> &p_attacker_attrs, const Ref<GameplayTagContainer> &p_attacker_tags, const Dictionary &p_damage_channels, const Ref<AttributeSet> &p_defender_attrs, const Ref<GameplayTagContainer> &p_defender_tags, const Vector3 &p_hit_point, const Vector3 &p_hit_normal) {
	Dictionary result;

	// 1. Invulnerability Check
	if (p_defender_tags.is_valid() && (p_defender_tags->has_tag("Combat.State.Invulnerable") || p_defender_tags->has_tag("Status.Invulnerable"))) {
		result["outcome"] = OUTCOME_IMMUNE;
		return result;
	}

	// 2. Parry Check
	bool is_unparryable = (p_attacker_tags.is_valid() && (p_attacker_tags->has_tag("Combat.Unparryable") || p_attacker_tags->has_tag("Combat.Telegraph.Grab")));
	if (!is_unparryable && p_defender_tags.is_valid() && p_defender_tags->has_tag("Combat.State.Parrying")) {
		result["outcome"] = OUTCOME_PARRY;
		real_t parry_posture_recoil = 50.0;
		if (p_attacker_attrs.is_valid() && p_attacker_attrs->has_attribute("Posture")) {
			p_attacker_attrs->apply_damage("Posture", parry_posture_recoil);
		}
		result["parry_recoil_inflicted"] = parry_posture_recoil;
		return result;
	}

	// 3. Block Check
	bool is_unblockable = (p_attacker_tags.is_valid() && (p_attacker_tags->has_tag("Combat.Unblockable") || p_attacker_tags->has_tag("Combat.Telegraph.Grab") || p_attacker_tags->has_tag("Combat.Telegraph.Sweep")));
	if (!is_unblockable && p_defender_tags.is_valid() && (p_defender_tags->has_tag("Combat.State.Guarding") || p_defender_tags->has_tag("Combat.State.Blocking"))) {
		result["outcome"] = OUTCOME_BLOCK;

		Ref<StandardDamageExecution> exec;
		exec.instantiate();
		Dictionary params;
		params["damage_channels"] = p_damage_channels;
		Dictionary calc = exec->execute_calculation(p_attacker_attrs, p_defender_attrs, params);

		real_t blocked_posture_dmg = real_t(calc.get("vitality_damage", 0.0)) * 0.6 + real_t(calc.get("posture_damage", 0.0));
		if (p_defender_attrs.is_valid() && p_defender_attrs->has_attribute("Posture")) {
			p_defender_attrs->apply_damage("Posture", blocked_posture_dmg);
		}
		result["blocked_posture_damage"] = blocked_posture_dmg;
		return result;
	}

	// 4. Clean Hit
	result["outcome"] = OUTCOME_HIT;
	Ref<StandardDamageExecution> exec;
	exec.instantiate();
	Dictionary params;
	params["damage_channels"] = p_damage_channels;
	Dictionary calc = exec->execute_calculation(p_attacker_attrs, p_defender_attrs, params);

	real_t vit_dmg = calc.get("vitality_damage", 0.0);
	real_t pos_dmg = calc.get("posture_damage", 0.0);
	real_t poi_dmg = calc.get("poise_damage", 0.0);

	if (p_defender_attrs.is_valid()) {
		if (vit_dmg > 0.0 && p_defender_attrs->has_attribute("Health")) {
			p_defender_attrs->apply_damage("Health", vit_dmg);
		}
		if (pos_dmg > 0.0 && p_defender_attrs->has_attribute("Posture")) {
			p_defender_attrs->apply_damage("Posture", pos_dmg);
		}
		if (poi_dmg > 0.0 && p_defender_attrs->has_attribute("Poise")) {
			p_defender_attrs->apply_damage("Poise", poi_dmg);
		}
	}

	result["vitality_damage_dealt"] = vit_dmg;
	result["posture_damage_dealt"] = pos_dmg;
	result["poise_damage_dealt"] = poi_dmg;
	result["channel_breakdown"] = calc.get("channel_breakdown", Dictionary());

	return result;
}

void DamageResolver::_bind_methods() {
	ClassDB::bind_method(D_METHOD("resolve_hit", "hitbox", "hurtbox", "hit_point", "hit_normal"), &DamageResolver::resolve_hit, DEFVAL(Vector3()), DEFVAL(Vector3(0, 1, 0)));
	ClassDB::bind_method(D_METHOD("resolve_interaction", "attacker_attrs", "attacker_tags", "damage_channels", "defender_attrs", "defender_tags", "hit_point", "hit_normal"), &DamageResolver::resolve_interaction, DEFVAL(Vector3()), DEFVAL(Vector3(0, 1, 0)));

	BIND_ENUM_CONSTANT(OUTCOME_HIT);
	BIND_ENUM_CONSTANT(OUTCOME_BLOCK);
	BIND_ENUM_CONSTANT(OUTCOME_PARRY);
	BIND_ENUM_CONSTANT(OUTCOME_EVADED);
	BIND_ENUM_CONSTANT(OUTCOME_IMMUNE);
}

DamageResolver::DamageResolver() {
	singleton = this;
}

DamageResolver::~DamageResolver() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
