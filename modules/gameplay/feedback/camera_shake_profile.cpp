/**************************************************************************/
/*  camera_shake_profile.cpp                                              */
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

#include "camera_shake_profile.h"
#include "core/math/math_funcs.h"

Vector3 CameraShakeProfile::evaluate_offset(real_t p_time_elapsed) const {
	if (p_time_elapsed >= duration || duration <= 0.0) {
		return Vector3();
	}

	real_t progress = CLAMP(p_time_elapsed / duration, 0.0, 1.0);
	real_t decay = 1.0 - progress;
	if (decay_curve.is_valid()) {
		decay = decay_curve->sample(progress);
	}

	real_t phase = p_time_elapsed * frequency * Math::TAU;
	Vector3 noise_dir(Math::sin(phase), Math::cos(phase * 1.3), Math::sin(phase * 0.7));

	return noise_dir * amplitude * decay;
}

void CameraShakeProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_duration", "duration"), &CameraShakeProfile::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &CameraShakeProfile::get_duration);

	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &CameraShakeProfile::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &CameraShakeProfile::get_frequency);

	ClassDB::bind_method(D_METHOD("set_amplitude", "amplitude"), &CameraShakeProfile::set_amplitude);
	ClassDB::bind_method(D_METHOD("get_amplitude"), &CameraShakeProfile::get_amplitude);

	ClassDB::bind_method(D_METHOD("set_decay_curve", "curve"), &CameraShakeProfile::set_decay_curve);
	ClassDB::bind_method(D_METHOD("get_decay_curve"), &CameraShakeProfile::get_decay_curve);

	ClassDB::bind_method(D_METHOD("evaluate_offset", "time_elapsed"), &CameraShakeProfile::evaluate_offset);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency"), "set_frequency", "get_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "amplitude"), "set_amplitude", "get_amplitude");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "decay_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_decay_curve", "get_decay_curve");
}
