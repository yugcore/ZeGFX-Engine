/**************************************************************************/
/*  gameplay_effect_execution.cpp                                         */
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

#include "gameplay_effect_execution.h"

// -----------------------------------------------------------------------------
// GameplayEffectExecution
// -----------------------------------------------------------------------------

Dictionary GameplayEffectExecution::execute_calculation(const Ref<AttributeSet> &p_instigator_attrs, const Ref<AttributeSet> &p_target_attrs, const Dictionary &p_params) const {
	Dictionary result;
	if (GDVIRTUAL_CALL(_execute_calculation, p_instigator_attrs, p_target_attrs, p_params, result)) {
		return result;
	}
	return result;
}

void GameplayEffectExecution::_bind_methods() {
	ClassDB::bind_method(D_METHOD("execute_calculation", "instigator_attrs", "target_attrs", "params"), &GameplayEffectExecution::execute_calculation);
	GDVIRTUAL_BIND(_execute_calculation, "instigator_attrs", "target_attrs", "params");
}

// -----------------------------------------------------------------------------
// StandardDamageExecution
// -----------------------------------------------------------------------------

Dictionary StandardDamageExecution::execute_calculation(const Ref<AttributeSet> &p_instigator_attrs, const Ref<AttributeSet> &p_target_attrs, const Dictionary &p_params) const {
	Dictionary result;
	Dictionary damage_channels = p_params.get("damage_channels", Dictionary());

	real_t total_vitality_damage = 0.0;
	real_t total_posture_damage = 0.0;
	real_t total_poise_damage = 0.0;
	Dictionary applied_channel_results;

	for (const Variant *key = damage_channels.next(nullptr); key != nullptr; key = damage_channels.next(key)) {
		StringName channel_tag = *key;
		real_t base_dmg = damage_channels[*key];

		real_t cut_rate = 1.0;
		if (p_target_attrs.is_valid()) {
			cut_rate = p_target_attrs->get_cut_rate(channel_tag);
		}

		real_t final_dmg = base_dmg * cut_rate;
		applied_channel_results[channel_tag] = final_dmg;

		String s = String(channel_tag);
		if (s.begins_with("Damage.Posture")) {
			total_posture_damage += final_dmg;
		} else if (s.begins_with("Damage.Poise")) {
			total_poise_damage += final_dmg;
		} else {
			total_vitality_damage += final_dmg;
		}
	}

	result["vitality_damage"] = total_vitality_damage;
	result["posture_damage"] = total_posture_damage;
	result["poise_damage"] = total_poise_damage;
	result["channel_breakdown"] = applied_channel_results;

	return result;
}

void StandardDamageExecution::_bind_methods() {}
