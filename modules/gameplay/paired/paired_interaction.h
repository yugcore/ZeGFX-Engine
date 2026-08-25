/**************************************************************************/
/*  paired_interaction.h                                                  */
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

#include "../attributes/gameplay_effect.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"
#include "core/math/transform_3d.h"

class PairedInteraction : public Resource {
	GDCLASS(PairedInteraction, Resource);

private:
	StringName interaction_id;
	Transform3D relative_victim_transform; // Offset of victim relative to attacker
	StringName attacker_animation;
	StringName victim_animation;
	real_t duration = 2.0;
	real_t alignment_duration = 0.2;
	Dictionary damage_payload;
	Ref<GameplayEffect> attacker_effect;
	Ref<GameplayEffect> victim_effect;
	Ref<GameplayTagContainer> attacker_granted_tags;
	Ref<GameplayTagContainer> victim_granted_tags;

protected:
	static void _bind_methods();

public:
	void set_interaction_id(const StringName &p_id) { interaction_id = p_id; emit_changed(); }
	StringName get_interaction_id() const { return interaction_id; }

	void set_relative_victim_transform(const Transform3D &p_xform) { relative_victim_transform = p_xform; emit_changed(); }
	Transform3D get_relative_victim_transform() const { return relative_victim_transform; }

	void set_attacker_animation(const StringName &p_anim) { attacker_animation = p_anim; emit_changed(); }
	StringName get_attacker_animation() const { return attacker_animation; }

	void set_victim_animation(const StringName &p_anim) { victim_animation = p_anim; emit_changed(); }
	StringName get_victim_animation() const { return victim_animation; }

	void set_duration(real_t p_dur) { duration = MAX(0.01, p_dur); emit_changed(); }
	real_t get_duration() const { return duration; }

	void set_alignment_duration(real_t p_dur) { alignment_duration = MAX(0.01, p_dur); emit_changed(); }
	real_t get_alignment_duration() const { return alignment_duration; }

	void set_damage_payload(const Dictionary &p_payload) { damage_payload = p_payload; emit_changed(); }
	Dictionary get_damage_payload() const { return damage_payload; }

	void set_attacker_effect(const Ref<GameplayEffect> &p_effect) { attacker_effect = p_effect; emit_changed(); }
	Ref<GameplayEffect> get_attacker_effect() const { return attacker_effect; }

	void set_victim_effect(const Ref<GameplayEffect> &p_effect) { victim_effect = p_effect; emit_changed(); }
	Ref<GameplayEffect> get_victim_effect() const { return victim_effect; }

	void set_attacker_granted_tags(const Ref<GameplayTagContainer> &p_tags) { attacker_granted_tags = p_tags; emit_changed(); }
	Ref<GameplayTagContainer> get_attacker_granted_tags() const { return attacker_granted_tags; }

	void set_victim_granted_tags(const Ref<GameplayTagContainer> &p_tags) { victim_granted_tags = p_tags; emit_changed(); }
	Ref<GameplayTagContainer> get_victim_granted_tags() const { return victim_granted_tags; }

	PairedInteraction() {
		// Default victim position is 1 meter directly in front of attacker
		relative_victim_transform.origin = Vector3(0, 0, -1.0);
	}
	~PairedInteraction() {}
};
