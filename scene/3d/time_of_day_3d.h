/**************************************************************************/
/*  time_of_day_3d.h                                                      */
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

#include "scene/3d/light_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/environment.h"

class DirectionalLight3D;

class TimeOfDay3D : public Node3D {
	GDCLASS(TimeOfDay3D, Node3D);

private:
	// Time parameters
	float time_of_day = 12.0f; // 0.0 to 24.0 (Hours)
	float time_scale = 0.0f; // Speed of day progression (0 = paused)
	bool play_in_editor = false;

	// Coordinates & Calendar
	float latitude = 35.0f; // degrees
	int day_of_year = 172; // Summer solstice default

	// Nodes to Drive
	NodePath sun_path;
	NodePath moon_path;
	NodePath environment_path;

	// Auto-discovered / Internal pointers
	DirectionalLight3D *managed_sun = nullptr;
	DirectionalLight3D *managed_moon = nullptr;
	WorldEnvironment *managed_environment = nullptr;

	// Lighting Intensities
	float sun_energy = 1.0f;
	float moon_energy = 0.2f;
	Color sun_zenith_color = Color(1.0f, 0.98f, 0.92f);
	Color sun_horizon_color = Color(1.0f, 0.52f, 0.20f);
	Color moon_color = Color(0.65f, 0.78f, 1.0f);

	// Atmosphere & Stars
	float stars_intensity = 1.0f;
	bool drive_sky = true;

	// State cache
	bool is_night = false;
	int last_hour = 12;

	void _find_or_create_nodes();
	void _update_celestial_positions();
	Color _calculate_sun_color(float p_solar_elevation) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_time_of_day(float p_time);
	float get_time_of_day() const;

	void set_time_scale(float p_scale);
	float get_time_scale() const;

	void set_play_in_editor(bool p_play);
	bool is_playing_in_editor() const;

	void set_latitude(float p_latitude);
	float get_latitude() const;

	void set_day_of_year(int p_day);
	int get_day_of_year() const;

	void set_sun_path(const NodePath &p_path);
	NodePath get_sun_path() const;

	void set_moon_path(const NodePath &p_path);
	NodePath get_moon_path() const;

	void set_environment_path(const NodePath &p_path);
	NodePath get_environment_path() const;

	void set_sun_energy(float p_energy);
	float get_sun_energy() const;

	void set_moon_energy(float p_energy);
	float get_moon_energy() const;

	void set_sun_zenith_color(const Color &p_color);
	Color get_sun_zenith_color() const;

	void set_sun_horizon_color(const Color &p_color);
	Color get_sun_horizon_color() const;

	void set_moon_color(const Color &p_color);
	Color get_moon_color() const;

	void set_stars_intensity(float p_intensity);
	float get_stars_intensity() const;

	void set_drive_sky(bool p_drive);
	bool is_driving_sky() const;

	bool is_night_time() const;
	float get_sun_elevation() const;
	float get_sun_azimuth() const;

	TimeOfDay3D();
	~TimeOfDay3D();
};
