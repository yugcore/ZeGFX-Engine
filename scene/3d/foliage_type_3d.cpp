/**************************************************************************/
/*  foliage_type_3d.cpp                                                   */
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

#include "foliage_type_3d.h"

#include "core/object/class_db.h"

void FoliageType3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_type_name", "type_name"), &FoliageType3D::set_type_name);
	ClassDB::bind_method(D_METHOD("get_type_name"), &FoliageType3D::get_type_name);

	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &FoliageType3D::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &FoliageType3D::get_mesh);

	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &FoliageType3D::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &FoliageType3D::get_material_override);

	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &FoliageType3D::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &FoliageType3D::is_enabled);

	ClassDB::bind_method(D_METHOD("set_density", "density"), &FoliageType3D::set_density);
	ClassDB::bind_method(D_METHOD("get_density"), &FoliageType3D::get_density);

	ClassDB::bind_method(D_METHOD("set_min_spacing", "spacing"), &FoliageType3D::set_min_spacing);
	ClassDB::bind_method(D_METHOD("get_min_spacing"), &FoliageType3D::get_min_spacing);

	ClassDB::bind_method(D_METHOD("set_min_scale", "scale"), &FoliageType3D::set_min_scale);
	ClassDB::bind_method(D_METHOD("get_min_scale"), &FoliageType3D::get_min_scale);

	ClassDB::bind_method(D_METHOD("set_max_scale", "scale"), &FoliageType3D::set_max_scale);
	ClassDB::bind_method(D_METHOD("get_max_scale"), &FoliageType3D::get_max_scale);

	ClassDB::bind_method(D_METHOD("set_uniform_scale", "uniform"), &FoliageType3D::set_uniform_scale);
	ClassDB::bind_method(D_METHOD("is_uniform_scale"), &FoliageType3D::is_uniform_scale);

	ClassDB::bind_method(D_METHOD("set_random_rotation_y", "enabled"), &FoliageType3D::set_random_rotation_y);
	ClassDB::bind_method(D_METHOD("is_random_rotation_y"), &FoliageType3D::is_random_rotation_y);

	ClassDB::bind_method(D_METHOD("set_random_pitch_roll", "degrees"), &FoliageType3D::set_random_pitch_roll);
	ClassDB::bind_method(D_METHOD("get_random_pitch_roll"), &FoliageType3D::get_random_pitch_roll);

	ClassDB::bind_method(D_METHOD("set_normal_align", "align"), &FoliageType3D::set_normal_align);
	ClassDB::bind_method(D_METHOD("get_normal_align"), &FoliageType3D::get_normal_align);

	ClassDB::bind_method(D_METHOD("set_ground_offset", "offset"), &FoliageType3D::set_ground_offset);
	ClassDB::bind_method(D_METHOD("get_ground_offset"), &FoliageType3D::get_ground_offset);

	ClassDB::bind_method(D_METHOD("set_min_altitude", "alt"), &FoliageType3D::set_min_altitude);
	ClassDB::bind_method(D_METHOD("get_min_altitude"), &FoliageType3D::get_min_altitude);

	ClassDB::bind_method(D_METHOD("set_max_altitude", "alt"), &FoliageType3D::set_max_altitude);
	ClassDB::bind_method(D_METHOD("get_max_altitude"), &FoliageType3D::get_max_altitude);

	ClassDB::bind_method(D_METHOD("set_max_slope_angle", "degrees"), &FoliageType3D::set_max_slope_angle);
	ClassDB::bind_method(D_METHOD("get_max_slope_angle"), &FoliageType3D::get_max_slope_angle);

	ClassDB::bind_method(D_METHOD("set_cast_shadow", "setting"), &FoliageType3D::set_cast_shadow);
	ClassDB::bind_method(D_METHOD("get_cast_shadow"), &FoliageType3D::get_cast_shadow);

	ClassDB::bind_method(D_METHOD("set_cull_distance", "distance"), &FoliageType3D::set_cull_distance);
	ClassDB::bind_method(D_METHOD("get_cull_distance"), &FoliageType3D::get_cull_distance);

	ClassDB::bind_method(D_METHOD("set_fade_range", "range"), &FoliageType3D::set_fade_range);
	ClassDB::bind_method(D_METHOD("get_fade_range"), &FoliageType3D::get_fade_range);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type_name"), "set_type_name", "get_type_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material_override", "get_material_override");

	ADD_GROUP("Painting & Density", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "0.01,50.0,0.01"), "set_density", "get_density");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_spacing", PROPERTY_HINT_RANGE, "0.05,50.0,0.05,suffix:m"), "set_min_spacing", "get_min_spacing");

	ADD_GROUP("Transform Variation", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "min_scale"), "set_min_scale", "get_min_scale");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "max_scale"), "set_max_scale", "get_max_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "uniform_scale"), "set_uniform_scale", "is_uniform_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_rotation_y"), "set_random_rotation_y", "is_random_rotation_y");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_pitch_roll", PROPERTY_HINT_RANGE, "0.0,90.0,0.5,degrees"), "set_random_pitch_roll", "get_random_pitch_roll");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_align", PROPERTY_HINT_RANGE, "0.0,1.0,0.05"), "set_normal_align", "get_normal_align");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ground_offset", PROPERTY_HINT_RANGE, "-10.0,10.0,0.01,suffix:m"), "set_ground_offset", "get_ground_offset");

	ADD_GROUP("Placement Filters", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_altitude", PROPERTY_HINT_RANGE, "-10000.0,10000.0,0.5,suffix:m"), "set_min_altitude", "get_min_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_altitude", PROPERTY_HINT_RANGE, "-10000.0,10000.0,0.5,suffix:m"), "set_max_altitude", "get_max_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_slope_angle", PROPERTY_HINT_RANGE, "0.0,90.0,0.5,degrees"), "set_max_slope_angle", "get_max_slope_angle");

	ADD_GROUP("Rendering & Performance", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cast_shadow", PROPERTY_HINT_ENUM, "Off,On,Double-Sided,Shadows Only"), "set_cast_shadow", "get_cast_shadow");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cull_distance", PROPERTY_HINT_RANGE, "10.0,4000.0,5.0,suffix:m"), "set_cull_distance", "get_cull_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_range", PROPERTY_HINT_RANGE, "0.0,200.0,1.0,suffix:m"), "set_fade_range", "get_fade_range");
}

FoliageType3D::FoliageType3D() {
}

void FoliageType3D::set_type_name(const String &p_name) {
	type_name = p_name;
	emit_changed();
}

String FoliageType3D::get_type_name() const {
	return type_name;
}

void FoliageType3D::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
	emit_changed();
}

Ref<Mesh> FoliageType3D::get_mesh() const {
	return mesh;
}

void FoliageType3D::set_material_override(const Ref<Material> &p_material) {
	material_override = p_material;
	emit_changed();
}

Ref<Material> FoliageType3D::get_material_override() const {
	return material_override;
}

void FoliageType3D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
	emit_changed();
}

bool FoliageType3D::is_enabled() const {
	return enabled;
}

void FoliageType3D::set_density(float p_density) {
	density = MAX(0.001f, p_density);
	emit_changed();
}

float FoliageType3D::get_density() const {
	return density;
}

void FoliageType3D::set_min_spacing(float p_spacing) {
	min_spacing = MAX(0.01f, p_spacing);
	emit_changed();
}

float FoliageType3D::get_min_spacing() const {
	return min_spacing;
}

void FoliageType3D::set_min_scale(const Vector3 &p_scale) {
	min_scale = p_scale;
	emit_changed();
}

Vector3 FoliageType3D::get_min_scale() const {
	return min_scale;
}

void FoliageType3D::set_max_scale(const Vector3 &p_scale) {
	max_scale = p_scale;
	emit_changed();
}

Vector3 FoliageType3D::get_max_scale() const {
	return max_scale;
}

void FoliageType3D::set_uniform_scale(bool p_uniform) {
	uniform_scale = p_uniform;
	emit_changed();
}

bool FoliageType3D::is_uniform_scale() const {
	return uniform_scale;
}

void FoliageType3D::set_random_rotation_y(bool p_enabled) {
	random_rotation_y = p_enabled;
	emit_changed();
}

bool FoliageType3D::is_random_rotation_y() const {
	return random_rotation_y;
}

void FoliageType3D::set_random_pitch_roll(float p_degrees) {
	random_pitch_roll = CLAMP(p_degrees, 0.0f, 90.0f);
	emit_changed();
}

float FoliageType3D::get_random_pitch_roll() const {
	return random_pitch_roll;
}

void FoliageType3D::set_normal_align(float p_align) {
	normal_align = CLAMP(p_align, 0.0f, 1.0f);
	emit_changed();
}

float FoliageType3D::get_normal_align() const {
	return normal_align;
}

void FoliageType3D::set_ground_offset(float p_offset) {
	ground_offset = p_offset;
	emit_changed();
}

float FoliageType3D::get_ground_offset() const {
	return ground_offset;
}

void FoliageType3D::set_min_altitude(float p_alt) {
	min_altitude = p_alt;
	emit_changed();
}

float FoliageType3D::get_min_altitude() const {
	return min_altitude;
}

void FoliageType3D::set_max_altitude(float p_alt) {
	max_altitude = p_alt;
	emit_changed();
}

float FoliageType3D::get_max_altitude() const {
	return max_altitude;
}

void FoliageType3D::set_max_slope_angle(float p_degrees) {
	max_slope_angle = CLAMP(p_degrees, 0.0f, 90.0f);
	emit_changed();
}

float FoliageType3D::get_max_slope_angle() const {
	return max_slope_angle;
}

void FoliageType3D::set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_setting) {
	cast_shadow = p_setting;
	emit_changed();
}

GeometryInstance3D::ShadowCastingSetting FoliageType3D::get_cast_shadow() const {
	return cast_shadow;
}

void FoliageType3D::set_cull_distance(float p_dist) {
	cull_distance = MAX(1.0f, p_dist);
	emit_changed();
}

float FoliageType3D::get_cull_distance() const {
	return cull_distance;
}

void FoliageType3D::set_fade_range(float p_range) {
	fade_range = MAX(0.0f, p_range);
	emit_changed();
}

float FoliageType3D::get_fade_range() const {
	return fade_range;
}
