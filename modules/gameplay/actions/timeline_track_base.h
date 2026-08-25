/**************************************************************************/
/*  timeline_track_base.h                                                 */
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

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "scene/main/node.h"

class TimelineTrackBase : public Resource {
	GDCLASS(TimelineTrackBase, Resource);

public:
	enum TrackAnchorMode {
		ANCHOR_PHASE_NORMALIZED, // 0.0 to 1.0 phase (scales with speed/hitstop)
		ANCHOR_TIME_SECONDS, // Fixed duration in seconds
	};

private:
	TrackAnchorMode anchor_mode = ANCHOR_PHASE_NORMALIZED;
	real_t start_phase = 0.0;
	real_t end_phase = 1.0;
	real_t start_time = 0.0;
	real_t end_time = 0.0;
	bool active = false;

protected:
	static void _bind_methods();

	GDVIRTUAL1(_on_enter, Node *);
	GDVIRTUAL3(_on_tick, Node *, real_t, real_t);
	GDVIRTUAL1(_on_exit, Node *);

public:
	void set_anchor_mode(TrackAnchorMode p_mode) { anchor_mode = p_mode; emit_changed(); }
	TrackAnchorMode get_anchor_mode() const { return anchor_mode; }

	void set_start_phase(real_t p_phase) { start_phase = CLAMP(p_phase, 0.0, 1.0); emit_changed(); }
	real_t get_start_phase() const { return start_phase; }

	void set_end_phase(real_t p_phase) { end_phase = CLAMP(p_phase, 0.0, 1.0); emit_changed(); }
	real_t get_end_phase() const { return end_phase; }

	void set_start_time(real_t p_time) { start_time = MAX(0.0, p_time); emit_changed(); }
	real_t get_start_time() const { return start_time; }

	void set_end_time(real_t p_time) { end_time = MAX(0.0, p_time); emit_changed(); }
	real_t get_end_time() const { return end_time; }

	bool is_active_at_phase(real_t p_phase) const {
		return (p_phase >= start_phase && p_phase <= end_phase);
	}

	bool is_active_at_time(real_t p_time) const {
		return (p_time >= start_time && p_time <= end_time);
	}

	bool is_currently_active() const { return active; }
	void set_currently_active(bool p_active) { active = p_active; }

	virtual void on_enter(Node *p_actor);
	virtual void on_tick(Node *p_actor, real_t p_delta, real_t p_phase);
	virtual void on_exit(Node *p_actor);

	TimelineTrackBase() {}
	~TimelineTrackBase() {}
};

VARIANT_ENUM_CAST(TimelineTrackBase::TrackAnchorMode);
