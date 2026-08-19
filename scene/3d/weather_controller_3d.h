/**************************************************************************/
/*  weather_controller_3d.h                                               */
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

#include "scene/3d/gpu_particles_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/time_of_day_3d.h"
#include "scene/3d/volumetric_clouds_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/resources/material.h"
#include "scene/resources/particle_process_material.h"

class WeatherController3D : public Node3D {
	GDCLASS(WeatherController3D, Node3D);

public:
	enum WeatherType {
		WEATHER_CLEAR,
		WEATHER_PARTLY_CLOUDY,
		WEATHER_OVERCAST,
		WEATHER_RAIN,
		WEATHER_STORM,
		WEATHER_FOGGY,
		WEATHER_SNOW,
		WEATHER_MAX
	};

private:
	WeatherType current_weather = WEATHER_CLEAR;
	WeatherType target_weather = WEATHER_CLEAR;
	float transition_duration = 3.0f; // seconds
	float transition_progress = 1.0f; // 0.0 to 1.0

	// Dynamic Environmental States
	float wetness = 0.0f; // 0.0 = dry, 1.0 = soaked
	float puddle_amount = 0.0f; // 0.0 = none, 1.0 = full puddles
	float rain_intensity = 0.0f;
	float wind_intensity = 4.0f;
	float fog_density_mod = 0.001f;
	float cloud_coverage = 0.1f;
	float sun_dimming = 1.0f;

	// Lightning System
	bool lightning_enabled = true;
	float lightning_timer = 0.0f;
	float lightning_flash_intensity = 0.0f;
	float next_lightning_interval = 7.0f;

	// Linked Nodes
	NodePath clouds_path;
	NodePath time_of_day_path;
	NodePath environment_path;

	// Cached Managed Pointers
	VolumetricClouds3D *managed_clouds = nullptr;
	TimeOfDay3D *managed_time_of_day = nullptr;
	WorldEnvironment *managed_environment = nullptr;

	// Internal Precipitation & Splash Systems
	GPUParticles3D *precipitation_particles = nullptr;
	GPUParticles3D *splash_particles = nullptr;
	Ref<ParticleProcessMaterial> rain_process_material;
	Ref<ParticleProcessMaterial> snow_process_material;
	Ref<ParticleProcessMaterial> splash_process_material;
	Ref<StandardMaterial3D> particle_material;
	Ref<StandardMaterial3D> splash_material;

	struct WeatherPresetParams {
		float cloud_coverage;
		float cloud_density;
		float rain_intensity;
		float wind_speed;
		float fog_density;
		float wetness_target;
		float puddle_target;
		float sun_dimming;
	};

	void _init_precipitation();
	void _find_linked_nodes();
	void _update_lightning(float p_delta);
	WeatherPresetParams _get_preset_params(WeatherType p_type) const;
	void _apply_weather_state();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_weather_type(WeatherType p_type);
	WeatherType get_weather_type() const;

	void set_transition_duration(float p_duration);
	float get_transition_duration() const;

	void change_weather(WeatherType p_type, float p_duration = 3.0f);

	void set_clouds_path(const NodePath &p_path);
	NodePath get_clouds_path() const;

	void set_time_of_day_path(const NodePath &p_path);
	NodePath get_time_of_day_path() const;

	void set_environment_path(const NodePath &p_path);
	NodePath get_environment_path() const;

	void set_lightning_enabled(bool p_enabled);
	bool is_lightning_enabled() const;

	void trigger_lightning_strike();

	float get_wetness() const;
	float get_puddle_amount() const;
	float get_rain_intensity() const;
	float get_wind_intensity() const;
	float get_lightning_flash() const;

	WeatherController3D();
	~WeatherController3D();
};

VARIANT_ENUM_CAST(WeatherController3D::WeatherType);
