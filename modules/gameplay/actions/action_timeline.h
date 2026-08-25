/**************************************************************************/
/*  action_timeline.h                                                     */
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

#include "timeline_track_base.h"
#include "core/io/resource.h"
#include "core/variant/typed_array.h"

class ActionTimeline : public Resource {
	GDCLASS(ActionTimeline, Resource);

private:
	real_t duration = 1.0;
	real_t reference_fps = 60.0;
	TypedArray<TimelineTrackBase> tracks;

protected:
	static void _bind_methods();

public:
	void set_duration(real_t p_dur) { duration = MAX(0.001, p_dur); emit_changed(); }
	real_t get_duration() const { return duration; }

	void set_reference_fps(real_t p_fps) { reference_fps = MAX(1.0, p_fps); emit_changed(); }
	real_t get_reference_fps() const { return reference_fps; }

	void set_tracks(const TypedArray<TimelineTrackBase> &p_tracks) { tracks = p_tracks; emit_changed(); }
	TypedArray<TimelineTrackBase> get_tracks() const { return tracks; }

	void add_track(const Ref<TimelineTrackBase> &p_track);
	void remove_track(const Ref<TimelineTrackBase> &p_track);
	void clear_tracks();
	int get_track_count() const { return tracks.size(); }
	Ref<TimelineTrackBase> get_track(int p_idx) const;

	void evaluate(Node *p_actor, real_t p_prev_phase, real_t p_curr_phase, real_t p_delta, real_t p_curr_time);
	void reset_tracks(Node *p_actor);

	bool is_cancel_available(real_t p_phase, const StringName &p_action_name) const;

	ActionTimeline() {}
	~ActionTimeline() {}
};
