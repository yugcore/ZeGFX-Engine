/**************************************************************************/
/*  timeline_track_base.cpp                                               */
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

#include "timeline_track_base.h"

void TimelineTrackBase::on_enter(Node *p_actor) {
	active = true;
	GDVIRTUAL_CALL(_on_enter, p_actor);
}

void TimelineTrackBase::on_tick(Node *p_actor, real_t p_delta, real_t p_phase) {
	GDVIRTUAL_CALL(_on_tick, p_actor, p_delta, p_phase);
}

void TimelineTrackBase::on_exit(Node *p_actor) {
	active = false;
	GDVIRTUAL_CALL(_on_exit, p_actor);
}

void TimelineTrackBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_anchor_mode", "mode"), &TimelineTrackBase::set_anchor_mode);
	ClassDB::bind_method(D_METHOD("get_anchor_mode"), &TimelineTrackBase::get_anchor_mode);

	ClassDB::bind_method(D_METHOD("set_start_phase", "phase"), &TimelineTrackBase::set_start_phase);
	ClassDB::bind_method(D_METHOD("get_start_phase"), &TimelineTrackBase::get_start_phase);

	ClassDB::bind_method(D_METHOD("set_end_phase", "phase"), &TimelineTrackBase::set_end_phase);
	ClassDB::bind_method(D_METHOD("get_end_phase"), &TimelineTrackBase::get_end_phase);

	ClassDB::bind_method(D_METHOD("set_start_time", "time"), &TimelineTrackBase::set_start_time);
	ClassDB::bind_method(D_METHOD("get_start_time"), &TimelineTrackBase::get_start_time);

	ClassDB::bind_method(D_METHOD("set_end_time", "time"), &TimelineTrackBase::set_end_time);
	ClassDB::bind_method(D_METHOD("get_end_time"), &TimelineTrackBase::get_end_time);

	ClassDB::bind_method(D_METHOD("is_active_at_phase", "phase"), &TimelineTrackBase::is_active_at_phase);
	ClassDB::bind_method(D_METHOD("is_active_at_time", "time"), &TimelineTrackBase::is_active_at_time);
	ClassDB::bind_method(D_METHOD("is_currently_active"), &TimelineTrackBase::is_currently_active);

	ClassDB::bind_method(D_METHOD("on_enter", "actor"), &TimelineTrackBase::on_enter);
	ClassDB::bind_method(D_METHOD("on_tick", "actor", "delta", "phase"), &TimelineTrackBase::on_tick);
	ClassDB::bind_method(D_METHOD("on_exit", "actor"), &TimelineTrackBase::on_exit);

	GDVIRTUAL_BIND(_on_enter, "actor");
	GDVIRTUAL_BIND(_on_tick, "actor", "delta", "phase");
	GDVIRTUAL_BIND(_on_exit, "actor");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "anchor_mode", PROPERTY_HINT_ENUM, "Phase Normalized,Time Seconds"), "set_anchor_mode", "get_anchor_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "start_phase", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_start_phase", "get_start_phase");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "end_phase", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_end_phase", "get_end_phase");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "start_time"), "set_start_time", "get_start_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "end_time"), "set_end_time", "get_end_time");

	BIND_ENUM_CONSTANT(ANCHOR_PHASE_NORMALIZED);
	BIND_ENUM_CONSTANT(ANCHOR_TIME_SECONDS);
}
