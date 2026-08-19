/**************************************************************************/
/*  weather_controller_3d.cpp                                             */
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

#include "weather_controller_3d.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/particle_process_material.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/rendering/rendering_server.h"

static void _set_or_add_gsp(RenderingServer *rs, const StringName &p_name, RenderingServerEnums::GlobalShaderParameterType p_type, const Variant &p_val) {
	if (rs->global_shader_parameter_get_type(p_name) == RenderingServerEnums::GLOBAL_VAR_TYPE_MAX) {
		rs->global_shader_parameter_add(p_name, p_type, p_val);
	} else {
		rs->global_shader_parameter_set(p_name, p_val);
	}
}

WeatherController3D::WeatherPresetParams WeatherController3D::_get_preset_params(WeatherType p_type) const {
	WeatherPresetParams p;
	switch (p_type) {
		case WEATHER_CLEAR:
			p.cloud_coverage = 0.1f;
			p.cloud_density = 0.5f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 3.0f;
			p.fog_density = 0.001f;
			p.wetness_target = 0.0f;
			p.puddle_target = 0.0f;
			p.sun_dimming = 1.0f;
			break;
		case WEATHER_PARTLY_CLOUDY:
			p.cloud_coverage = 0.45f;
			p.cloud_density = 0.85f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 6.0f;
			p.fog_density = 0.002f;
			p.wetness_target = 0.0f;
			p.puddle_target = 0.0f;
			p.sun_dimming = 0.95f;
			break;
		case WEATHER_OVERCAST:
			p.cloud_coverage = 0.85f;
			p.cloud_density = 1.4f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 10.0f;
			p.fog_density = 0.008f;
			p.wetness_target = 0.2f;
			p.puddle_target = 0.0f;
			p.sun_dimming = 0.55f;
			break;
		case WEATHER_RAIN:
			p.cloud_coverage = 0.95f;
			p.cloud_density = 2.0f;
			p.rain_intensity = 0.85f;
			p.wind_speed = 20.0f;
			p.fog_density = 0.022f;
			p.wetness_target = 0.95f;
			p.puddle_target = 0.8f;
			p.sun_dimming = 0.30f;
			break;
		case WEATHER_STORM:
			p.cloud_coverage = 1.0f;
			p.cloud_density = 3.0f;
			p.rain_intensity = 1.0f;
			p.wind_speed = 36.0f;
			p.fog_density = 0.055f;
			p.wetness_target = 1.0f;
			p.puddle_target = 1.0f;
			p.sun_dimming = 0.15f;
			break;
		case WEATHER_FOGGY:
			p.cloud_coverage = 0.65f;
			p.cloud_density = 1.2f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 1.5f;
			p.fog_density = 0.10f;
			p.wetness_target = 0.45f;
			p.puddle_target = 0.25f;
			p.sun_dimming = 0.40f;
			break;
		case WEATHER_SNOW:
			p.cloud_coverage = 0.9f;
			p.cloud_density = 1.5f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 8.0f;
			p.fog_density = 0.028f;
			p.wetness_target = 0.65f;
			p.puddle_target = 0.2f;
			p.sun_dimming = 0.45f;
			break;
		default:
			p.cloud_coverage = 0.2f;
			p.cloud_density = 0.6f;
			p.rain_intensity = 0.0f;
			p.wind_speed = 5.0f;
			p.fog_density = 0.001f;
			p.wetness_target = 0.0f;
			p.puddle_target = 0.0f;
			p.sun_dimming = 1.0f;
			break;
	}
	return p;
}

void WeatherController3D::_init_precipitation() {
	if (!precipitation_particles) {
		precipitation_particles = Object::cast_to<GPUParticles3D>(get_node_or_null(NodePath("PrecipitationParticles3D")));
		if (!precipitation_particles) {
			precipitation_particles = memnew(GPUParticles3D);
			precipitation_particles->set_name("PrecipitationParticles3D");
			precipitation_particles->set_amount(4500);
			precipitation_particles->set_lifetime(1.0);
			precipitation_particles->set_visibility_aabb(AABB(Vector3(-30, -25, -30), Vector3(60, 50, 60)));

			// Setup Ultra-Thin Rain Streak Mesh
			Ref<BoxMesh> streak_mesh;
			streak_mesh.instantiate();
			streak_mesh->set_size(Vector3(0.008f, 0.65f, 0.008f));

			particle_material.instantiate();
			particle_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
			particle_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
			particle_material->set_albedo(Color(0.82f, 0.90f, 1.0f, 0.55f));
			streak_mesh->set_material(particle_material);

			precipitation_particles->set_draw_pass_mesh(0, streak_mesh);

			// Rain Process Material (High Speed Downward Sheets)
			rain_process_material.instantiate();
			rain_process_material->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_BOX);
			rain_process_material->set_emission_box_extents(Vector3(25.0f, 3.0f, 25.0f));
			rain_process_material->set_direction(Vector3(0.15f, -1.0f, 0.05f));
			rain_process_material->set_spread(4.0f);
			rain_process_material->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 38.0f);
			rain_process_material->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 46.0f);

			// Snow Process Material
			snow_process_material.instantiate();
			snow_process_material->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_BOX);
			snow_process_material->set_emission_box_extents(Vector3(25.0f, 3.0f, 25.0f));
			snow_process_material->set_direction(Vector3(0.1f, -1.0f, 0.1f));
			snow_process_material->set_spread(35.0f);
			snow_process_material->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 2.5f);
			snow_process_material->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 5.0f);

			precipitation_particles->set_process_material(rain_process_material);
			precipitation_particles->set_emitting(false);

			add_child(precipitation_particles, false, INTERNAL_MODE_FRONT);
		}
	}

	if (!splash_particles) {
		splash_particles = Object::cast_to<GPUParticles3D>(get_node_or_null(NodePath("GroundSplashParticles3D")));
		if (!splash_particles) {
			splash_particles = memnew(GPUParticles3D);
			splash_particles->set_name("GroundSplashParticles3D");
			splash_particles->set_amount(2000);
			splash_particles->set_lifetime(0.25);
			splash_particles->set_visibility_aabb(AABB(Vector3(-25, -5, -25), Vector3(50, 10, 50)));

			Ref<SphereMesh> splash_mesh;
			splash_mesh.instantiate();
			splash_mesh->set_radius(0.035f);
			splash_mesh->set_height(0.07f);

			splash_material.instantiate();
			splash_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
			splash_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
			splash_material->set_albedo(Color(0.85f, 0.92f, 1.0f, 0.35f));
			splash_mesh->set_material(splash_material);

			splash_particles->set_draw_pass_mesh(0, splash_mesh);

			splash_process_material.instantiate();
			splash_process_material->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_BOX);
			splash_process_material->set_emission_box_extents(Vector3(20.0f, 0.5f, 20.0f));
			splash_process_material->set_direction(Vector3(0.0f, 1.0f, 0.0f));
			splash_process_material->set_spread(80.0f);
			splash_process_material->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 1.5f);
			splash_process_material->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, 3.5f);

			splash_particles->set_process_material(splash_process_material);
			splash_particles->set_emitting(false);

			add_child(splash_particles, false, INTERNAL_MODE_FRONT);
		}
	}
}

void WeatherController3D::_find_linked_nodes() {
	if (!is_inside_tree()) {
		return;
	}
	// 1. Clouds
	managed_clouds = nullptr;
	if (!clouds_path.is_empty()) {
		managed_clouds = Object::cast_to<VolumetricClouds3D>(get_node_or_null(clouds_path));
	}
	if (!managed_clouds) {
		Node *parent = get_parent();
		if (parent) {
			for (int i = 0; i < parent->get_child_count(); i++) {
				VolumetricClouds3D *vc = Object::cast_to<VolumetricClouds3D>(parent->get_child(i));
				if (vc && (Node *)vc != (Node *)this) {
					managed_clouds = vc;
					break;
				}
			}
		}
	}

	// 2. TimeOfDay
	managed_time_of_day = nullptr;
	if (!time_of_day_path.is_empty()) {
		managed_time_of_day = Object::cast_to<TimeOfDay3D>(get_node_or_null(time_of_day_path));
	}
	if (!managed_time_of_day) {
		Node *parent = get_parent();
		if (parent) {
			for (int i = 0; i < parent->get_child_count(); i++) {
				TimeOfDay3D *tod = Object::cast_to<TimeOfDay3D>(parent->get_child(i));
				if (tod && (Node *)tod != (Node *)this) {
					managed_time_of_day = tod;
					break;
				}
			}
		}
	}

	// 3. WorldEnvironment
	managed_environment = nullptr;
	if (!environment_path.is_empty()) {
		managed_environment = Object::cast_to<WorldEnvironment>(get_node_or_null(environment_path));
	}
	if (!managed_environment) {
		Node *parent = get_parent();
		if (parent) {
			for (int i = 0; i < parent->get_child_count(); i++) {
				WorldEnvironment *we = Object::cast_to<WorldEnvironment>(parent->get_child(i));
				if (we && (Node *)we != (Node *)this) {
					managed_environment = we;
					break;
				}
			}
		}
	}
}

void WeatherController3D::_update_lightning(float p_delta) {
	if (!lightning_enabled || target_weather != WEATHER_STORM) {
		lightning_flash_intensity = 0.0f;
		return;
	}

	lightning_timer += p_delta;
	if (lightning_timer >= next_lightning_interval) {
		trigger_lightning_strike();
		lightning_timer = 0.0f;
		next_lightning_interval = Math::random(4.0f, 10.0f);
	}

	// Decay flash intensity exponentially
	if (lightning_flash_intensity > 0.001f) {
		lightning_flash_intensity = Math::lerp(lightning_flash_intensity, 0.0f, p_delta * 12.0f);
	} else {
		lightning_flash_intensity = 0.0f;
	}
}

void WeatherController3D::trigger_lightning_strike() {
	lightning_flash_intensity = 1.0f;
	emit_signal("lightning_struck");
}

void WeatherController3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_init_precipitation();
			_find_linked_nodes();
			_apply_weather_state();
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			float delta = get_process_delta_time();

			// Keep precipitation and ground splashes centered on active camera
			Viewport *vp = get_viewport();
			if (vp) {
				Camera3D *cam = vp->get_camera_3d();
				if (cam) {
					Vector3 cam_pos = cam->get_global_position();
					if (precipitation_particles) {
						precipitation_particles->set_global_position(cam_pos + Vector3(0, 12.0f, 0));
					}
					if (splash_particles) {
						splash_particles->set_global_position(Vector3(cam_pos.x, 0.0f, cam_pos.z));
					}
				}
			}

			_update_lightning(delta);

			if (transition_progress < 1.0f) {
				float step = (transition_duration > 0.001f) ? (delta / transition_duration) : 1.0f;
				transition_progress = MIN(1.0f, transition_progress + step);
				_apply_weather_state();

				if (transition_progress >= 1.0f) {
					current_weather = target_weather;
					emit_signal("transition_completed");
				}
			} else {
				// Wetness drying / accumulation
				WeatherPresetParams target = _get_preset_params(current_weather);
				if (wetness < target.wetness_target) {
					wetness = MIN(target.wetness_target, wetness + delta * 0.12f);
					_apply_weather_state();
				} else if (wetness > target.wetness_target) {
					wetness = MAX(target.wetness_target, wetness - delta * 0.015f); // slow drying
					_apply_weather_state();
				}
			}
		} break;
	}
}

void WeatherController3D::_apply_weather_state() {
	if (!is_inside_tree()) {
		return;
	}

	_find_linked_nodes();

	WeatherPresetParams from = _get_preset_params(current_weather);
	WeatherPresetParams to = _get_preset_params(target_weather);
	float t = Math::smoothstep(0.0f, 1.0f, transition_progress);

	cloud_coverage = Math::lerp(from.cloud_coverage, to.cloud_coverage, t);
	rain_intensity = Math::lerp(from.rain_intensity, to.rain_intensity, t);
	wind_intensity = Math::lerp(from.wind_speed, to.wind_speed, t);
	fog_density_mod = Math::lerp(from.fog_density, to.fog_density, t);
	sun_dimming = Math::lerp(from.sun_dimming, to.sun_dimming, t);
	puddle_amount = Math::lerp(from.puddle_target, to.puddle_target, t) * wetness;

	// 1. Drive linked VolumetricClouds3D
	if (managed_clouds) {
		managed_clouds->set_coverage(cloud_coverage);
		managed_clouds->set_density(Math::lerp(from.cloud_density, to.cloud_density, t));
		managed_clouds->set_wind_speed(wind_intensity);
		if (target_weather == WEATHER_STORM) {
			managed_clouds->set_shadow_color(Color(0.18f, 0.20f, 0.28f, 1.0f));
			managed_clouds->set_albedo_color(Color(0.70f, 0.75f, 0.85f, 1.0f) + Color(1, 1, 1) * (lightning_flash_intensity * 3.0f));
		} else {
			managed_clouds->set_shadow_color(Color(0.28f, 0.32f, 0.42f, 1.0f));
			managed_clouds->set_albedo_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	// 2. Drive linked WorldEnvironment
	if (managed_environment) {
		Ref<Environment> env = managed_environment->get_environment();
		if (env.is_valid()) {
			env->set_volumetric_fog_enabled(true);
			float storm_fog = fog_density_mod + lightning_flash_intensity * 0.02f;
			env->set_volumetric_fog_density(storm_fog);
		}
	}

	// 3. Drive linked TimeOfDay3D (Sun dimming + lightning flash)
	if (managed_time_of_day) {
		float flash_boost = lightning_flash_intensity * 6.0f;
		managed_time_of_day->set_sun_energy(sun_dimming + flash_boost);
	}

	// 4. Drive Precipitation Particles & Splashes
	if (precipitation_particles) {
		if (target_weather == WEATHER_RAIN || target_weather == WEATHER_STORM) {
			precipitation_particles->set_process_material(rain_process_material);
			precipitation_particles->set_emitting(true);
			precipitation_particles->set_amount(target_weather == WEATHER_STORM ? 5500 : 3000);
			if (splash_particles) {
				splash_particles->set_emitting(true);
				splash_particles->set_amount(target_weather == WEATHER_STORM ? 2500 : 1200);
			}
		} else if (target_weather == WEATHER_SNOW) {
			precipitation_particles->set_process_material(snow_process_material);
			precipitation_particles->set_emitting(true);
			precipitation_particles->set_amount(3000);
			if (splash_particles) {
				splash_particles->set_emitting(false);
			}
		} else {
			precipitation_particles->set_emitting(false);
			if (splash_particles) {
				splash_particles->set_emitting(false);
			}
		}
	}

	// 5. Update Global Shader Uniforms for PBR Shading & Terrain
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		_set_or_add_gsp(rs, "weather_wetness", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, wetness);
		_set_or_add_gsp(rs, "weather_puddle_amount", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, puddle_amount);
		_set_or_add_gsp(rs, "weather_rain_intensity", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, rain_intensity);
		_set_or_add_gsp(rs, "weather_wind_speed", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, wind_intensity);
		_set_or_add_gsp(rs, "weather_fog_density", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, fog_density_mod);
		_set_or_add_gsp(rs, "weather_lightning_flash", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, lightning_flash_intensity);
	}
}

void WeatherController3D::set_weather_type(WeatherType p_type) {
	change_weather(p_type, 0.0f);
}

WeatherController3D::WeatherType WeatherController3D::get_weather_type() const {
	return target_weather;
}

void WeatherController3D::set_transition_duration(float p_duration) {
	transition_duration = MAX(0.0f, p_duration);
}

float WeatherController3D::get_transition_duration() const {
	return transition_duration;
}

void WeatherController3D::change_weather(WeatherType p_type, float p_duration) {
	if (p_type < 0 || p_type >= WEATHER_MAX) {
		return;
	}
	current_weather = (transition_progress >= 1.0f) ? target_weather : current_weather;
	target_weather = p_type;
	transition_duration = MAX(0.001f, p_duration);
	transition_progress = (p_duration <= 0.001f) ? 1.0f : 0.0f;
	_apply_weather_state();
	emit_signal("weather_changed", int(target_weather));
}

void WeatherController3D::set_clouds_path(const NodePath &p_path) {
	clouds_path = p_path;
	_apply_weather_state();
}

NodePath WeatherController3D::get_clouds_path() const {
	return clouds_path;
}

void WeatherController3D::set_time_of_day_path(const NodePath &p_path) {
	time_of_day_path = p_path;
	_apply_weather_state();
}

NodePath WeatherController3D::get_time_of_day_path() const {
	return time_of_day_path;
}

void WeatherController3D::set_environment_path(const NodePath &p_path) {
	environment_path = p_path;
	_apply_weather_state();
}

NodePath WeatherController3D::get_environment_path() const {
	return environment_path;
}

void WeatherController3D::set_lightning_enabled(bool p_enabled) {
	lightning_enabled = p_enabled;
}

bool WeatherController3D::is_lightning_enabled() const {
	return lightning_enabled;
}

float WeatherController3D::get_wetness() const {
	return wetness;
}

float WeatherController3D::get_puddle_amount() const {
	return puddle_amount;
}

float WeatherController3D::get_rain_intensity() const {
	return rain_intensity;
}

float WeatherController3D::get_wind_intensity() const {
	return wind_intensity;
}

float WeatherController3D::get_lightning_flash() const {
	return lightning_flash_intensity;
}

void WeatherController3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_weather_type", "type"), &WeatherController3D::set_weather_type);
	ClassDB::bind_method(D_METHOD("get_weather_type"), &WeatherController3D::get_weather_type);

	ClassDB::bind_method(D_METHOD("set_transition_duration", "duration"), &WeatherController3D::set_transition_duration);
	ClassDB::bind_method(D_METHOD("get_transition_duration"), &WeatherController3D::get_transition_duration);

	ClassDB::bind_method(D_METHOD("change_weather", "type", "duration"), &WeatherController3D::change_weather, DEFVAL(3.0f));

	ClassDB::bind_method(D_METHOD("set_clouds_path", "path"), &WeatherController3D::set_clouds_path);
	ClassDB::bind_method(D_METHOD("get_clouds_path"), &WeatherController3D::get_clouds_path);

	ClassDB::bind_method(D_METHOD("set_time_of_day_path", "path"), &WeatherController3D::set_time_of_day_path);
	ClassDB::bind_method(D_METHOD("get_time_of_day_path"), &WeatherController3D::get_time_of_day_path);

	ClassDB::bind_method(D_METHOD("set_environment_path", "path"), &WeatherController3D::set_environment_path);
	ClassDB::bind_method(D_METHOD("get_environment_path"), &WeatherController3D::get_environment_path);

	ClassDB::bind_method(D_METHOD("set_lightning_enabled", "enabled"), &WeatherController3D::set_lightning_enabled);
	ClassDB::bind_method(D_METHOD("is_lightning_enabled"), &WeatherController3D::is_lightning_enabled);
	ClassDB::bind_method(D_METHOD("trigger_lightning_strike"), &WeatherController3D::trigger_lightning_strike);

	ClassDB::bind_method(D_METHOD("get_wetness"), &WeatherController3D::get_wetness);
	ClassDB::bind_method(D_METHOD("get_puddle_amount"), &WeatherController3D::get_puddle_amount);
	ClassDB::bind_method(D_METHOD("get_rain_intensity"), &WeatherController3D::get_rain_intensity);
	ClassDB::bind_method(D_METHOD("get_wind_intensity"), &WeatherController3D::get_wind_intensity);
	ClassDB::bind_method(D_METHOD("get_lightning_flash"), &WeatherController3D::get_lightning_flash);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "weather_type", PROPERTY_HINT_ENUM, "Clear,Partly Cloudy,Overcast,Rain,Storm,Foggy,Snow"), "set_weather_type", "get_weather_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transition_duration", PROPERTY_HINT_RANGE, "0,60,0.1,suffix:s"), "set_transition_duration", "get_transition_duration");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lightning_enabled"), "set_lightning_enabled", "is_lightning_enabled");

	ADD_GROUP("Linked Nodes (Auto-Discovered if empty)", "");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "clouds_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "VolumetricClouds3D"), "set_clouds_path", "get_clouds_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "time_of_day_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "TimeOfDay3D"), "set_time_of_day_path", "get_time_of_day_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "environment_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "WorldEnvironment"), "set_environment_path", "get_environment_path");

	BIND_ENUM_CONSTANT(WEATHER_CLEAR);
	BIND_ENUM_CONSTANT(WEATHER_PARTLY_CLOUDY);
	BIND_ENUM_CONSTANT(WEATHER_OVERCAST);
	BIND_ENUM_CONSTANT(WEATHER_RAIN);
	BIND_ENUM_CONSTANT(WEATHER_STORM);
	BIND_ENUM_CONSTANT(WEATHER_FOGGY);
	BIND_ENUM_CONSTANT(WEATHER_SNOW);

	ADD_SIGNAL(MethodInfo("weather_changed", PropertyInfo(Variant::INT, "type")));
	ADD_SIGNAL(MethodInfo("transition_completed"));
	ADD_SIGNAL(MethodInfo("lightning_struck"));
}

WeatherController3D::WeatherController3D() {
}

WeatherController3D::~WeatherController3D() {
}
