/**************************************************************************/
/*  action_timeline.cpp                                                   */
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

#include "action_timeline.h"
#include "timeline_tracks.h"

void ActionTimeline::add_track(const Ref<TimelineTrackBase> &p_track) {
	if (p_track.is_valid() && !tracks.has(p_track)) {
		tracks.push_back(p_track);
		emit_changed();
	}
}

void ActionTimeline::remove_track(const Ref<TimelineTrackBase> &p_track) {
	if (tracks.has(p_track)) {
		tracks.erase(p_track);
		emit_changed();
	}
}

void ActionTimeline::clear_tracks() {
	tracks.clear();
	emit_changed();
}

Ref<TimelineTrackBase> ActionTimeline::get_track(int p_idx) const {
	if (p_idx >= 0 && p_idx < tracks.size()) {
		return tracks[p_idx];
	}
	return Ref<TimelineTrackBase>();
}

void ActionTimeline::evaluate(Node *p_actor, real_t p_prev_phase, real_t p_curr_phase, real_t p_delta, real_t p_curr_time) {
	for (int i = 0; i < tracks.size(); i++) {
		Ref<TimelineTrackBase> track = tracks[i];
		if (track.is_null()) {
			continue;
		}

		bool should_be_active = false;
		if (track->get_anchor_mode() == TimelineTrackBase::ANCHOR_PHASE_NORMALIZED) {
			should_be_active = track->is_active_at_phase(p_curr_phase);
		} else {
			should_be_active = track->is_active_at_time(p_curr_time);
		}

		bool was_active = track->is_currently_active();

		if (!was_active && should_be_active) {
			track->on_enter(p_actor);
		} else if (was_active && should_be_active) {
			track->on_tick(p_actor, p_delta, p_curr_phase);
		} else if (was_active && !should_be_active) {
			track->on_exit(p_actor);
		}
	}
}

void ActionTimeline::reset_tracks(Node *p_actor) {
	for (int i = 0; i < tracks.size(); i++) {
		Ref<TimelineTrackBase> track = tracks[i];
		if (track.is_valid() && track->is_currently_active()) {
			track->on_exit(p_actor);
		}
	}
}

bool ActionTimeline::is_cancel_available(real_t p_phase, const StringName &p_action_name) const {
	for (int i = 0; i < tracks.size(); i++) {
		Ref<CancelWindowTimelineTrack> cancel_track = tracks[i];
		if (cancel_track.is_valid()) {
			if (cancel_track->is_active_at_phase(p_phase)) {
				if (cancel_track->is_cancel_allowed(p_action_name)) {
					return true;
				}
			}
		}
	}
	return false;
}

void ActionTimeline::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_duration", "duration"), &ActionTimeline::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &ActionTimeline::get_duration);

	ClassDB::bind_method(D_METHOD("set_reference_fps", "fps"), &ActionTimeline::set_reference_fps);
	ClassDB::bind_method(D_METHOD("get_reference_fps"), &ActionTimeline::get_reference_fps);

	ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &ActionTimeline::set_tracks);
	ClassDB::bind_method(D_METHOD("get_tracks"), &ActionTimeline::get_tracks);

	ClassDB::bind_method(D_METHOD("add_track", "track"), &ActionTimeline::add_track);
	ClassDB::bind_method(D_METHOD("remove_track", "track"), &ActionTimeline::remove_track);
	ClassDB::bind_method(D_METHOD("clear_tracks"), &ActionTimeline::clear_tracks);
	ClassDB::bind_method(D_METHOD("get_track_count"), &ActionTimeline::get_track_count);
	ClassDB::bind_method(D_METHOD("get_track", "index"), &ActionTimeline::get_track);

	ClassDB::bind_method(D_METHOD("evaluate", "actor", "prev_phase", "curr_phase", "delta", "curr_time"), &ActionTimeline::evaluate);
	ClassDB::bind_method(D_METHOD("reset_tracks", "actor"), &ActionTimeline::reset_tracks);
	ClassDB::bind_method(D_METHOD("is_cancel_available", "phase", "action_name"), &ActionTimeline::is_cancel_available);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_fps"), "set_reference_fps", "get_reference_fps");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_RESOURCE_TYPE, "TimelineTrackBase"), "set_tracks", "get_tracks");
}
