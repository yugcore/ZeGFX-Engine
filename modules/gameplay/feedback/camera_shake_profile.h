/**************************************************************************/
/*  camera_shake_profile.h                                                */
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
#include "core/math/vector3.h"
#include "core/object/class_db.h"
#include "scene/resources/curve.h"

class CameraShakeProfile : public Resource {
	GDCLASS(CameraShakeProfile, Resource);

private:
	real_t duration = 0.2;
	real_t frequency = 25.0;
	Vector3 amplitude = Vector3(0.1, 0.1, 0.0);
	Ref<Curve> decay_curve;

protected:
	static void _bind_methods();

public:
	void set_duration(real_t p_dur) { duration = MAX(0.01, p_dur); emit_changed(); }
	real_t get_duration() const { return duration; }

	void set_frequency(real_t p_freq) { frequency = MAX(0.1, p_freq); emit_changed(); }
	real_t get_frequency() const { return frequency; }

	void set_amplitude(const Vector3 &p_amp) { amplitude = p_amp; emit_changed(); }
	Vector3 get_amplitude() const { return amplitude; }

	void set_decay_curve(const Ref<Curve> &p_curve) { decay_curve = p_curve; emit_changed(); }
	Ref<Curve> get_decay_curve() const { return decay_curve; }

	Vector3 evaluate_offset(real_t p_time_elapsed) const;

	CameraShakeProfile() {}
	~CameraShakeProfile() {}
};
