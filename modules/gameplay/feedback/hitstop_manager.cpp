/**************************************************************************/
/*  hitstop_manager.cpp                                                   */
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

#include "hitstop_manager.h"
#include "core/config/engine.h"
#include "scene/animation/animation_player.h"

HitstopManager *HitstopManager::singleton = nullptr;

void HitstopManager::trigger_hitstop(Node *p_actor, real_t p_duration_seconds, real_t p_time_dilation) {
	if (!p_actor || p_duration_seconds <= 0.0) {
		return;
	}

	ObjectID id = p_actor->get_instance_id();
	HitstopEntry entry;
	entry.node_id = id;
	entry.duration_remaining = p_duration_seconds;
	entry.time_dilation = p_time_dilation;

	// Check if actor has AnimationPlayer or custom speed_scale
	AnimationPlayer *anim_player = Object::cast_to<AnimationPlayer>(p_actor->find_child("*AnimationPlayer*", true, false));
	if (anim_player) {
		entry.original_speed_scale = anim_player->get_speed_scale();
		anim_player->set_speed_scale(p_time_dilation);
	}

	active_hitstops[id] = entry;
}

void HitstopManager::trigger_global_hitstop(real_t p_duration_seconds, real_t p_time_dilation) {
	if (p_duration_seconds <= 0.0) {
		return;
	}
	if (global_hitstop_remaining <= 0.0) {
		global_time_scale_restore = Engine::get_singleton()->get_time_scale();
	}
	global_hitstop_remaining = p_duration_seconds;
	Engine::get_singleton()->set_time_scale(p_time_dilation);
}

void HitstopManager::process_hitstops(real_t p_delta) {
	// 1. Process actor-specific hitstops
	Vector<ObjectID> to_remove;
	for (KeyValue<ObjectID, HitstopEntry> &E : active_hitstops) {
		E.value.duration_remaining -= p_delta;
		if (E.value.duration_remaining <= 0.0) {
			Node *node = Object::cast_to<Node>(ObjectDB::get_instance(E.key));
			if (node) {
				AnimationPlayer *anim_player = Object::cast_to<AnimationPlayer>(node->find_child("*AnimationPlayer*", true, false));
				if (anim_player) {
					anim_player->set_speed_scale(E.value.original_speed_scale);
				}
			}
			to_remove.push_back(E.key);
		}
	}

	for (int i = 0; i < to_remove.size(); i++) {
		active_hitstops.erase(to_remove[i]);
	}

	// 2. Process global hitstop
	if (global_hitstop_remaining > 0.0) {
		global_hitstop_remaining -= p_delta;
		if (global_hitstop_remaining <= 0.0) {
			Engine::get_singleton()->set_time_scale(global_time_scale_restore);
		}
	}
}

void HitstopManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("trigger_hitstop", "actor", "duration", "time_dilation"), &HitstopManager::trigger_hitstop, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("trigger_global_hitstop", "duration", "time_dilation"), &HitstopManager::trigger_global_hitstop, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("process_hitstops", "delta"), &HitstopManager::process_hitstops);
}

HitstopManager::HitstopManager() {
	singleton = this;
}

HitstopManager::~HitstopManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
