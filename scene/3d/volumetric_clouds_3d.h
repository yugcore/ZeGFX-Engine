/**************************************************************************/
/*  volumetric_clouds_3d.h                                                */
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

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/material.h"

class VolumetricClouds3D : public Node3D {
	GDCLASS(VolumetricClouds3D, Node3D);

public:
	enum Quality {
		QUALITY_LOW,
		QUALITY_MEDIUM,
		QUALITY_HIGH,
		QUALITY_ULTRA,
		QUALITY_MAX
	};

private:
	bool enabled = true;
	Quality quality = QUALITY_HIGH;

	// Geometry & Altitude
	float base_altitude = 1500.0f; // meters
	float cloud_thickness = 2500.0f; // meters

	// Shape & Density
	float coverage = 0.5f; // 0.0 to 1.0
	float density = 1.0f; // 0.0 to 5.0
	float detail_scale = 1.0f;
	float detail_erosion = 0.35f;

	// Wind & Animation
	Vector3 wind_direction = Vector3(1, 0, 0.2).normalized();
	float wind_speed = 12.0f; // m/s
	Vector3 wind_offset;

	// Optical Properties & Lighting
	Color albedo_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	Color shadow_color = Color(0.32f, 0.38f, 0.48f, 1.0f);
	float silver_lining_intensity = 1.5f;
	float silver_lining_spread = 0.7f;
	float powder_effect_strength = 0.5f;
	float shadow_density = 0.8f;

	// Ground Shadows
	bool cast_shadows_on_ground = true;
	float shadow_ground_intensity = 0.5f;

	// Internal Visual Skydome
	MeshInstance3D *cloud_dome = nullptr;
	Ref<ShaderMaterial> cloud_material;
	Ref<Shader> cloud_shader;

	void _init_cloud_dome();
	void _update_clouds();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_quality(Quality p_quality);
	Quality get_quality() const;

	void set_base_altitude(float p_altitude);
	float get_base_altitude() const;

	void set_cloud_thickness(float p_thickness);
	float get_cloud_thickness() const;

	void set_coverage(float p_coverage);
	float get_coverage() const;

	void set_density(float p_density);
	float get_density() const;

	void set_detail_scale(float p_scale);
	float get_detail_scale() const;

	void set_detail_erosion(float p_erosion);
	float get_detail_erosion() const;

	void set_wind_direction(const Vector3 &p_dir);
	Vector3 get_wind_direction() const;

	void set_wind_speed(float p_speed);
	float get_wind_speed() const;

	void set_albedo_color(const Color &p_color);
	Color get_albedo_color() const;

	void set_shadow_color(const Color &p_color);
	Color get_shadow_color() const;

	void set_silver_lining_intensity(float p_intensity);
	float get_silver_lining_intensity() const;

	void set_silver_lining_spread(float p_spread);
	float get_silver_lining_spread() const;

	void set_powder_effect_strength(float p_strength);
	float get_powder_effect_strength() const;

	void set_shadow_density(float p_density);
	float get_shadow_density() const;

	void set_cast_shadows_on_ground(bool p_cast);
	bool is_casting_shadows_on_ground() const;

	void set_shadow_ground_intensity(float p_intensity);
	float get_shadow_ground_intensity() const;

	VolumetricClouds3D();
	~VolumetricClouds3D();
};

VARIANT_ENUM_CAST(VolumetricClouds3D::Quality);
