/**************************************************************************/
/*  foliage_type_3d.h                                                     */
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
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"

class FoliageType3D : public Resource {
	GDCLASS(FoliageType3D, Resource);

private:
	String type_name = "Foliage Type";
	Ref<Mesh> mesh;
	Ref<Material> material_override;

	// Painting & Density Settings
	bool enabled = true;
	float density = 1.0f;
	float min_spacing = 0.5f;

	// Scale & Transform Variation
	Vector3 min_scale = Vector3(0.8f, 0.8f, 0.8f);
	Vector3 max_scale = Vector3(1.2f, 1.2f, 1.2f);
	bool uniform_scale = true;
	bool random_rotation_y = true;
	float random_pitch_roll = 5.0f; // in degrees
	float normal_align = 0.5f; // 0.0 = Up, 1.0 = Terrain normal
	float ground_offset = 0.0f;

	// Placement Filters
	float min_altitude = -10000.0f;
	float max_altitude = 10000.0f;
	float max_slope_angle = 45.0f; // in degrees

	// Rendering & Performance
	GeometryInstance3D::ShadowCastingSetting cast_shadow = GeometryInstance3D::SHADOW_CASTING_SETTING_ON;
	float cull_distance = 150.0f;
	float fade_range = 25.0f;

protected:
	static void _bind_methods();

public:
	void set_type_name(const String &p_name);
	String get_type_name() const;

	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_density(float p_density);
	float get_density() const;

	void set_min_spacing(float p_spacing);
	float get_min_spacing() const;

	void set_min_scale(const Vector3 &p_scale);
	Vector3 get_min_scale() const;

	void set_max_scale(const Vector3 &p_scale);
	Vector3 get_max_scale() const;

	void set_uniform_scale(bool p_uniform);
	bool is_uniform_scale() const;

	void set_random_rotation_y(bool p_enabled);
	bool is_random_rotation_y() const;

	void set_random_pitch_roll(float p_degrees);
	float get_random_pitch_roll() const;

	void set_normal_align(float p_align);
	float get_normal_align() const;

	void set_ground_offset(float p_offset);
	float get_ground_offset() const;

	void set_min_altitude(float p_alt);
	float get_min_altitude() const;

	void set_max_altitude(float p_alt);
	float get_max_altitude() const;

	void set_max_slope_angle(float p_degrees);
	float get_max_slope_angle() const;

	void set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_setting);
	GeometryInstance3D::ShadowCastingSetting get_cast_shadow() const;

	void set_cull_distance(float p_dist);
	float get_cull_distance() const;

	void set_fade_range(float p_range);
	float get_fade_range() const;

	FoliageType3D();
	~FoliageType3D() = default;
};
