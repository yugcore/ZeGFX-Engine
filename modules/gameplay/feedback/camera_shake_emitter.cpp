/**************************************************************************/
/*  camera_shake_emitter.cpp                                              */
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

#include "camera_shake_emitter.h"

void CameraShakeEmitter::play_shake(const Ref<CameraShakeProfile> &p_profile) {
	if (p_profile.is_null()) {
		return;
	}
	ActiveShakeInstance inst;
	inst.profile = p_profile;
	inst.time_elapsed = 0.0;
	active_shakes.push_back(inst);
}

void CameraShakeEmitter::stop_all_shakes() {
	active_shakes.clear();
	current_offset = Vector3();
	set_position(Vector3());
}

void CameraShakeEmitter::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_PROCESS: {
			real_t delta = get_process_delta_time();
			Vector3 accum_offset;

			for (int i = active_shakes.size() - 1; i >= 0; i--) {
				active_shakes.write[i].time_elapsed += delta;
				Ref<CameraShakeProfile> prof = active_shakes[i].profile;
				if (prof.is_valid()) {
					if (active_shakes[i].time_elapsed >= prof->get_duration()) {
						active_shakes.remove_at(i);
					} else {
						accum_offset += prof->evaluate_offset(active_shakes[i].time_elapsed);
					}
				}
			}

			current_offset = accum_offset;
			set_position(accum_offset);
		} break;
	}
}

void CameraShakeEmitter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("play_shake", "profile"), &CameraShakeEmitter::play_shake);
	ClassDB::bind_method(D_METHOD("stop_all_shakes"), &CameraShakeEmitter::stop_all_shakes);
	ClassDB::bind_method(D_METHOD("get_current_shake_offset"), &CameraShakeEmitter::get_current_shake_offset);
}

CameraShakeEmitter::CameraShakeEmitter() {
	set_process(true);
}

CameraShakeEmitter::~CameraShakeEmitter() {}
