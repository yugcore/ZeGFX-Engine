/**************************************************************************/
/*  combat_telemetry.cpp                                                  */
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

#include "combat_telemetry.h"

CombatTelemetry *CombatTelemetry::singleton = nullptr;

CombatTelemetry::CombatTelemetry() {
	if (!singleton) {
		singleton = this;
	}
}

CombatTelemetry::~CombatTelemetry() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void CombatTelemetry::reset_session() {
	total_damage_dealt = 0.0;
	total_damage_taken = 0.0;
	session_time = 0.0;
	total_hits = 0;
	total_parries = 0;
	total_blocks = 0;
}

void CombatTelemetry::record_hit(double p_damage) {
	total_damage_dealt += p_damage;
	total_hits++;
}

void CombatTelemetry::record_damage_taken(double p_damage) {
	total_damage_taken += p_damage;
}

void CombatTelemetry::record_parry() {
	total_parries++;
}

void CombatTelemetry::record_block(double p_damage) {
	total_blocks++;
}

void CombatTelemetry::tick_session(double p_delta) {
	session_time += p_delta;
}

Dictionary CombatTelemetry::get_summary() const {
	Dictionary d;
	d["total_damage_dealt"] = total_damage_dealt;
	d["total_damage_taken"] = total_damage_taken;
	d["dps"] = (session_time > 0.0) ? (total_damage_dealt / session_time) : 0.0;
	d["total_hits"] = total_hits;
	d["total_parries"] = total_parries;
	d["total_blocks"] = total_blocks;

	int total_defensive = total_parries + total_blocks;
	d["parry_success_rate"] = (total_defensive > 0) ? ((double)total_parries / (double)total_defensive) : 0.0;

	return d;
}

void CombatTelemetry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("reset_session"), &CombatTelemetry::reset_session);
	ClassDB::bind_method(D_METHOD("record_hit", "damage"), &CombatTelemetry::record_hit);
	ClassDB::bind_method(D_METHOD("record_damage_taken", "damage"), &CombatTelemetry::record_damage_taken);
	ClassDB::bind_method(D_METHOD("record_parry"), &CombatTelemetry::record_parry);
	ClassDB::bind_method(D_METHOD("record_block", "damage"), &CombatTelemetry::record_block, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("tick_session", "delta"), &CombatTelemetry::tick_session);
	ClassDB::bind_method(D_METHOD("get_summary"), &CombatTelemetry::get_summary);
}
