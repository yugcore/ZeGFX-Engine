/**************************************************************************/
/*  timeline_tracks.cpp                                                   */
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

#include "timeline_tracks.h"
#include "../collision/hitbox_3d.h"
#include "../core/event_bus.h"

// -----------------------------------------------------------------------------
// HitboxTimelineTrack
// -----------------------------------------------------------------------------

void HitboxTimelineTrack::on_enter(Node *p_actor) {
	TimelineTrackBase::on_enter(p_actor);
	if (!p_actor) {
		return;
	}
	Hitbox3D *hitbox = nullptr;
	if (hitbox_node_name != StringName()) {
		hitbox = Object::cast_to<Hitbox3D>(p_actor->find_child(hitbox_node_name, true, false));
	}
	if (!hitbox) {
		hitbox = Object::cast_to<Hitbox3D>(p_actor->find_child("Hitbox3D", true, false));
	}
	if (hitbox) {
		if (!damage_channels.is_empty()) {
			hitbox->set_damage_channels(damage_channels);
		}
		hitbox->activate();
	}
}

void HitboxTimelineTrack::on_exit(Node *p_actor) {
	TimelineTrackBase::on_exit(p_actor);
	if (!p_actor) {
		return;
	}
	Hitbox3D *hitbox = nullptr;
	if (hitbox_node_name != StringName()) {
		hitbox = Object::cast_to<Hitbox3D>(p_actor->find_child(hitbox_node_name, true, false));
	}
	if (!hitbox) {
		hitbox = Object::cast_to<Hitbox3D>(p_actor->find_child("Hitbox3D", true, false));
	}
	if (hitbox) {
		hitbox->deactivate();
	}
}

void HitboxTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_hitbox_node_name", "name"), &HitboxTimelineTrack::set_hitbox_node_name);
	ClassDB::bind_method(D_METHOD("get_hitbox_node_name"), &HitboxTimelineTrack::get_hitbox_node_name);

	ClassDB::bind_method(D_METHOD("set_damage_channels", "channels"), &HitboxTimelineTrack::set_damage_channels);
	ClassDB::bind_method(D_METHOD("get_damage_channels"), &HitboxTimelineTrack::get_damage_channels);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "hitbox_node_name"), "set_hitbox_node_name", "get_hitbox_node_name");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "damage_channels"), "set_damage_channels", "get_damage_channels");
}

// -----------------------------------------------------------------------------
// CancelWindowTimelineTrack
// -----------------------------------------------------------------------------

bool CancelWindowTimelineTrack::is_cancel_allowed(const StringName &p_action_name) const {
	if (allowed_cancel_actions.is_empty()) {
		return true; // Any action can cancel
	}
	for (int i = 0; i < allowed_cancel_actions.size(); i++) {
		if (StringName(allowed_cancel_actions[i]) == p_action_name) {
			return true;
		}
	}
	return false;
}

void CancelWindowTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_allowed_cancel_actions", "actions"), &CancelWindowTimelineTrack::set_allowed_cancel_actions);
	ClassDB::bind_method(D_METHOD("get_allowed_cancel_actions"), &CancelWindowTimelineTrack::get_allowed_cancel_actions);

	ClassDB::bind_method(D_METHOD("set_cancel_requirements", "query"), &CancelWindowTimelineTrack::set_cancel_requirements);
	ClassDB::bind_method(D_METHOD("get_cancel_requirements"), &CancelWindowTimelineTrack::get_cancel_requirements);

	ClassDB::bind_method(D_METHOD("is_cancel_allowed", "action_name"), &CancelWindowTimelineTrack::is_cancel_allowed);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "allowed_cancel_actions"), "set_allowed_cancel_actions", "get_allowed_cancel_actions");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cancel_requirements", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_cancel_requirements", "get_cancel_requirements");
}

// -----------------------------------------------------------------------------
// DefenseWindowTimelineTrack
// -----------------------------------------------------------------------------

static void _set_actor_tag(Node *p_actor, const StringName &p_tag, bool p_add) {
	if (!p_actor) {
		return;
	}
	Variant v_tags = p_actor->get("state_tags");
	Ref<GameplayTagContainer> container = v_tags;
	if (container.is_valid()) {
		if (p_add) {
			container->add_tag(p_tag);
		} else {
			container->remove_tag(p_tag);
		}
	}
}

void DefenseWindowTimelineTrack::on_enter(Node *p_actor) {
	TimelineTrackBase::on_enter(p_actor);
	StringName tag;
	switch (defense_type) {
		case DEFENSE_PARRY:
			tag = "Combat.State.Parrying";
			break;
		case DEFENSE_BLOCK:
			tag = "Combat.State.Guarding";
			break;
		case DEFENSE_IFRAME:
			tag = "Combat.State.Invulnerable";
			break;
	}
	_set_actor_tag(p_actor, tag, true);
}

void DefenseWindowTimelineTrack::on_exit(Node *p_actor) {
	TimelineTrackBase::on_exit(p_actor);
	StringName tag;
	switch (defense_type) {
		case DEFENSE_PARRY:
			tag = "Combat.State.Parrying";
			break;
		case DEFENSE_BLOCK:
			tag = "Combat.State.Guarding";
			break;
		case DEFENSE_IFRAME:
			tag = "Combat.State.Invulnerable";
			break;
	}
	_set_actor_tag(p_actor, tag, false);
}

void DefenseWindowTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_defense_type", "type"), &DefenseWindowTimelineTrack::set_defense_type);
	ClassDB::bind_method(D_METHOD("get_defense_type"), &DefenseWindowTimelineTrack::get_defense_type);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "defense_type", PROPERTY_HINT_ENUM, "Parry,Block,Invulnerable IFrame"), "set_defense_type", "get_defense_type");

	BIND_ENUM_CONSTANT(DEFENSE_PARRY);
	BIND_ENUM_CONSTANT(DEFENSE_BLOCK);
	BIND_ENUM_CONSTANT(DEFENSE_IFRAME);
}

// -----------------------------------------------------------------------------
// TagWindowTimelineTrack
// -----------------------------------------------------------------------------

void TagWindowTimelineTrack::on_enter(Node *p_actor) {
	TimelineTrackBase::on_enter(p_actor);
	for (int i = 0; i < granted_tags.size(); i++) {
		_set_actor_tag(p_actor, StringName(granted_tags[i]), true);
	}
}

void TagWindowTimelineTrack::on_exit(Node *p_actor) {
	TimelineTrackBase::on_exit(p_actor);
	for (int i = 0; i < granted_tags.size(); i++) {
		_set_actor_tag(p_actor, StringName(granted_tags[i]), false);
	}
}

void TagWindowTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_granted_tags", "tags"), &TagWindowTimelineTrack::set_granted_tags);
	ClassDB::bind_method(D_METHOD("get_granted_tags"), &TagWindowTimelineTrack::get_granted_tags);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "granted_tags"), "set_granted_tags", "get_granted_tags");
}

// -----------------------------------------------------------------------------
// NotifyTimelineTrack
// -----------------------------------------------------------------------------

void NotifyTimelineTrack::on_enter(Node *p_actor) {
	TimelineTrackBase::on_enter(p_actor);
	if (signal_name != StringName() && EventBus::get_singleton()) {
		Array args;
		args.push_back(p_actor);
		args.push_back(parameters);
		EventBus::get_singleton()->publish(signal_name, args);
	}
}

void NotifyTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_signal_name", "name"), &NotifyTimelineTrack::set_signal_name);
	ClassDB::bind_method(D_METHOD("get_signal_name"), &NotifyTimelineTrack::get_signal_name);

	ClassDB::bind_method(D_METHOD("set_parameters", "params"), &NotifyTimelineTrack::set_parameters);
	ClassDB::bind_method(D_METHOD("get_parameters"), &NotifyTimelineTrack::get_parameters);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "signal_name"), "set_signal_name", "get_signal_name");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "parameters"), "set_parameters", "get_parameters");
}

// -----------------------------------------------------------------------------
// ScriptCallbackTimelineTrack
// -----------------------------------------------------------------------------

void ScriptCallbackTimelineTrack::on_enter(Node *p_actor) {
	TimelineTrackBase::on_enter(p_actor);
	if (callback.is_valid()) {
		Callable::CallError ce;
		Variant ret;
		const Variant arg = p_actor;
		const Variant *args[1] = { &arg };
		callback.callp(args, 1, ret, ce);
	}
}

void ScriptCallbackTimelineTrack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_callback", "callable"), &ScriptCallbackTimelineTrack::set_callback);
	ClassDB::bind_method(D_METHOD("get_callback"), &ScriptCallbackTimelineTrack::get_callback);

	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "callback"), "set_callback", "get_callback");
}
