/**************************************************************************/
/*  time_of_day_3d.cpp                                                    */
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

#include "time_of_day_3d.h"
#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "scene/3d/light_3d.h"
#include "scene/resources/sky.h"
#include "scene/resources/3d/sky_material.h"
#include "servers/rendering/rendering_server.h"

static void _set_or_add_gsp(RenderingServer *rs, const StringName &p_name, RenderingServerEnums::GlobalShaderParameterType p_type, const Variant &p_val) {
	if (rs->global_shader_parameter_get_type(p_name) == RenderingServerEnums::GLOBAL_VAR_TYPE_MAX) {
		rs->global_shader_parameter_add(p_name, p_type, p_val);
	} else {
		rs->global_shader_parameter_set(p_name, p_val);
	}
}

void TimeOfDay3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_find_or_create_nodes();
			_update_celestial_positions();
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			if (time_scale != 0.0f) {
				if (!Engine::get_singleton()->is_editor_hint() || play_in_editor) {
					float delta = get_process_delta_time();
					float new_time = time_of_day + (time_scale * delta) / 3600.0f;
					while (new_time >= 24.0f) {
						new_time -= 24.0f;
					}
					while (new_time < 0.0f) {
						new_time += 24.0f;
					}
					time_of_day = new_time;
					_update_celestial_positions();
				}
			}
		} break;
	}
}

void TimeOfDay3D::_find_or_create_nodes() {
	if (!is_inside_tree()) {
		return;
	}

	// 1. Find or auto-discover Sun
	managed_sun = nullptr;
	if (!sun_path.is_empty()) {
		managed_sun = Object::cast_to<DirectionalLight3D>(get_node_or_null(sun_path));
	}
	if (!managed_sun) {
		// Search parent and siblings
		Node *parent = get_parent();
		if (parent) {
			for (int i = 0; i < parent->get_child_count(); i++) {
				DirectionalLight3D *dl = Object::cast_to<DirectionalLight3D>(parent->get_child(i));
				if (dl && (Node *)dl != (Node *)this) {
					managed_sun = dl;
					break;
				}
			}
		}
	}
	if (!managed_sun) {
		// Check if internal sun was already created
		managed_sun = Object::cast_to<DirectionalLight3D>(get_node_or_null(NodePath("AutoSun3D")));
		if (!managed_sun) {
			managed_sun = memnew(DirectionalLight3D);
			managed_sun->set_name("AutoSun3D");
			managed_sun->set_shadow(true);
			add_child(managed_sun, false, INTERNAL_MODE_FRONT);
		}
	}

	// 2. Find or auto-discover Moon
	managed_moon = nullptr;
	if (!moon_path.is_empty()) {
		managed_moon = Object::cast_to<DirectionalLight3D>(get_node_or_null(moon_path));
	}

	// 3. Find or auto-discover WorldEnvironment
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

Color TimeOfDay3D::_calculate_sun_color(float p_solar_elevation) const {
	float t = CLAMP(p_solar_elevation / 0.35f, 0.0f, 1.0f);
	return sun_horizon_color.lerp(sun_zenith_color, t);
}

void TimeOfDay3D::_update_celestial_positions() {
	if (!is_inside_tree()) {
		return;
	}

	_find_or_create_nodes();

	// 1. Calculate Solar Coordinates (Ephemeris)
	float lat_rad = Math::deg_to_rad(latitude);
	// Solar declination approx: delta = -23.44 * cos(360/365 * (N + 10))
	float declination = Math::deg_to_rad(-23.44f * Math::cos(Math::deg_to_rad((360.0f / 365.0f) * (day_of_year + 10))));
	// Hour angle: 15 degrees per hour from solar noon (12:00)
	float hour_angle = Math::deg_to_rad((time_of_day - 12.0f) * 15.0f);

	// Solar elevation angle (altitude): sin(alpha) = sin(lat)*sin(dec) + cos(lat)*cos(dec)*cos(H)
	float sin_elevation = Math::sin(lat_rad) * Math::sin(declination) + Math::cos(lat_rad) * Math::cos(declination) * Math::cos(hour_angle);
	sin_elevation = CLAMP(sin_elevation, -1.0f, 1.0f);
	float elevation = Math::asin(sin_elevation);

	// Solar azimuth angle: cos(A) = (sin(dec) - sin(lat)*sin(alpha)) / (cos(lat)*cos(alpha))
	float cos_elevation = Math::cos(elevation);
	float cos_azimuth = 0.0f;
	if (Math::abs(cos_elevation) > 0.0001f) {
		cos_azimuth = (Math::sin(declination) - Math::sin(lat_rad) * sin_elevation) / (Math::cos(lat_rad) * cos_elevation);
		cos_azimuth = CLAMP(cos_azimuth, -1.0f, 1.0f);
	}
	float azimuth = Math::acos(cos_azimuth);
	if (hour_angle > 0.0f) {
		azimuth = float(Math::TAU) - azimuth;
	}

	// Calculate 3D Direction Vector for Sun
	Vector3 sun_dir = Vector3(
			Math::cos(elevation) * Math::sin(azimuth),
			Math::sin(elevation),
			Math::cos(elevation) * Math::cos(azimuth))
							  .normalized();

	// Moon is directly opposite in orbit with 5-degree inclination offset
	Vector3 moon_dir = -sun_dir;

	bool now_night = (elevation < -0.02f);
	if (now_night != is_night) {
		is_night = now_night;
		if (is_night) {
			emit_signal("night_started");
		} else {
			emit_signal("day_started");
		}
	}

	int current_hour = int(time_of_day);
	if (current_hour != last_hour) {
		last_hour = current_hour;
		emit_signal("hour_passed", last_hour);
	}
	emit_signal("time_changed", time_of_day);

	// 2. Drive Sun DirectionalLight3D
	if (managed_sun) {
		if (elevation > -0.1f) {
			managed_sun->show();
			Transform3D xform;
			xform = xform.looking_at(-sun_dir, Vector3(0, 1, 0));
			managed_sun->set_global_transform(Transform3D(xform.basis, managed_sun->get_global_position()));

			float sun_fade = CLAMP((elevation + 0.1f) / 0.15f, 0.0f, 1.0f);
			managed_sun->set_param(Light3D::PARAM_ENERGY, sun_energy * sun_fade);
			managed_sun->set_color(_calculate_sun_color(elevation));
		} else {
			managed_sun->hide();
		}
	}

	// 3. Drive Moon DirectionalLight3D
	if (managed_moon) {
		if (is_night) {
			managed_moon->show();
			Transform3D xform;
			xform = xform.looking_at(-moon_dir, Vector3(0, 1, 0));
			managed_moon->set_global_transform(Transform3D(xform.basis, managed_moon->get_global_position()));

			float moon_fade = CLAMP((-elevation) / 0.15f, 0.0f, 1.0f);
			managed_moon->set_param(Light3D::PARAM_ENERGY, moon_energy * moon_fade);
			managed_moon->set_color(moon_color);
		} else {
			managed_moon->hide();
		}
	}

	// 4. Drive Sky and Ambient in WorldEnvironment
	if (drive_sky && managed_environment) {
		Ref<Environment> env = managed_environment->get_environment();
		if (env.is_valid()) {
			float elev_ratio = CLAMP((elevation + 0.05f) / 0.40f, 0.0f, 1.0f);
			if (elevation > 0.0f) {
				// Daytime / Sunrise / Sunset
				Color sky_top = Color(0.15f, 0.25f, 0.60f).lerp(Color(0.25f, 0.45f, 0.88f), elev_ratio);
				Color sky_horiz = Color(0.95f, 0.45f, 0.18f).lerp(Color(0.68f, 0.78f, 0.92f), elev_ratio);
				Color ground_horiz = Color(0.65f, 0.35f, 0.15f).lerp(Color(0.40f, 0.45f, 0.50f), elev_ratio);
				Color ground_bot = Color(0.12f, 0.12f, 0.15f);

				Ref<Sky> sky = env->get_sky();
				if (sky.is_valid()) {
					Ref<ProceduralSkyMaterial> proc_mat = sky->get_material();
					if (proc_mat.is_valid()) {
						proc_mat->set_sky_top_color(sky_top);
						proc_mat->set_sky_horizon_color(sky_horiz);
						proc_mat->set_ground_horizon_color(ground_horiz);
						proc_mat->set_ground_bottom_color(ground_bot);
						proc_mat->set_sky_energy_multiplier(Math::lerp(0.3f, 1.0f, elev_ratio));
					}
					Ref<PhysicalSkyMaterial> phys_mat = sky->get_material();
					if (phys_mat.is_valid()) {
						phys_mat->set_energy_multiplier(Math::lerp(0.2f, 1.0f, elev_ratio));
					}
				}
				env->set_ambient_source(Environment::AMBIENT_SOURCE_SKY);
				env->set_ambient_light_energy(Math::lerp(0.2f, 1.0f, elev_ratio));
			} else {
				// Nighttime
				Color sky_top = Color(0.015f, 0.02f, 0.06f);
				Color sky_horiz = Color(0.04f, 0.05f, 0.12f);
				Color ground_horiz = Color(0.03f, 0.03f, 0.05f);
				Color ground_bot = Color(0.01f, 0.01f, 0.02f);

				Ref<Sky> sky = env->get_sky();
				if (sky.is_valid()) {
					Ref<ProceduralSkyMaterial> proc_mat = sky->get_material();
					if (proc_mat.is_valid()) {
						proc_mat->set_sky_top_color(sky_top);
						proc_mat->set_sky_horizon_color(sky_horiz);
						proc_mat->set_ground_horizon_color(ground_horiz);
						proc_mat->set_ground_bottom_color(ground_bot);
						proc_mat->set_sky_energy_multiplier(0.15f);
					}
					Ref<PhysicalSkyMaterial> phys_mat = sky->get_material();
					if (phys_mat.is_valid()) {
						phys_mat->set_energy_multiplier(0.12f);
					}
				}
				env->set_ambient_source(Environment::AMBIENT_SOURCE_COLOR);
				env->set_ambient_light_color(Color(0.08f, 0.12f, 0.25f));
				env->set_ambient_light_energy(0.25f);
			}
		}
	}

	// 5. Update Global Shader Parameters for Sky & Atmosphere
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		_set_or_add_gsp(rs, "celestial_sun_direction", RenderingServerEnums::GLOBAL_VAR_TYPE_VEC3, sun_dir);
		_set_or_add_gsp(rs, "celestial_moon_direction", RenderingServerEnums::GLOBAL_VAR_TYPE_VEC3, moon_dir);
		_set_or_add_gsp(rs, "celestial_time_of_day", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, time_of_day);
		_set_or_add_gsp(rs, "celestial_sun_elevation", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, elevation);
		_set_or_add_gsp(rs, "celestial_is_night", RenderingServerEnums::GLOBAL_VAR_TYPE_BOOL, is_night);
		_set_or_add_gsp(rs, "celestial_stars_intensity", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, is_night ? stars_intensity : 0.0f);
	}
}

void TimeOfDay3D::set_time_of_day(float p_time) {
	time_of_day = CLAMP(p_time, 0.0f, 24.0f);
	_update_celestial_positions();
}

float TimeOfDay3D::get_time_of_day() const {
	return time_of_day;
}

void TimeOfDay3D::set_time_scale(float p_scale) {
	time_scale = p_scale;
}

float TimeOfDay3D::get_time_scale() const {
	return time_scale;
}

void TimeOfDay3D::set_play_in_editor(bool p_play) {
	play_in_editor = p_play;
}

bool TimeOfDay3D::is_playing_in_editor() const {
	return play_in_editor;
}

void TimeOfDay3D::set_latitude(float p_latitude) {
	latitude = CLAMP(p_latitude, -90.0f, 90.0f);
	_update_celestial_positions();
}

float TimeOfDay3D::get_latitude() const {
	return latitude;
}

void TimeOfDay3D::set_day_of_year(int p_day) {
	day_of_year = CLAMP(p_day, 1, 365);
	_update_celestial_positions();
}

int TimeOfDay3D::get_day_of_year() const {
	return day_of_year;
}

void TimeOfDay3D::set_sun_path(const NodePath &p_path) {
	sun_path = p_path;
	_find_or_create_nodes();
	_update_celestial_positions();
}

NodePath TimeOfDay3D::get_sun_path() const {
	return sun_path;
}

void TimeOfDay3D::set_moon_path(const NodePath &p_path) {
	moon_path = p_path;
	_find_or_create_nodes();
	_update_celestial_positions();
}

NodePath TimeOfDay3D::get_moon_path() const {
	return moon_path;
}

void TimeOfDay3D::set_environment_path(const NodePath &p_path) {
	environment_path = p_path;
	_find_or_create_nodes();
	_update_celestial_positions();
}

NodePath TimeOfDay3D::get_environment_path() const {
	return environment_path;
}

void TimeOfDay3D::set_sun_energy(float p_energy) {
	sun_energy = MAX(0.0f, p_energy);
	_update_celestial_positions();
}

float TimeOfDay3D::get_sun_energy() const {
	return sun_energy;
}

void TimeOfDay3D::set_moon_energy(float p_energy) {
	moon_energy = MAX(0.0f, p_energy);
	_update_celestial_positions();
}

float TimeOfDay3D::get_moon_energy() const {
	return moon_energy;
}

void TimeOfDay3D::set_sun_zenith_color(const Color &p_color) {
	sun_zenith_color = p_color;
	_update_celestial_positions();
}

Color TimeOfDay3D::get_sun_zenith_color() const {
	return sun_zenith_color;
}

void TimeOfDay3D::set_sun_horizon_color(const Color &p_color) {
	sun_horizon_color = p_color;
	_update_celestial_positions();
}

Color TimeOfDay3D::get_sun_horizon_color() const {
	return sun_horizon_color;
}

void TimeOfDay3D::set_moon_color(const Color &p_color) {
	moon_color = p_color;
	_update_celestial_positions();
}

Color TimeOfDay3D::get_moon_color() const {
	return moon_color;
}

void TimeOfDay3D::set_stars_intensity(float p_intensity) {
	stars_intensity = MAX(0.0f, p_intensity);
	_update_celestial_positions();
}

float TimeOfDay3D::get_stars_intensity() const {
	return stars_intensity;
}

void TimeOfDay3D::set_drive_sky(bool p_drive) {
	drive_sky = p_drive;
	_update_celestial_positions();
}

bool TimeOfDay3D::is_driving_sky() const {
	return drive_sky;
}

bool TimeOfDay3D::is_night_time() const {
	return is_night;
}

float TimeOfDay3D::get_sun_elevation() const {
	float lat_rad = Math::deg_to_rad(latitude);
	float declination = Math::deg_to_rad(-23.44f * Math::cos(Math::deg_to_rad((360.0f / 365.0f) * (day_of_year + 10))));
	float hour_angle = Math::deg_to_rad((time_of_day - 12.0f) * 15.0f);
	float sin_elevation = Math::sin(lat_rad) * Math::sin(declination) + Math::cos(lat_rad) * Math::cos(declination) * Math::cos(hour_angle);
	return Math::asin(CLAMP(sin_elevation, -1.0f, 1.0f));
}

float TimeOfDay3D::get_sun_azimuth() const {
	float lat_rad = Math::deg_to_rad(latitude);
	float declination = Math::deg_to_rad(-23.44f * Math::cos(Math::deg_to_rad((360.0f / 365.0f) * (day_of_year + 10))));
	float hour_angle = Math::deg_to_rad((time_of_day - 12.0f) * 15.0f);
	float sin_elevation = Math::sin(lat_rad) * Math::sin(declination) + Math::cos(lat_rad) * Math::cos(declination) * Math::cos(hour_angle);
	float elevation = Math::asin(CLAMP(sin_elevation, -1.0f, 1.0f));
	float cos_elevation = Math::cos(elevation);
	float cos_azimuth = 0.0f;
	if (Math::abs(cos_elevation) > 0.0001f) {
		cos_azimuth = (Math::sin(declination) - Math::sin(lat_rad) * sin_elevation) / (Math::cos(lat_rad) * cos_elevation);
	}
	float azimuth = Math::acos(CLAMP(cos_azimuth, -1.0f, 1.0f));
	if (hour_angle > 0.0f) {
		azimuth = float(Math::TAU) - azimuth;
	}
	return azimuth;
}

void TimeOfDay3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_time_of_day", "time"), &TimeOfDay3D::set_time_of_day);
	ClassDB::bind_method(D_METHOD("get_time_of_day"), &TimeOfDay3D::get_time_of_day);

	ClassDB::bind_method(D_METHOD("set_time_scale", "scale"), &TimeOfDay3D::set_time_scale);
	ClassDB::bind_method(D_METHOD("get_time_scale"), &TimeOfDay3D::get_time_scale);

	ClassDB::bind_method(D_METHOD("set_play_in_editor", "play"), &TimeOfDay3D::set_play_in_editor);
	ClassDB::bind_method(D_METHOD("is_playing_in_editor"), &TimeOfDay3D::is_playing_in_editor);

	ClassDB::bind_method(D_METHOD("set_latitude", "latitude"), &TimeOfDay3D::set_latitude);
	ClassDB::bind_method(D_METHOD("get_latitude"), &TimeOfDay3D::get_latitude);

	ClassDB::bind_method(D_METHOD("set_day_of_year", "day"), &TimeOfDay3D::set_day_of_year);
	ClassDB::bind_method(D_METHOD("get_day_of_year"), &TimeOfDay3D::get_day_of_year);

	ClassDB::bind_method(D_METHOD("set_sun_path", "path"), &TimeOfDay3D::set_sun_path);
	ClassDB::bind_method(D_METHOD("get_sun_path"), &TimeOfDay3D::get_sun_path);

	ClassDB::bind_method(D_METHOD("set_moon_path", "path"), &TimeOfDay3D::set_moon_path);
	ClassDB::bind_method(D_METHOD("get_moon_path"), &TimeOfDay3D::get_moon_path);

	ClassDB::bind_method(D_METHOD("set_environment_path", "path"), &TimeOfDay3D::set_environment_path);
	ClassDB::bind_method(D_METHOD("get_environment_path"), &TimeOfDay3D::get_environment_path);

	ClassDB::bind_method(D_METHOD("set_sun_energy", "energy"), &TimeOfDay3D::set_sun_energy);
	ClassDB::bind_method(D_METHOD("get_sun_energy"), &TimeOfDay3D::get_sun_energy);

	ClassDB::bind_method(D_METHOD("set_moon_energy", "energy"), &TimeOfDay3D::set_moon_energy);
	ClassDB::bind_method(D_METHOD("get_moon_energy"), &TimeOfDay3D::get_moon_energy);

	ClassDB::bind_method(D_METHOD("set_sun_zenith_color", "color"), &TimeOfDay3D::set_sun_zenith_color);
	ClassDB::bind_method(D_METHOD("get_sun_zenith_color"), &TimeOfDay3D::get_sun_zenith_color);

	ClassDB::bind_method(D_METHOD("set_sun_horizon_color", "color"), &TimeOfDay3D::set_sun_horizon_color);
	ClassDB::bind_method(D_METHOD("get_sun_horizon_color"), &TimeOfDay3D::get_sun_horizon_color);

	ClassDB::bind_method(D_METHOD("set_moon_color", "color"), &TimeOfDay3D::set_moon_color);
	ClassDB::bind_method(D_METHOD("get_moon_color"), &TimeOfDay3D::get_moon_color);

	ClassDB::bind_method(D_METHOD("set_stars_intensity", "intensity"), &TimeOfDay3D::set_stars_intensity);
	ClassDB::bind_method(D_METHOD("get_stars_intensity"), &TimeOfDay3D::get_stars_intensity);

	ClassDB::bind_method(D_METHOD("set_drive_sky", "drive"), &TimeOfDay3D::set_drive_sky);
	ClassDB::bind_method(D_METHOD("is_driving_sky"), &TimeOfDay3D::is_driving_sky);

	ClassDB::bind_method(D_METHOD("is_night_time"), &TimeOfDay3D::is_night_time);
	ClassDB::bind_method(D_METHOD("get_sun_elevation"), &TimeOfDay3D::get_sun_elevation);
	ClassDB::bind_method(D_METHOD("get_sun_azimuth"), &TimeOfDay3D::get_sun_azimuth);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_of_day", PROPERTY_HINT_RANGE, "0,24,0.01,suffix:h"), "set_time_of_day", "get_time_of_day");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_scale", PROPERTY_HINT_RANGE, "0,3600,0.1"), "set_time_scale", "get_time_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "play_in_editor"), "set_play_in_editor", "is_playing_in_editor");

	ADD_GROUP("Calendar & Coordinates", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_RANGE, "-90,90,0.1,suffix:deg"), "set_latitude", "get_latitude");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "day_of_year", PROPERTY_HINT_RANGE, "1,365,1"), "set_day_of_year", "get_day_of_year");

	ADD_GROUP("Linked Nodes (Auto-Discovered if empty)", "");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "sun_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "DirectionalLight3D"), "set_sun_path", "get_sun_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "moon_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "DirectionalLight3D"), "set_moon_path", "get_moon_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "environment_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "WorldEnvironment"), "set_environment_path", "get_environment_path");

	ADD_GROUP("Sun Lighting", "sun_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sun_energy", PROPERTY_HINT_RANGE, "0,16,0.01"), "set_sun_energy", "get_sun_energy");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "sun_zenith_color"), "set_sun_zenith_color", "get_sun_zenith_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "sun_horizon_color"), "set_sun_horizon_color", "get_sun_horizon_color");

	ADD_GROUP("Moon Lighting", "moon_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "moon_energy", PROPERTY_HINT_RANGE, "0,5,0.01"), "set_moon_energy", "get_moon_energy");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "moon_color"), "set_moon_color", "get_moon_color");

	ADD_GROUP("Sky & Atmosphere", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drive_sky"), "set_drive_sky", "is_driving_sky");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stars_intensity", PROPERTY_HINT_RANGE, "0,5,0.01"), "set_stars_intensity", "get_stars_intensity");

	ADD_SIGNAL(MethodInfo("time_changed", PropertyInfo(Variant::FLOAT, "time")));
	ADD_SIGNAL(MethodInfo("hour_passed", PropertyInfo(Variant::INT, "hour")));
	ADD_SIGNAL(MethodInfo("day_started"));
	ADD_SIGNAL(MethodInfo("night_started"));
}

TimeOfDay3D::TimeOfDay3D() {
}

TimeOfDay3D::~TimeOfDay3D() {
}
