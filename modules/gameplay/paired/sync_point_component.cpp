/**************************************************************************/
/*  sync_point_component.cpp                                              */
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

#include "sync_point_component.h"
#include "../attributes/attribute_set.h"

void SyncPointComponent::_apply_tags(Node *p_node, const Ref<GameplayTagContainer> &p_tags, bool p_add) {
	if (!p_node || p_tags.is_null()) {
		return;
	}
	Variant v;
	if (p_node->has_meta("state_tags")) {
		v = p_node->get_meta("state_tags");
	} else {
		v = p_node->get("state_tags");
	}
	Ref<GameplayTagContainer> target_container = v;
	if (target_container.is_valid()) {
		PackedStringArray arr = p_tags->get_tags();
		for (int i = 0; i < arr.size(); i++) {
			if (p_add) {
				target_container->add_tag(StringName(arr[i]));
			} else {
				target_container->remove_tag(StringName(arr[i]));
			}
		}
	}
}

bool SyncPointComponent::initiate_interaction(Node3D *p_attacker, Node3D *p_victim, const Ref<PairedInteraction> &p_interaction) {
	if (!p_attacker || !p_victim || p_interaction.is_null()) {
		return false;
	}

	attacker_node = p_attacker;
	victim_node = p_victim;
	active_interaction = p_interaction;
	current_time = 0.0;
	current_state = SYNC_ALIGNING;

	victim_initial_transform = victim_node->is_inside_tree() ? victim_node->get_global_transform() : victim_node->get_transform();

	_apply_tags(attacker_node, active_interaction->get_attacker_granted_tags(), true);
	_apply_tags(victim_node, active_interaction->get_victim_granted_tags(), true);

	emit_signal(SNAME("interaction_started"), attacker_node, victim_node, active_interaction);
	return true;
}

void SyncPointComponent::cancel_interaction() {
	if (current_state == SYNC_IDLE) {
		return;
	}

	if (active_interaction.is_valid()) {
		_apply_tags(attacker_node, active_interaction->get_attacker_granted_tags(), false);
		_apply_tags(victim_node, active_interaction->get_victim_granted_tags(), false);
	}

	emit_signal(SNAME("interaction_interrupted"), attacker_node, victim_node);

	attacker_node = nullptr;
	victim_node = nullptr;
	active_interaction = Ref<PairedInteraction>();
	current_state = SYNC_IDLE;
}

void SyncPointComponent::finish_interaction() {
	if (current_state == SYNC_IDLE) {
		return;
	}

	// Apply damage payload if victim has attribute set
	if (victim_node && active_interaction.is_valid()) {
		Variant v_attrs;
		if (victim_node->has_meta("attribute_set")) {
			v_attrs = victim_node->get_meta("attribute_set");
		} else {
			v_attrs = victim_node->get("attribute_set");
		}
		Ref<AttributeSet> attrs = v_attrs;
		if (attrs.is_valid()) {
			Dictionary payload = active_interaction->get_damage_payload();
			for (const Variant *key = payload.next(nullptr); key != nullptr; key = payload.next(key)) {
				attrs->apply_damage(*key, payload[*key]);
			}
		}
	}

	if (active_interaction.is_valid()) {
		_apply_tags(attacker_node, active_interaction->get_attacker_granted_tags(), false);
		_apply_tags(victim_node, active_interaction->get_victim_granted_tags(), false);
	}

	emit_signal(SNAME("interaction_completed"), attacker_node, victim_node, active_interaction);

	attacker_node = nullptr;
	victim_node = nullptr;
	active_interaction = Ref<PairedInteraction>();
	current_state = SYNC_IDLE;
}

void SyncPointComponent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_PHYSICS_PROCESS: {
			if (current_state == SYNC_IDLE || !attacker_node || !victim_node || active_interaction.is_null()) {
				return;
			}

			real_t delta = get_physics_process_delta_time();
			current_time += delta;

			Transform3D target_global_victim = attacker_node->get_global_transform() * active_interaction->get_relative_victim_transform();

			real_t align_dur = active_interaction->get_alignment_duration();
			if (current_state == SYNC_ALIGNING) {
				real_t t = CLAMP(current_time / align_dur, 0.0, 1.0);
				Transform3D blended;
				blended.origin = victim_initial_transform.origin.lerp(target_global_victim.origin, t);
				blended.basis = victim_initial_transform.basis.slerp(target_global_victim.basis, t);
				victim_node->set_global_transform(blended);

				if (current_time >= align_dur) {
					current_state = SYNC_EXECUTING;
				}
			} else if (current_state == SYNC_EXECUTING) {
				victim_node->set_global_transform(target_global_victim);
				if (current_time >= active_interaction->get_duration()) {
					finish_interaction();
				}
			}
		} break;
	}
}

void SyncPointComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("initiate_interaction", "attacker", "victim", "interaction"), &SyncPointComponent::initiate_interaction);
	ClassDB::bind_method(D_METHOD("cancel_interaction"), &SyncPointComponent::cancel_interaction);
	ClassDB::bind_method(D_METHOD("finish_interaction"), &SyncPointComponent::finish_interaction);

	ClassDB::bind_method(D_METHOD("get_current_state"), &SyncPointComponent::get_current_state);
	ClassDB::bind_method(D_METHOD("get_attacker_node"), &SyncPointComponent::get_attacker_node);
	ClassDB::bind_method(D_METHOD("get_victim_node"), &SyncPointComponent::get_victim_node);
	ClassDB::bind_method(D_METHOD("get_active_interaction"), &SyncPointComponent::get_active_interaction);

	ADD_SIGNAL(MethodInfo("interaction_started", PropertyInfo(Variant::OBJECT, "attacker", PROPERTY_HINT_NODE_TYPE, "Node3D"), PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Node3D"), PropertyInfo(Variant::OBJECT, "interaction", PROPERTY_HINT_RESOURCE_TYPE, "PairedInteraction")));
	ADD_SIGNAL(MethodInfo("interaction_completed", PropertyInfo(Variant::OBJECT, "attacker", PROPERTY_HINT_NODE_TYPE, "Node3D"), PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Node3D"), PropertyInfo(Variant::OBJECT, "interaction", PROPERTY_HINT_RESOURCE_TYPE, "PairedInteraction")));
	ADD_SIGNAL(MethodInfo("interaction_interrupted", PropertyInfo(Variant::OBJECT, "attacker", PROPERTY_HINT_NODE_TYPE, "Node3D"), PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Node3D")));

	BIND_ENUM_CONSTANT(SYNC_IDLE);
	BIND_ENUM_CONSTANT(SYNC_ALIGNING);
	BIND_ENUM_CONSTANT(SYNC_EXECUTING);
}

SyncPointComponent::SyncPointComponent() {
	set_physics_process(true);
}

SyncPointComponent::~SyncPointComponent() {}
