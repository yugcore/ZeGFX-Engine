/**************************************************************************/
/*  volumetric_clouds_3d.cpp                                              */
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

#include "volumetric_clouds_3d.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/rendering/rendering_server.h"

static void _set_or_add_gsp(RenderingServer *rs, const StringName &p_name, RenderingServerEnums::GlobalShaderParameterType p_type, const Variant &p_val) {
	if (rs->global_shader_parameter_get_type(p_name) == RenderingServerEnums::GLOBAL_VAR_TYPE_MAX) {
		rs->global_shader_parameter_add(p_name, p_type, p_val);
	} else {
		rs->global_shader_parameter_set(p_name, p_val);
	}
}

static const char *volumetric_cloud_shader_code =
		"shader_type spatial;\n"
		"render_mode unshaded, depth_draw_never, cull_front, fog_disabled;\n"
		"\n"
		"uniform float base_altitude = 1500.0;\n"
		"uniform float cloud_thickness = 2500.0;\n"
		"uniform float coverage : hint_range(0.0, 1.0) = 0.5;\n"
		"uniform float density : hint_range(0.0, 5.0) = 1.0;\n"
		"uniform float detail_scale = 1.0;\n"
		"uniform float detail_erosion : hint_range(0.0, 1.0) = 0.35;\n"
		"uniform vec3 wind_direction = vec3(1.0, 0.0, 0.2);\n"
		"uniform float wind_speed = 12.0;\n"
		"uniform vec4 albedo_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"uniform vec4 shadow_color : source_color = vec4(0.28, 0.32, 0.42, 1.0);\n"
		"uniform float silver_lining_intensity : hint_range(0.0, 5.0) = 1.8;\n"
		"uniform float silver_lining_spread : hint_range(0.01, 0.99) = 0.75;\n"
		"uniform float powder_effect_strength : hint_range(0.0, 1.0) = 0.6;\n"
		"\n"
		"// High-performance 3D Procedural Noise\n"
		"float hash31(vec3 p) {\n"
		"    p = fract(p * 0.3183099 + 0.1);\n"
		"    p *= 17.0;\n"
		"    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));\n"
		"}\n"
		"\n"
		"float noise3D(vec3 x) {\n"
		"    vec3 p = floor(x);\n"
		"    vec3 w = fract(x);\n"
		"    vec3 u = w * w * w * (w * (w * 6.0 - 15.0) + 10.0);\n"
		"    float n000 = hash31(p + vec3(0.0, 0.0, 0.0));\n"
		"    float n100 = hash31(p + vec3(1.0, 0.0, 0.0));\n"
		"    float n010 = hash31(p + vec3(0.0, 1.0, 0.0));\n"
		"    float n110 = hash31(p + vec3(1.0, 1.0, 0.0));\n"
		"    float n001 = hash31(p + vec3(0.0, 0.0, 1.0));\n"
		"    float n101 = hash31(p + vec3(1.0, 0.0, 1.0));\n"
		"    float n011 = hash31(p + vec3(0.0, 1.0, 1.0));\n"
		"    float n111 = hash31(p + vec3(1.0, 1.0, 1.0));\n"
		"    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y), mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);\n"
		"}\n"
		"\n"
		"float worley3D(vec3 p) {\n"
		"    vec3 id = floor(p);\n"
		"    vec3 f = fract(p);\n"
		"    float min_dist = 1.0;\n"
		"    for (int z = -1; z <= 1; z++) {\n"
		"        for (int y = -1; y <= 1; y++) {\n"
		"            for (int x = -1; x <= 1; x++) {\n"
		"                vec3 offset = vec3(float(x), float(y), float(z));\n"
		"                vec3 h = vec3(hash31(id + offset), hash31(id + offset + 1.3), hash31(id + offset + 2.7));\n"
		"                vec3 diff = offset + h - f;\n"
		"                min_dist = min(min_dist, dot(diff, diff));\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"    return clamp(1.0 - sqrt(min_dist), 0.0, 1.0);\n"
		"}\n"
		"\n"
		"float fbmPerlinWorley(vec3 p) {\n"
		"    float perlin = noise3D(p) * 0.5 + noise3D(p * 2.0) * 0.25 + noise3D(p * 4.0) * 0.125;\n"
		"    float worley = worley3D(p * 2.0) * 0.625 + worley3D(p * 4.0) * 0.25 + worley3D(p * 8.0) * 0.125;\n"
		"    return clamp(perlin * 0.6 + worley * 0.4, 0.0, 1.0);\n"
		"}\n"
		"\n"
		"// Dual Henyey-Greenstein Phase Function\n"
		"float hgPhase(float cos_theta, float g) {\n"
		"    float g2 = g * g;\n"
		"    return (1.0 - g2) / (4.0 * 3.14159265 * pow(max(1e-4, 1.0 + g2 - 2.0 * g * cos_theta), 1.5));\n"
		"}\n"
		"\n"
		"float dualHGPhase(float cos_theta, float g_fwd, float g_back, float blend) {\n"
		"    return mix(hgPhase(cos_theta, g_back), hgPhase(cos_theta, g_fwd), blend);\n"
		"}\n"
		"\n"
		"// Cloud Height Density Gradient Curve\n"
		"float getDensityHeightGradient(float h) {\n"
		"    return smoothstep(0.0, 0.18, h) * smoothstep(1.0, 0.65, h);\n"
		"}\n"
		"\n"
		"// Atmospheric Cloud Volume Density Sampler\n"
		"float sampleCloudDensity(vec3 p, float h, vec3 wind_disp) {\n"
		"    vec3 sample_pos = (p + wind_disp) * 0.00028 * detail_scale;\n"
		"    float height_grad = getDensityHeightGradient(h);\n"
		"    float base_noise = fbmPerlinWorley(sample_pos);\n"
		"    float density_val = (base_noise - (1.0 - coverage)) / max(0.001, coverage);\n"
		"    density_val *= height_grad;\n"
		"    if (density_val <= 0.0) return 0.0;\n"
		"    float detail_noise = worley3D(sample_pos * 6.0);\n"
		"    density_val = max(0.0, density_val - detail_noise * detail_erosion * (1.0 - height_grad * 0.5));\n"
		"    return density_val * density;\n"
		"}\n"
		"\n"
		"void vertex() {\n"
		"    POSITION = vec4(VERTEX, 1.0);\n"
		"}\n"
		"\n"
		"void fragment() {\n"
		"    vec3 view_dir = normalize((INV_VIEW_MATRIX * vec4(VIEW, 0.0)).xyz);\n"
		"    if (view_dir.y <= 0.015) {\n"
		"        ALPHA = 0.0;\n"
		"        discard;\n"
		"    }\n"
		"\n"
		"    // Solar Direction & Phase\n"
		"    vec3 sun_dir = normalize(vec3(0.35, 0.65, 0.45));\n"
		"    float cos_theta = dot(view_dir, sun_dir);\n"
		"    float phase = dualHGPhase(cos_theta, silver_lining_spread, -0.3, 0.7) * (1.0 + silver_lining_intensity * 2.0);\n"
		"\n"
		"    // Multi-Step Volumetric Raymarch (18 steps with altitude integration)\n"
		"    const int STEPS = 18;\n"
		"    float step_length = cloud_thickness / float(STEPS);\n"
		"    vec3 wind_norm = normalize(wind_direction + vec3(1e-5));\n"
		"    vec3 wind_disp = wind_norm * (TIME * wind_speed * 0.8);\n"
		"\n"
		"    float total_transmittance = 1.0;\n"
		"    vec3 accumulated_light = vec3(0.0);\n"
		"    float start_dist = base_altitude / max(0.08, view_dir.y);\n"
		"\n"
		"    for (int i = 0; i < STEPS; i++) {\n"
		"        float t_dist = start_dist + float(i) * (step_length / max(0.08, view_dir.y));\n"
		"        vec3 sample_point = view_dir * t_dist;\n"
		"        float h = float(i) / float(STEPS);\n"
		"        vec3 altitude_shear = vec3(wind_norm.x, 0.0, wind_norm.z) * (h * 400.0);\n"
		"        float sample_density = sampleCloudDensity(sample_point, h, wind_disp + altitude_shear);\n"
		"\n"
		"        if (sample_density > 0.001) {\n"
		"            // 4-Step Secondary Light March toward Sun (Self-Shadowing)\n"
		"            float light_step = 80.0;\n"
		"            float light_optical_depth = 0.0;\n"
		"            for (int j = 1; j <= 4; j++) {\n"
		"                vec3 light_sample_pos = sample_point + sun_dir * (float(j) * light_step);\n"
		"                light_optical_depth += sampleCloudDensity(light_sample_pos, clamp(h + float(j)*0.08, 0.0, 1.0), wind_disp);\n"
		"            }\n"
		"            float light_transmittance = exp(-light_optical_depth * 0.45);\n"
		"            float powder_sugar = 1.0 - exp(-sample_density * 4.0 * powder_effect_strength);\n"
		"\n"
		"            // Energy-conserving ambient + direct in-scattering\n"
		"            vec3 direct_light = albedo_color.rgb * light_transmittance * phase * powder_sugar * 1.8;\n"
		"            vec3 ambient_light = shadow_color.rgb * (0.35 + 0.65 * h);\n"
		"            vec3 step_color = direct_light + ambient_light;\n"
		"\n"
		"            float step_extinction = exp(-sample_density * (step_length * 0.0012));\n"
		"            accumulated_light += total_transmittance * (1.0 - step_extinction) * step_color;\n"
		"            total_transmittance *= step_extinction;\n"
		"\n"
		"            if (total_transmittance < 0.01) break;\n"
		"        }\n"
		"    }\n"
		"\n"
		"    float horizon_fade = smoothstep(0.015, 0.15, view_dir.y);\n"
		"    float final_alpha = clamp((1.0 - total_transmittance) * horizon_fade, 0.0, 1.0);\n"
		"    ALBEDO = accumulated_light;\n"
		"    ALPHA = final_alpha;\n"
		"}\n";

void VolumetricClouds3D::_init_cloud_dome() {
	if (!cloud_dome) {
		cloud_dome = Object::cast_to<MeshInstance3D>(get_node_or_null(NodePath("InternalCloudDome")));
		if (!cloud_dome) {
			cloud_dome = memnew(MeshInstance3D);
			cloud_dome->set_name("InternalCloudDome");
			cloud_dome->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);

			Ref<SphereMesh> sphere;
			sphere.instantiate();
			sphere->set_radius(4500.0f);
			sphere->set_height(9000.0f);
			sphere->set_radial_segments(64);
			sphere->set_rings(32);
			cloud_dome->set_mesh(sphere);

			cloud_shader.instantiate();
			cloud_shader->set_code(volumetric_cloud_shader_code);

			cloud_material.instantiate();
			cloud_material->set_shader(cloud_shader);
			cloud_dome->set_material_override(cloud_material);

			add_child(cloud_dome, false, INTERNAL_MODE_FRONT);
		}
	}
}

void VolumetricClouds3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_init_cloud_dome();
			_update_clouds();
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			if (cloud_dome && enabled) {
				// Keep dome centered around camera
				Viewport *vp = get_viewport();
				if (vp) {
					Camera3D *cam = vp->get_camera_3d();
					if (cam) {
						cloud_dome->set_global_position(cam->get_global_position());
					}
				}
			}
		} break;
	}
}

void VolumetricClouds3D::_update_clouds() {
	_init_cloud_dome();

	if (cloud_dome) {
		cloud_dome->set_visible(enabled);
	}

	if (cloud_material.is_valid()) {
		cloud_material->set_shader_parameter("base_altitude", base_altitude);
		cloud_material->set_shader_parameter("cloud_thickness", cloud_thickness);
		cloud_material->set_shader_parameter("coverage", coverage);
		cloud_material->set_shader_parameter("density", density);
		cloud_material->set_shader_parameter("detail_scale", detail_scale);
		cloud_material->set_shader_parameter("detail_erosion", detail_erosion);
		cloud_material->set_shader_parameter("wind_direction", wind_direction);
		cloud_material->set_shader_parameter("wind_speed", wind_speed);
		cloud_material->set_shader_parameter("albedo_color", albedo_color);
		cloud_material->set_shader_parameter("shadow_color", shadow_color);
		cloud_material->set_shader_parameter("silver_lining_intensity", silver_lining_intensity);
		cloud_material->set_shader_parameter("silver_lining_spread", silver_lining_spread);
		cloud_material->set_shader_parameter("powder_effect_strength", powder_effect_strength);
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		_set_or_add_gsp(rs, "volumetric_clouds_enabled", RenderingServerEnums::GLOBAL_VAR_TYPE_BOOL, enabled);
		_set_or_add_gsp(rs, "volumetric_clouds_base_altitude", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, base_altitude);
		_set_or_add_gsp(rs, "volumetric_clouds_thickness", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, cloud_thickness);
		_set_or_add_gsp(rs, "volumetric_clouds_coverage", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, coverage);
		_set_or_add_gsp(rs, "volumetric_clouds_density", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, density);
		_set_or_add_gsp(rs, "volumetric_clouds_detail_scale", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, detail_scale);
		_set_or_add_gsp(rs, "volumetric_clouds_detail_erosion", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, detail_erosion);
		_set_or_add_gsp(rs, "volumetric_clouds_albedo", RenderingServerEnums::GLOBAL_VAR_TYPE_COLOR, albedo_color);
		_set_or_add_gsp(rs, "volumetric_clouds_shadow_color", RenderingServerEnums::GLOBAL_VAR_TYPE_COLOR, shadow_color);
		_set_or_add_gsp(rs, "volumetric_clouds_silver_lining", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, silver_lining_intensity);
		_set_or_add_gsp(rs, "volumetric_clouds_powder_effect", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, powder_effect_strength);
		_set_or_add_gsp(rs, "volumetric_clouds_shadow_ground", RenderingServerEnums::GLOBAL_VAR_TYPE_FLOAT, cast_shadows_on_ground ? shadow_ground_intensity : 0.0f);
	}
}

void VolumetricClouds3D::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	_update_clouds();
}

bool VolumetricClouds3D::is_enabled() const {
	return enabled;
}

void VolumetricClouds3D::set_quality(Quality p_quality) {
	quality = p_quality;
	_update_clouds();
}

VolumetricClouds3D::Quality VolumetricClouds3D::get_quality() const {
	return quality;
}

void VolumetricClouds3D::set_base_altitude(float p_altitude) {
	base_altitude = MAX(0.0f, p_altitude);
	_update_clouds();
}

float VolumetricClouds3D::get_base_altitude() const {
	return base_altitude;
}

void VolumetricClouds3D::set_cloud_thickness(float p_thickness) {
	cloud_thickness = MAX(100.0f, p_thickness);
	_update_clouds();
}

float VolumetricClouds3D::get_cloud_thickness() const {
	return cloud_thickness;
}

void VolumetricClouds3D::set_coverage(float p_coverage) {
	coverage = CLAMP(p_coverage, 0.0f, 1.0f);
	_update_clouds();
}

float VolumetricClouds3D::get_coverage() const {
	return coverage;
}

void VolumetricClouds3D::set_density(float p_density) {
	density = MAX(0.0f, p_density);
	_update_clouds();
}

float VolumetricClouds3D::get_density() const {
	return density;
}

void VolumetricClouds3D::set_detail_scale(float p_scale) {
	detail_scale = MAX(0.01f, p_scale);
	_update_clouds();
}

float VolumetricClouds3D::get_detail_scale() const {
	return detail_scale;
}

void VolumetricClouds3D::set_detail_erosion(float p_erosion) {
	detail_erosion = CLAMP(p_erosion, 0.0f, 1.0f);
	_update_clouds();
}

float VolumetricClouds3D::get_detail_erosion() const {
	return detail_erosion;
}

void VolumetricClouds3D::set_wind_direction(const Vector3 &p_dir) {
	wind_direction = p_dir.normalized();
	_update_clouds();
}

Vector3 VolumetricClouds3D::get_wind_direction() const {
	return wind_direction;
}

void VolumetricClouds3D::set_wind_speed(float p_speed) {
	wind_speed = p_speed;
	_update_clouds();
}

float VolumetricClouds3D::get_wind_speed() const {
	return wind_speed;
}

void VolumetricClouds3D::set_albedo_color(const Color &p_color) {
	albedo_color = p_color;
	_update_clouds();
}

Color VolumetricClouds3D::get_albedo_color() const {
	return albedo_color;
}

void VolumetricClouds3D::set_shadow_color(const Color &p_color) {
	shadow_color = p_color;
	_update_clouds();
}

Color VolumetricClouds3D::get_shadow_color() const {
	return shadow_color;
}

void VolumetricClouds3D::set_silver_lining_intensity(float p_intensity) {
	silver_lining_intensity = MAX(0.0f, p_intensity);
	_update_clouds();
}

float VolumetricClouds3D::get_silver_lining_intensity() const {
	return silver_lining_intensity;
}

void VolumetricClouds3D::set_silver_lining_spread(float p_spread) {
	silver_lining_spread = CLAMP(p_spread, 0.01f, 0.99f);
	_update_clouds();
}

float VolumetricClouds3D::get_silver_lining_spread() const {
	return silver_lining_spread;
}

void VolumetricClouds3D::set_powder_effect_strength(float p_strength) {
	powder_effect_strength = CLAMP(p_strength, 0.0f, 1.0f);
	_update_clouds();
}

float VolumetricClouds3D::get_powder_effect_strength() const {
	return powder_effect_strength;
}

void VolumetricClouds3D::set_shadow_density(float p_density) {
	shadow_density = CLAMP(p_density, 0.0f, 1.0f);
	_update_clouds();
}

float VolumetricClouds3D::get_shadow_density() const {
	return shadow_density;
}

void VolumetricClouds3D::set_cast_shadows_on_ground(bool p_cast) {
	cast_shadows_on_ground = p_cast;
	_update_clouds();
}

bool VolumetricClouds3D::is_casting_shadows_on_ground() const {
	return cast_shadows_on_ground;
}

void VolumetricClouds3D::set_shadow_ground_intensity(float p_intensity) {
	shadow_ground_intensity = CLAMP(p_intensity, 0.0f, 1.0f);
	_update_clouds();
}

float VolumetricClouds3D::get_shadow_ground_intensity() const {
	return shadow_ground_intensity;
}

void VolumetricClouds3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &VolumetricClouds3D::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &VolumetricClouds3D::is_enabled);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &VolumetricClouds3D::set_quality);
	ClassDB::bind_method(D_METHOD("get_quality"), &VolumetricClouds3D::get_quality);

	ClassDB::bind_method(D_METHOD("set_base_altitude", "altitude"), &VolumetricClouds3D::set_base_altitude);
	ClassDB::bind_method(D_METHOD("get_base_altitude"), &VolumetricClouds3D::get_base_altitude);

	ClassDB::bind_method(D_METHOD("set_cloud_thickness", "thickness"), &VolumetricClouds3D::set_cloud_thickness);
	ClassDB::bind_method(D_METHOD("get_cloud_thickness"), &VolumetricClouds3D::get_cloud_thickness);

	ClassDB::bind_method(D_METHOD("set_coverage", "coverage"), &VolumetricClouds3D::set_coverage);
	ClassDB::bind_method(D_METHOD("get_coverage"), &VolumetricClouds3D::get_coverage);

	ClassDB::bind_method(D_METHOD("set_density", "density"), &VolumetricClouds3D::set_density);
	ClassDB::bind_method(D_METHOD("get_density"), &VolumetricClouds3D::get_density);

	ClassDB::bind_method(D_METHOD("set_detail_scale", "scale"), &VolumetricClouds3D::set_detail_scale);
	ClassDB::bind_method(D_METHOD("get_detail_scale"), &VolumetricClouds3D::get_detail_scale);

	ClassDB::bind_method(D_METHOD("set_detail_erosion", "erosion"), &VolumetricClouds3D::set_detail_erosion);
	ClassDB::bind_method(D_METHOD("get_detail_erosion"), &VolumetricClouds3D::get_detail_erosion);

	ClassDB::bind_method(D_METHOD("set_wind_direction", "direction"), &VolumetricClouds3D::set_wind_direction);
	ClassDB::bind_method(D_METHOD("get_wind_direction"), &VolumetricClouds3D::get_wind_direction);

	ClassDB::bind_method(D_METHOD("set_wind_speed", "speed"), &VolumetricClouds3D::set_wind_speed);
	ClassDB::bind_method(D_METHOD("get_wind_speed"), &VolumetricClouds3D::get_wind_speed);

	ClassDB::bind_method(D_METHOD("set_albedo_color", "color"), &VolumetricClouds3D::set_albedo_color);
	ClassDB::bind_method(D_METHOD("get_albedo_color"), &VolumetricClouds3D::get_albedo_color);

	ClassDB::bind_method(D_METHOD("set_shadow_color", "color"), &VolumetricClouds3D::set_shadow_color);
	ClassDB::bind_method(D_METHOD("get_shadow_color"), &VolumetricClouds3D::get_shadow_color);

	ClassDB::bind_method(D_METHOD("set_silver_lining_intensity", "intensity"), &VolumetricClouds3D::set_silver_lining_intensity);
	ClassDB::bind_method(D_METHOD("get_silver_lining_intensity"), &VolumetricClouds3D::get_silver_lining_intensity);

	ClassDB::bind_method(D_METHOD("set_silver_lining_spread", "spread"), &VolumetricClouds3D::set_silver_lining_spread);
	ClassDB::bind_method(D_METHOD("get_silver_lining_spread"), &VolumetricClouds3D::get_silver_lining_spread);

	ClassDB::bind_method(D_METHOD("set_powder_effect_strength", "strength"), &VolumetricClouds3D::set_powder_effect_strength);
	ClassDB::bind_method(D_METHOD("get_powder_effect_strength"), &VolumetricClouds3D::get_powder_effect_strength);

	ClassDB::bind_method(D_METHOD("set_shadow_density", "density"), &VolumetricClouds3D::set_shadow_density);
	ClassDB::bind_method(D_METHOD("get_shadow_density"), &VolumetricClouds3D::get_shadow_density);

	ClassDB::bind_method(D_METHOD("set_cast_shadows_on_ground", "cast"), &VolumetricClouds3D::set_cast_shadows_on_ground);
	ClassDB::bind_method(D_METHOD("is_casting_shadows_on_ground"), &VolumetricClouds3D::is_casting_shadows_on_ground);

	ClassDB::bind_method(D_METHOD("set_shadow_ground_intensity", "intensity"), &VolumetricClouds3D::set_shadow_ground_intensity);
	ClassDB::bind_method(D_METHOD("get_shadow_ground_intensity"), &VolumetricClouds3D::get_shadow_ground_intensity);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quality", PROPERTY_HINT_ENUM, "Low,Medium,High,Ultra"), "set_quality", "get_quality");

	ADD_GROUP("Geometry", "base_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_altitude", PROPERTY_HINT_RANGE, "0,15000,10,suffix:m"), "set_base_altitude", "get_base_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cloud_thickness", PROPERTY_HINT_RANGE, "100,10000,10,suffix:m"), "set_cloud_thickness", "get_cloud_thickness");

	ADD_GROUP("Shape & Density", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coverage", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_coverage", "get_coverage");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "0,5,0.01"), "set_density", "get_density");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "detail_scale", PROPERTY_HINT_RANGE, "0.1,10,0.1"), "set_detail_scale", "get_detail_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "detail_erosion", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_detail_erosion", "get_detail_erosion");

	ADD_GROUP("Wind", "wind_");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "wind_direction"), "set_wind_direction", "get_wind_direction");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_speed", PROPERTY_HINT_RANGE, "0,100,0.1,suffix:m/s"), "set_wind_speed", "get_wind_speed");

	ADD_GROUP("Lighting & Optics", "");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color"), "set_albedo_color", "get_albedo_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "shadow_color"), "set_shadow_color", "get_shadow_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "silver_lining_intensity", PROPERTY_HINT_RANGE, "0,5,0.01"), "set_silver_lining_intensity", "get_silver_lining_intensity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "silver_lining_spread", PROPERTY_HINT_RANGE, "0.01,0.99,0.01"), "set_silver_lining_spread", "get_silver_lining_spread");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "powder_effect_strength", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_powder_effect_strength", "get_powder_effect_strength");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shadow_density", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_shadow_density", "get_shadow_density");

	ADD_GROUP("Ground Shadows", "shadow_ground_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cast_shadows_on_ground"), "set_cast_shadows_on_ground", "is_casting_shadows_on_ground");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shadow_ground_intensity", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_shadow_ground_intensity", "get_shadow_ground_intensity");

	BIND_ENUM_CONSTANT(QUALITY_LOW);
	BIND_ENUM_CONSTANT(QUALITY_MEDIUM);
	BIND_ENUM_CONSTANT(QUALITY_HIGH);
	BIND_ENUM_CONSTANT(QUALITY_ULTRA);
}

VolumetricClouds3D::VolumetricClouds3D() {
}

VolumetricClouds3D::~VolumetricClouds3D() {
}
