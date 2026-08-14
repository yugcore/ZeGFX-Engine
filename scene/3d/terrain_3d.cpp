/**************************************************************************/
/*  terrain_3d.cpp                                                        */
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

#include "terrain_3d.h"

#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/shader.h"

#include <cmath>

void Terrain3D::_init_debug_materials() {
	if (!debug_lod_materials.is_empty()) return;

	Color lod_colors[] = {
		Color(0.2f, 0.9f, 0.2f), // LOD 0: Bright Green
		Color(0.2f, 0.8f, 1.0f), // LOD 1: Cyan
		Color(1.0f, 0.9f, 0.2f), // LOD 2: Yellow
		Color(1.0f, 0.5f, 0.1f), // LOD 3: Orange
		Color(0.9f, 0.2f, 0.2f), // LOD 4: Red
		Color(0.8f, 0.2f, 0.9f), // LOD 5: Magenta
	};

	for (int i = 0; i < 6; ++i) {
		Ref<StandardMaterial3D> mat;
		mat.instantiate();
		mat->set_albedo(lod_colors[i]);
		mat->set_roughness(0.8f);
		mat->set_cull_mode(BaseMaterial3D::CULL_BACK);
		debug_lod_materials.push_back(mat);
	}
}

static const char *s_terrain_default_shader =
	"shader_type spatial;\n"
	"render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;\n\n"
	"uniform bool auto_material_enabled = true;\n"
	"uniform sampler2D splatmap_tex : source_color, filter_linear_mipmap;\n\n"
	"uniform vec4 layer_0_color : source_color = vec4(0.28, 0.48, 0.20, 1.0);\n"
	"uniform sampler2D layer_0_albedo : source_color, filter_linear_mipmap_anisotropic;\n"
	"uniform sampler2D layer_0_normal : hint_normal, filter_linear_mipmap_anisotropic;\n"
	"uniform float layer_0_roughness : hint_range(0.0, 1.0) = 0.8;\n"
	"uniform float layer_0_uv_scale = 20.0;\n\n"
	"uniform vec4 layer_1_color : source_color = vec4(0.44, 0.42, 0.40, 1.0);\n"
	"uniform sampler2D layer_1_albedo : source_color, filter_linear_mipmap_anisotropic;\n"
	"uniform sampler2D layer_1_normal : hint_normal, filter_linear_mipmap_anisotropic;\n"
	"uniform float layer_1_roughness : hint_range(0.0, 1.0) = 0.9;\n"
	"uniform float layer_1_uv_scale = 15.0;\n\n"
	"uniform vec4 layer_2_color : source_color = vec4(0.92, 0.95, 0.98, 1.0);\n"
	"uniform sampler2D layer_2_albedo : source_color, filter_linear_mipmap_anisotropic;\n"
	"uniform sampler2D layer_2_normal : hint_normal, filter_linear_mipmap_anisotropic;\n"
	"uniform float layer_2_roughness : hint_range(0.0, 1.0) = 0.4;\n"
	"uniform float layer_2_uv_scale = 20.0;\n\n"
	"uniform vec4 layer_3_color : source_color = vec4(0.68, 0.58, 0.42, 1.0);\n"
	"uniform sampler2D layer_3_albedo : source_color, filter_linear_mipmap_anisotropic;\n"
	"uniform sampler2D layer_3_normal : hint_normal, filter_linear_mipmap_anisotropic;\n"
	"uniform float layer_3_roughness : hint_range(0.0, 1.0) = 0.85;\n"
	"uniform float layer_3_uv_scale = 20.0;\n\n"
	"uniform float slope_threshold : hint_range(0.0, 1.0) = 0.65;\n"
	"uniform float slope_sharpness : hint_range(1.0, 16.0) = 6.0;\n"
	"uniform float snow_altitude = 75.0;\n"
	"uniform float snow_falloff = 15.0;\n"
	"uniform float sand_altitude = 5.0;\n"
	"uniform float sand_falloff = 5.0;\n"
	"uniform float macro_variation : hint_range(0.0, 1.0) = 0.25;\n"
	"uniform bool triplanar_cliffs = true;\n\n"
	"varying vec3 v_world_pos;\n"
	"varying vec3 v_world_normal;\n\n"
	"void vertex() {\n"
	"	v_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;\n"
	"	v_world_normal = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);\n"
	"}\n\n"
	"vec4 sample_triplanar(sampler2D tex, vec3 pos, vec3 norm, float scale) {\n"
	"	vec3 triblend = pow(abs(norm), vec3(4.0));\n"
	"	triblend /= max(0.0001, triblend.x + triblend.y + triblend.z);\n"
	"	vec4 col_x = texture(tex, pos.zy * scale);\n"
	"	vec4 col_y = texture(tex, pos.xz * scale);\n"
	"	vec4 col_z = texture(tex, pos.xy * scale);\n"
	"	return col_x * triblend.x + col_y * triblend.y + col_z * triblend.z;\n"
	"}\n\n"
	"void fragment() {\n"
	"	float slope = 1.0 - clamp(abs(v_world_normal.y), 0.0, 1.0);\n"
	"	vec4 weights = vec4(0.0);\n\n"
	"	if (auto_material_enabled) {\n"
	"		float cliff_w = clamp(pow(slope / max(0.001, slope_threshold), slope_sharpness), 0.0, 1.0);\n"
	"		float snow_w = clamp((v_world_pos.y - snow_altitude) / max(0.001, snow_falloff), 0.0, 1.0);\n"
	"		float sand_w = clamp((sand_altitude - v_world_pos.y) / max(0.001, sand_falloff), 0.0, 1.0);\n"
	"		float base_w = max(0.0, 1.0 - cliff_w - snow_w - sand_w);\n"
	"		weights = vec4(base_w, cliff_w, snow_w, sand_w);\n"
	"		weights /= max(0.001, weights.r + weights.g + weights.b + weights.a);\n"
	"	} else {\n"
	"		weights = texture(splatmap_tex, UV);\n"
	"		weights /= max(0.001, weights.r + weights.g + weights.b + weights.a);\n"
	"	}\n\n"
	"	vec2 uv0 = UV * layer_0_uv_scale;\n"
	"	vec4 c0 = layer_0_color;\n"
	"	vec4 tex0 = texture(layer_0_albedo, uv0);\n"
	"	if (tex0.a > 0.01) c0 *= tex0;\n\n"
	"	vec4 c1 = layer_1_color;\n"
	"	if (triplanar_cliffs) {\n"
	"		vec4 tex1 = sample_triplanar(layer_1_albedo, v_world_pos, v_world_normal, layer_1_uv_scale * 0.05);\n"
	"		if (tex1.a > 0.01) c1 *= tex1;\n"
	"	} else {\n"
	"		vec2 uv1 = UV * layer_1_uv_scale;\n"
	"		vec4 tex1 = texture(layer_1_albedo, uv1);\n"
	"		if (tex1.a > 0.01) c1 *= tex1;\n"
	"	}\n\n"
	"	vec2 uv2 = UV * layer_2_uv_scale;\n"
	"	vec4 c2 = layer_2_color;\n"
	"	vec4 tex2 = texture(layer_2_albedo, uv2);\n"
	"	if (tex2.a > 0.01) c2 *= tex2;\n\n"
	"	vec2 uv3 = UV * layer_3_uv_scale;\n"
	"	vec4 c3 = layer_3_color;\n"
	"	vec4 tex3 = texture(layer_3_albedo, uv3);\n"
	"	if (tex3.a > 0.01) c3 *= tex3;\n\n"
	"	vec3 final_albedo = c0.rgb * weights.r + c1.rgb * weights.g + c2.rgb * weights.b + c3.rgb * weights.a;\n"
	"	if (macro_variation > 0.0) {\n"
	"		float macro_noise = sin(v_world_pos.x * 0.02) * cos(v_world_pos.z * 0.02);\n"
	"		final_albedo = mix(final_albedo, final_albedo * (1.0 + macro_noise * 0.25), macro_variation);\n"
	"	}\n\n"
	"	ALBEDO = final_albedo;\n"
	"	ROUGHNESS = layer_0_roughness * weights.r + layer_1_roughness * weights.g + layer_2_roughness * weights.b + layer_3_roughness * weights.a;\n"
	"}\n";

void Terrain3D::_ensure_default_material() {
	if (default_terrain_material.is_valid()) return;

	default_terrain_shader.instantiate();
	default_terrain_shader->set_code(s_terrain_default_shader);

	default_terrain_material.instantiate();
	default_terrain_material->set_shader(default_terrain_shader);

	_sync_material_uniforms();
}

void Terrain3D::_sync_material_uniforms() {
	if (!default_terrain_material.is_valid()) return;

	default_terrain_material->set_shader_parameter("auto_material_enabled", auto_material_enabled);
	default_terrain_material->set_shader_parameter("splatmap_tex", splatmap_texture);

	default_terrain_material->set_shader_parameter("layer_0_color", layer_0_color);
	default_terrain_material->set_shader_parameter("layer_0_albedo", layer_0_albedo);
	default_terrain_material->set_shader_parameter("layer_0_normal", layer_0_normal);
	default_terrain_material->set_shader_parameter("layer_0_roughness", layer_0_roughness);
	default_terrain_material->set_shader_parameter("layer_0_uv_scale", layer_0_uv_scale);

	default_terrain_material->set_shader_parameter("layer_1_color", layer_1_color);
	default_terrain_material->set_shader_parameter("layer_1_albedo", layer_1_albedo);
	default_terrain_material->set_shader_parameter("layer_1_normal", layer_1_normal);
	default_terrain_material->set_shader_parameter("layer_1_roughness", layer_1_roughness);
	default_terrain_material->set_shader_parameter("layer_1_uv_scale", layer_1_uv_scale);

	default_terrain_material->set_shader_parameter("layer_2_color", layer_2_color);
	default_terrain_material->set_shader_parameter("layer_2_albedo", layer_2_albedo);
	default_terrain_material->set_shader_parameter("layer_2_normal", layer_2_normal);
	default_terrain_material->set_shader_parameter("layer_2_roughness", layer_2_roughness);
	default_terrain_material->set_shader_parameter("layer_2_uv_scale", layer_2_uv_scale);

	default_terrain_material->set_shader_parameter("layer_3_color", layer_3_color);
	default_terrain_material->set_shader_parameter("layer_3_albedo", layer_3_albedo);
	default_terrain_material->set_shader_parameter("layer_3_normal", layer_3_normal);
	default_terrain_material->set_shader_parameter("layer_3_roughness", layer_3_roughness);
	default_terrain_material->set_shader_parameter("layer_3_uv_scale", layer_3_uv_scale);

	default_terrain_material->set_shader_parameter("slope_threshold", slope_threshold);
	default_terrain_material->set_shader_parameter("slope_sharpness", slope_sharpness);
	default_terrain_material->set_shader_parameter("snow_altitude", snow_altitude);
	default_terrain_material->set_shader_parameter("snow_falloff", snow_falloff);
	default_terrain_material->set_shader_parameter("sand_altitude", sand_altitude);
	default_terrain_material->set_shader_parameter("sand_falloff", sand_falloff);
	default_terrain_material->set_shader_parameter("macro_variation", macro_variation);
	default_terrain_material->set_shader_parameter("triplanar_cliffs", triplanar_cliffs);
}

Terrain3D::Terrain3D() {
	_init_debug_materials();
	_ensure_default_material();

	chunks_container = memnew(Node3D);
	chunks_container->set_name("ChunksContainer");
	add_child(chunks_container, false, INTERNAL_MODE_FRONT);

#ifndef PHYSICS_3D_DISABLED
	collision_body = memnew(StaticBody3D);
	collision_body->set_name("TerrainStaticBody");
	add_child(collision_body, false, INTERNAL_MODE_FRONT);

	collision_shape_node = memnew(CollisionShape3D);
	collision_shape_node->set_name("TerrainCollisionShape");
	collision_body->add_child(collision_shape_node, false, INTERNAL_MODE_FRONT);

	collision_shape.instantiate();
	collision_shape_node->set_shape(collision_shape);
#endif // PHYSICS_3D_DISABLED

	// Default fallback flat heightmap (65x65)
	map_width = 65;
	map_height = 65;
	heights.resize(map_width * map_height);
	for (int i = 0; i < heights.size(); ++i) {
		heights.write[i] = 0.0f;
	}

	set_process_internal(true);
}

Terrain3D::~Terrain3D() {
	_clear_chunks();
}

void Terrain3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (chunks.is_empty()) {
				rebuild_terrain();
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!chunks.is_empty() && (lod_enabled || debug_lod_colors)) {
				Viewport *vp = get_viewport();
				if (vp) {
					Camera3D *cam = vp->get_camera_3d();
					if (cam) {
						_update_lod(cam->get_global_position());
					}
				}
			}
		} break;
	}
}

void Terrain3D::_clear_chunks() {
	for (int i = 0; i < chunks.size(); ++i) {
		if (chunks[i].mesh_instance) {
			chunks[i].mesh_instance->queue_free();
		}
	}
	chunks.clear();
}

float Terrain3D::_get_height_raw(int x, int z) const {
	if (x < 0 || x >= map_width || z < 0 || z >= map_height || heights.is_empty()) {
		return 0.0f;
	}
	return heights[z * map_width + x];
}

Vector3 Terrain3D::_calc_normal(int x, int z) const {
	int x0 = MAX(0, x - 1);
	int x1 = MIN(map_width - 1, x + 1);
	int z0 = MAX(0, z - 1);
	int z1 = MIN(map_height - 1, z + 1);

	float z00 = _get_height_raw(x0, z0) * height_scale;
	float z01 = _get_height_raw(x, z0) * height_scale;
	float z02 = _get_height_raw(x1, z0) * height_scale;
	float z10 = _get_height_raw(x0, z) * height_scale;
	float z12 = _get_height_raw(x1, z) * height_scale;
	float z20 = _get_height_raw(x0, z1) * height_scale;
	float z21 = _get_height_raw(x, z1) * height_scale;
	float z22 = _get_height_raw(x1, z1) * height_scale;

	float dx = (z02 + 2.0f * z12 + z22) - (z00 + 2.0f * z10 + z20);
	float dz = (z20 + 2.0f * z21 + z22) - (z00 + 2.0f * z01 + z02);
	float scale = (x1 - x0) * cell_size * 4.0f;
	if (scale <= 0.0f) scale = 1.0f;

	Vector3 normal(-dx, scale, -dz);
	return normal.normalized();
}

void Terrain3D::_smooth_heights(int p_cycles) {
	if (p_cycles <= 0 || heights.is_empty() || map_width < 3 || map_height < 3) {
		return;
	}

	for (int iter = 0; iter < p_cycles; ++iter) {
		Vector<float> smoothed = heights;
		float *dst = smoothed.ptrw();
		const float *src = heights.ptr();

		for (int z = 0; z < map_height; ++z) {
			int z_prev = MAX(0, z - 1);
			int z_next = MIN(map_height - 1, z + 1);

			for (int x = 0; x < map_width; ++x) {
				int x_prev = MAX(0, x - 1);
				int x_next = MIN(map_width - 1, x + 1);

				float val = (
					src[z_prev * map_width + x_prev] * 0.0625f +
					src[z_prev * map_width + x] * 0.125f +
					src[z_prev * map_width + x_next] * 0.0625f +
					src[z * map_width + x_prev] * 0.125f +
					src[z * map_width + x] * 0.25f +
					src[z * map_width + x_next] * 0.125f +
					src[z_next * map_width + x_prev] * 0.0625f +
					src[z_next * map_width + x_next] * 0.0625f +
					src[z_next * map_width + x] * 0.125f
				);
				dst[z * map_width + x] = val;
			}
		}
		heights = smoothed;
	}
}

void Terrain3D::_load_heightmap_data() {
	Ref<Image> img;

	if (heightmap_texture.is_valid()) {
		img = heightmap_texture->get_image();
	} else if (!heightmap_file.is_empty()) {
		String ext = heightmap_file.get_extension().to_lower();
		if (ext == "r16" || ext == "raw" || ext == "r8") {
			Ref<FileAccess> f = FileAccess::open(heightmap_file, FileAccess::READ);
			if (f.is_valid()) {
				uint64_t len = f->get_length();
				bool is_16 = (ext == "r16" || (len % 2 == 0 && ext == "raw"));
				int samples = is_16 ? (len / 2) : len;
				int dim = (int)Math::sqrt((double)samples);
				if (dim * dim == samples) {
					map_width = dim;
					map_height = dim;
					heights.resize(samples);
					if (is_16) {
						for (int i = 0; i < samples; ++i) {
							uint16_t val = f->get_16();
							heights.write[i] = (float)val / 65535.0f;
						}
					} else {
						for (int i = 0; i < samples; ++i) {
							uint8_t val = f->get_8();
							heights.write[i] = (float)val / 255.0f;
						}
					}
					_smooth_heights(smoothing_cycles);
					return;
				}
			}
		} else {
			img = Image::load_from_file(heightmap_file);
			if (img.is_null() && ResourceLoader::exists(heightmap_file)) {
				Ref<Texture2D> tex = ResourceLoader::load(heightmap_file);
				if (tex.is_valid()) {
					img = tex->get_image();
				}
			}
		}
	}

	if (img.is_valid() && !img->is_empty()) {
		map_width = img->get_width();
		map_height = img->get_height();
		int total_samples = map_width * map_height;
		heights.resize(total_samples);

		Ref<Image> rf_img = img->duplicate();
		if (rf_img->get_format() != Image::FORMAT_RF) {
			rf_img->convert(Image::FORMAT_RF);
		}
		Vector<uint8_t> raw_bytes = rf_img->get_data();
		const float *float_ptr = (const float *)raw_bytes.ptr();
		if (float_ptr) {
			for (int i = 0; i < total_samples; ++i) {
				heights.write[i] = float_ptr[i];
			}
		} else {
			for (int y = 0; y < map_height; ++y) {
				for (int x = 0; x < map_width; ++x) {
					heights.write[y * map_width + x] = img->get_pixel(x, y).r;
				}
			}
		}
		_smooth_heights(smoothing_cycles);
	} else {
		// Fallback default flat plane (65x65)
		map_width = 65;
		map_height = 65;
		heights.resize(map_width * map_height);
		for (int i = 0; i < heights.size(); ++i) {
			heights.write[i] = 0.0f;
		}
	}
}

Ref<ArrayMesh> Terrain3D::_create_chunk_lod_mesh(int cx, int cz, int p_lod, AABB &r_aabb) {
	int base_step = MAX(4, chunk_size);
	int start_x = cx * base_step;
	int start_z = cz * base_step;
	int end_x = MIN(map_width - 1, start_x + base_step);
	int end_z = MIN(map_height - 1, start_z + base_step);

	int raw_quad_w = end_x - start_x;
	int raw_quad_h = end_z - start_z;
	if (raw_quad_w <= 0 || raw_quad_h <= 0) return Ref<ArrayMesh>();

	int stride = 1 << p_lod; // 1, 2, 4, 8...
	stride = MIN(stride, MIN(raw_quad_w, raw_quad_h));
	if (stride < 1) stride = 1;

	int quad_w = (raw_quad_w + stride - 1) / stride;
	int quad_h = (raw_quad_h + stride - 1) / stride;
	int vert_w = quad_w + 1;
	int vert_h = quad_h + 1;

	float origin_x = center_pivot ? -(map_width - 1) * cell_size * 0.5f : 0.0f;
	float origin_z = center_pivot ? -(map_height - 1) * cell_size * 0.5f : 0.0f;

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedFloat32Array tangents;
	PackedVector2Array uvs;
	PackedInt32Array indices;

	int grid_vert_count = vert_w * vert_h;
	vertices.resize(grid_vert_count);
	normals.resize(grid_vert_count);
	tangents.resize(grid_vert_count * 4);
	uvs.resize(grid_vert_count);

	Vector3 *v_ptr = vertices.ptrw();
	Vector3 *n_ptr = normals.ptrw();
	float *t_ptr = tangents.ptrw();
	Vector2 *uv_ptr = uvs.ptrw();

	Vector3 aabb_min(1e9f, 1e9f, 1e9f);
	Vector3 aabb_max(-1e9f, -1e9f, -1e9f);

	for (int lz = 0; lz < vert_h; ++lz) {
		int gz = MIN(map_height - 1, start_z + lz * stride);
		float wz = origin_z + gz * cell_size;

		for (int lx = 0; lx < vert_w; ++lx) {
			int gx = MIN(map_width - 1, start_x + lx * stride);
			float wx = origin_x + gx * cell_size;
			float wy = _get_height_raw(gx, gz) * height_scale;

			int v_idx = lz * vert_w + lx;
			Vector3 pos(wx, wy, wz);
			v_ptr[v_idx] = pos;
			n_ptr[v_idx] = _calc_normal(gx, gz);

			t_ptr[v_idx * 4 + 0] = 1.0f;
			t_ptr[v_idx * 4 + 1] = 0.0f;
			t_ptr[v_idx * 4 + 2] = 0.0f;
			t_ptr[v_idx * 4 + 3] = 1.0f;

			uv_ptr[v_idx] = Vector2((float)gx / (map_width - 1), (float)gz / (map_height - 1));

			aabb_min.x = MIN(aabb_min.x, pos.x);
			aabb_min.y = MIN(aabb_min.y, pos.y);
			aabb_min.z = MIN(aabb_min.z, pos.z);

			aabb_max.x = MAX(aabb_max.x, pos.x);
			aabb_max.y = MAX(aabb_max.y, pos.y);
			aabb_max.z = MAX(aabb_max.z, pos.z);
		}
	}

	// Quad surface indices
	for (int lz = 0; lz < quad_h; ++lz) {
		for (int lx = 0; lx < quad_w; ++lx) {
			int v00 = lz * vert_w + lx;
			int v10 = lz * vert_w + (lx + 1);
			int v01 = (lz + 1) * vert_w + lx;
			int v11 = (lz + 1) * vert_w + (lx + 1);

			// Triangle 1
			indices.push_back(v00);
			indices.push_back(v10);
			indices.push_back(v01);

			// Triangle 2
			indices.push_back(v10);
			indices.push_back(v11);
			indices.push_back(v01);
		}
	}

	// Perimeter Skirts
	if (skirt_height > 0.0f) {
		auto add_skirt_edge = [&](int i0, int i1) {
			Vector3 p0 = vertices[i0];
			Vector3 p1 = vertices[i1];
			Vector3 p0_skirt = p0 - Vector3(0, skirt_height, 0);
			Vector3 p1_skirt = p1 - Vector3(0, skirt_height, 0);

			int s0 = vertices.size();
			int s1 = s0 + 1;

			vertices.push_back(p0_skirt);
			vertices.push_back(p1_skirt);

			normals.push_back(normals[i0]);
			normals.push_back(normals[i1]);

			for (int k = 0; k < 8; ++k) tangents.push_back(0.0f);

			uvs.push_back(uvs[i0]);
			uvs.push_back(uvs[i1]);

			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(s0);

			indices.push_back(i1);
			indices.push_back(s1);
			indices.push_back(s0);
		};

		// North Edge (lz = 0)
		for (int lx = 0; lx < quad_w; ++lx) {
			add_skirt_edge(lx, lx + 1);
		}
		// South Edge (lz = quad_h)
		int south_offset = quad_h * vert_w;
		for (int lx = 0; lx < quad_w; ++lx) {
			add_skirt_edge(south_offset + lx + 1, south_offset + lx);
		}
		// West Edge (lx = 0)
		for (int lz = 0; lz < quad_h; ++lz) {
			add_skirt_edge((lz + 1) * vert_w, lz * vert_w);
		}
		// East Edge (lx = quad_w)
		for (int lz = 0; lz < quad_h; ++lz) {
			add_skirt_edge(lz * vert_w + quad_w, (lz + 1) * vert_w + quad_w);
		}
	}

	r_aabb = AABB(aabb_min, aabb_max - aabb_min);

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	arrays[Mesh::ARRAY_TANGENT] = tangents;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_INDEX] = indices;

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return mesh;
}

void Terrain3D::_rebuild_chunk(int cx, int cz) {
	for (int i = 0; i < chunks.size(); ++i) {
		TerrainChunk &tc = chunks.write[i];
		if (tc.chunk_x == cx && tc.chunk_z == cz && tc.mesh_instance) {
			int total_lods = CLAMP(lod_count, 1, 6);
			tc.lod_meshes.clear();
			AABB chunk_aabb;
			for (int l = 0; l < total_lods; ++l) {
				Ref<ArrayMesh> lod_m = _create_chunk_lod_mesh(cx, cz, l, chunk_aabb);
				if (lod_m.is_valid()) {
					tc.lod_meshes.push_back(lod_m);
				}
			}
			tc.aabb = chunk_aabb;
			if (!tc.lod_meshes.is_empty()) {
				int cur_lod = CLAMP(tc.current_lod, 0, tc.lod_meshes.size() - 1);
				tc.mesh_instance->set_mesh(tc.lod_meshes[cur_lod]);
			}
			return;
		}
	}
}

void Terrain3D::update_chunks_in_region(int p_min_gx, int p_min_gz, int p_max_gx, int p_max_gz) {
	int base_step = MAX(4, chunk_size);
	int min_cx = MAX(0, (p_min_gx - 1) / base_step);
	int max_cx = (p_max_gx + 1) / base_step;
	int min_cz = MAX(0, (p_min_gz - 1) / base_step);
	int max_cz = (p_max_gz + 1) / base_step;

	for (int cz = min_cz; cz <= max_cz; ++cz) {
		for (int cx = min_cx; cx <= max_cx; ++cx) {
			_rebuild_chunk(cx, cz);
		}
	}
}

void Terrain3D::_build_chunk_meshes() {
	_clear_chunks();

	if (map_width < 2 || map_height < 2 || heights.is_empty()) {
		return;
	}

	_ensure_default_material();
	_sync_material_uniforms();

	int base_step = MAX(4, chunk_size);
	int chunks_x = (map_width - 1 + base_step - 1) / base_step;
	int chunks_z = (map_height - 1 + base_step - 1) / base_step;

	int total_lods = CLAMP(lod_count, 1, 6);
	Ref<Material> active_mat = material.is_valid() ? material : Ref<Material>(default_terrain_material);

	for (int cz = 0; cz < chunks_z; ++cz) {
		for (int cx = 0; cx < chunks_x; ++cx) {
			TerrainChunk tc;
			tc.chunk_x = cx;
			tc.chunk_z = cz;
			tc.current_lod = 0;

			AABB chunk_aabb;
			for (int l = 0; l < total_lods; ++l) {
				Ref<ArrayMesh> lod_m = _create_chunk_lod_mesh(cx, cz, l, chunk_aabb);
				if (lod_m.is_valid()) {
					tc.lod_meshes.push_back(lod_m);
				}
			}

			if (tc.lod_meshes.is_empty()) continue;

			tc.aabb = chunk_aabb;

			MeshInstance3D *mi = memnew(MeshInstance3D);
			mi->set_name(vformat("Chunk_%d_%d", cx, cz));
			mi->set_mesh(tc.lod_meshes[0]);
			mi->set_material_override(active_mat);
			mi->set_cast_shadows_setting(cast_shadow);
			chunks_container->add_child(mi, false, INTERNAL_MODE_FRONT);

			tc.mesh_instance = mi;
			chunks.push_back(tc);
		}
	}

	_update_lod_materials();
}

void Terrain3D::_update_lod(const Vector3 &p_camera_pos) {
	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local_cam = xform.affine_inverse().xform(p_camera_pos);

	for (int i = 0; i < chunks.size(); ++i) {
		TerrainChunk &chunk = chunks.write[i];
		if (!chunk.mesh_instance) continue;

		Vector3 chunk_center = chunk.aabb.position + chunk.aabb.size * 0.5f;
		float dist = chunk_center.distance_to(local_cam);

		if (max_view_distance > 0.0f && dist > max_view_distance) {
			chunk.mesh_instance->set_visible(false);
			continue;
		}

		int target_lod = 0;
		if (lod_enabled && lod_distance_step > 0.0f) {
			target_lod = CLAMP((int)(dist / lod_distance_step), 0, chunk.lod_meshes.size() - 1);
		}

		if (chunk.current_lod != target_lod) {
			chunk.current_lod = target_lod;
			chunk.mesh_instance->set_mesh(chunk.lod_meshes[target_lod]);
			if (debug_lod_colors) {
				int mat_idx = CLAMP(target_lod, 0, debug_lod_materials.size() - 1);
				chunk.mesh_instance->set_material_override(debug_lod_materials[mat_idx]);
			}
		}

		chunk.mesh_instance->set_visible(true);
	}
}

void Terrain3D::_update_materials() {
	if (debug_lod_colors) {
		_update_lod_materials();
		return;
	}
	_sync_material_uniforms();
	Ref<Material> active_mat = material.is_valid() ? material : Ref<Material>(default_terrain_material);
	for (int i = 0; i < chunks.size(); ++i) {
		if (chunks[i].mesh_instance) {
			chunks[i].mesh_instance->set_material_override(active_mat);
			chunks[i].mesh_instance->set_cast_shadows_setting(cast_shadow);
		}
	}
}

void Terrain3D::_update_lod_materials() {
	_sync_material_uniforms();
	Ref<Material> active_mat = material.is_valid() ? material : Ref<Material>(default_terrain_material);
	for (int i = 0; i < chunks.size(); ++i) {
		if (chunks[i].mesh_instance) {
			if (debug_lod_colors) {
				int mat_idx = CLAMP(chunks[i].current_lod, 0, debug_lod_materials.size() - 1);
				chunks[i].mesh_instance->set_material_override(debug_lod_materials[mat_idx]);
			} else {
				chunks[i].mesh_instance->set_material_override(active_mat);
			}
			chunks[i].mesh_instance->set_cast_shadows_setting(cast_shadow);
		}
	}
}

void Terrain3D::_sync_physics() {
#ifndef PHYSICS_3D_DISABLED
	if (!collision_body || !collision_shape_node || !collision_shape.is_valid()) {
		return;
	}

	collision_body->set_collision_layer(collision_layer);
	collision_body->set_collision_mask(collision_mask);

	if (!generate_collision || map_width < 2 || map_height < 2 || heights.is_empty()) {
		collision_shape_node->set_disabled(true);
		return;
	}

	collision_shape_node->set_disabled(false);
	collision_shape->set_map_width(map_width);
	collision_shape->set_map_depth(map_height);

	Vector<real_t> scaled_heights;
	scaled_heights.resize(heights.size());
	real_t *sh_ptr = scaled_heights.ptrw();
	for (int i = 0; i < heights.size(); ++i) {
		sh_ptr[i] = heights[i] * height_scale;
	}
	collision_shape->set_map_data(scaled_heights);

	Transform3D xform;
	xform.basis = Basis::from_scale(Vector3(cell_size, 1.0f, cell_size));
	if (!center_pivot) {
		xform.origin = Vector3((map_width - 1) * cell_size * 0.5f, 0.0f, (map_height - 1) * cell_size * 0.5f);
	} else {
		xform.origin = Vector3(0.0f, 0.0f, 0.0f);
	}
	collision_shape_node->set_transform(xform);
#endif // PHYSICS_3D_DISABLED
}

void Terrain3D::rebuild_terrain() {
	_load_heightmap_data();
	_build_chunk_meshes();
	_sync_physics();
	is_dirty = false;
}

void Terrain3D::bake_collision() {
	_sync_physics();
}

void Terrain3D::set_height_at_grid(int p_x, int p_z, float p_height) {
	if (p_x < 0 || p_x >= map_width || p_z < 0 || p_z >= map_height || heights.is_empty()) return;
	float norm_h = height_scale > 0.001f ? (p_height / height_scale) : 0.0f;
	heights.write[p_z * map_width + p_x] = norm_h;
	update_chunks_in_region(p_x, p_z, p_x, p_z);
	_sync_physics();
}

void Terrain3D::sculpt(const Vector3 &p_world_pos, float p_radius, float p_strength, BrushMode p_mode, float p_target_height) {
	if (map_width < 2 || map_height < 2 || heights.is_empty() || cell_size <= 0.0f) return;

	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local_pos = xform.affine_inverse().xform(p_world_pos);

	float origin_x = center_pivot ? -(map_width - 1) * cell_size * 0.5f : 0.0f;
	float origin_z = center_pivot ? -(map_height - 1) * cell_size * 0.5f : 0.0f;

	int center_gx = (int)Math::round((local_pos.x - origin_x) / cell_size);
	int center_gz = (int)Math::round((local_pos.z - origin_z) / cell_size);
	float grid_r = p_radius / cell_size;

	sculpt_grid(center_gx, center_gz, grid_r, p_strength, p_mode, p_target_height);
}

void Terrain3D::sculpt_grid(int p_gx, int p_gz, float p_grid_radius, float p_strength, BrushMode p_mode, float p_target_height) {
	if (map_width < 2 || map_height < 2 || heights.is_empty() || p_grid_radius <= 0.0f) return;

	int r_int = (int)Math::ceil(p_grid_radius);
	int min_gx = MAX(0, p_gx - r_int);
	int max_gx = MIN(map_width - 1, p_gx + r_int);
	int min_gz = MAX(0, p_gz - r_int);
	int max_gz = MIN(map_height - 1, p_gz + r_int);

	if (min_gx > max_gx || min_gz > max_gz) return;

	float r2 = p_grid_radius * p_grid_radius;

	for (int z = min_gz; z <= max_gz; ++z) {
		for (int x = min_gx; x <= max_gx; ++x) {
			float dx = (float)(x - p_gx);
			float dz = (float)(z - p_gz);
			float d2 = dx * dx + dz * dz;
			if (d2 > r2) continue;

			float d = Math::sqrt(d2);
			float t = d / p_grid_radius;
			float falloff = 1.0f - t;
			falloff = falloff * falloff * (3.0f - 2.0f * falloff);
			float delta = falloff * p_strength;

			int idx = z * map_width + x;
			float current_h = heights[idx] * height_scale;

			switch (p_mode) {
				case BRUSH_RAISE: {
					current_h += delta * 0.1f;
				} break;
				case BRUSH_LOWER: {
					current_h -= delta * 0.1f;
				} break;
				case BRUSH_SMOOTH: {
					int x_prev = MAX(0, x - 1);
					int x_next = MIN(map_width - 1, x + 1);
					int z_prev = MAX(0, z - 1);
					int z_next = MIN(map_height - 1, z + 1);

					float avg = (
						heights[z_prev * map_width + x_prev] + heights[z_prev * map_width + x] + heights[z_prev * map_width + x_next] +
						heights[z * map_width + x_prev] + heights[idx] + heights[z * map_width + x_next] +
						heights[z_next * map_width + x_prev] + heights[z_next * map_width + x] + heights[z_next * map_width + x_next]
					) / 9.0f * height_scale;

					float diff = avg - current_h;
					float step = SIGN(diff) * MIN(Math::abs(diff), delta * 0.1f);
					current_h += step;
				} break;
				case BRUSH_FLATTEN: {
					float diff = p_target_height - current_h;
					float step = SIGN(diff) * MIN(Math::abs(diff), delta * 0.1f);
					current_h += step;
				} break;
			}

			heights.write[idx] = height_scale > 0.001f ? (current_h / height_scale) : 0.0f;
		}
	}

	update_chunks_in_region(min_gx, min_gz, max_gx, max_gz);
}

float Terrain3D::sample_height(const Vector3 &p_world_pos) const {
	if (map_width < 2 || map_height < 2 || heights.is_empty() || cell_size <= 0.0f) {
		return 0.0f;
	}

	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local_pos = xform.affine_inverse().xform(p_world_pos);

	float origin_x = center_pivot ? -(map_width - 1) * cell_size * 0.5f : 0.0f;
	float origin_z = center_pivot ? -(map_height - 1) * cell_size * 0.5f : 0.0f;

	float gx = (local_pos.x - origin_x) / cell_size;
	float gz = (local_pos.z - origin_z) / cell_size;

	gx = CLAMP(gx, 0.0f, (float)(map_width - 1));
	gz = CLAMP(gz, 0.0f, (float)(map_height - 1));

	int x0 = (int)Math::floor(gx);
	int x1 = MIN(map_width - 1, x0 + 1);
	int z0 = (int)Math::floor(gz);
	int z1 = MIN(map_height - 1, z0 + 1);

	float fx = gx - x0;
	float fz = gz - z0;

	float h00 = _get_height_raw(x0, z0);
	float h10 = _get_height_raw(x1, z0);
	float h01 = _get_height_raw(x0, z1);
	float h11 = _get_height_raw(x1, z1);

	float h0 = h00 * (1.0f - fx) + h10 * fx;
	float h1 = h01 * (1.0f - fx) + h11 * fx;
	float local_h = (h0 * (1.0f - fz) + h1 * fz) * height_scale;

	Vector3 world_sample = xform.xform(Vector3(local_pos.x, local_h, local_pos.z));
	return world_sample.y;
}

Vector3 Terrain3D::get_normal_at(const Vector3 &p_world_pos) const {
	if (map_width < 2 || map_height < 2 || heights.is_empty() || cell_size <= 0.0f) {
		return Vector3(0, 1, 0);
	}

	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local_pos = xform.affine_inverse().xform(p_world_pos);

	float origin_x = center_pivot ? -(map_width - 1) * cell_size * 0.5f : 0.0f;
	float origin_z = center_pivot ? -(map_height - 1) * cell_size * 0.5f : 0.0f;

	int gx = (int)Math::round((local_pos.x - origin_x) / cell_size);
	int gz = (int)Math::round((local_pos.z - origin_z) / cell_size);

	gx = CLAMP(gx, 0, map_width - 1);
	gz = CLAMP(gz, 0, map_height - 1);

	Vector3 local_normal = _calc_normal(gx, gz);
	return xform.basis.xform(local_normal).normalized();
}

float Terrain3D::get_height_at_grid(int p_x, int p_z) const {
	return _get_height_raw(p_x, p_z) * height_scale;
}

Vector2 Terrain3D::get_terrain_size() const {
	return Vector2((map_width - 1) * cell_size, (map_height - 1) * cell_size);
}

AABB Terrain3D::get_total_aabb() const {
	if (chunks.is_empty()) {
		return AABB();
	}
	AABB total = chunks[0].aabb;
	for (int i = 1; i < chunks.size(); ++i) {
		total.merge_with(chunks[i].aabb);
	}
	return total;
}

TypedArray<AABB> Terrain3D::get_chunk_aabbs() const {
	TypedArray<AABB> res;
	for (int i = 0; i < chunks.size(); ++i) {
		res.push_back(chunks[i].aabb);
	}
	return res;
}

// Property Setters / Getters
void Terrain3D::set_heightmap_file(const String &p_path) {
	if (heightmap_file != p_path) {
		heightmap_file = p_path;
		rebuild_terrain();
	}
}

String Terrain3D::get_heightmap_file() const {
	return heightmap_file;
}

void Terrain3D::set_heightmap_texture(const Ref<Texture2D> &p_texture) {
	if (heightmap_texture != p_texture) {
		heightmap_texture = p_texture;
		rebuild_terrain();
	}
}

Ref<Texture2D> Terrain3D::get_heightmap_texture() const {
	return heightmap_texture;
}

void Terrain3D::set_smoothing_cycles(int p_cycles) {
	int c = CLAMP(p_cycles, 0, 8);
	if (smoothing_cycles != c) {
		smoothing_cycles = c;
		rebuild_terrain();
	}
}

int Terrain3D::get_smoothing_cycles() const {
	return smoothing_cycles;
}

void Terrain3D::set_cell_size(float p_size) {
	if (p_size > 0.001f && cell_size != p_size) {
		cell_size = p_size;
		rebuild_terrain();
	}
}

float Terrain3D::get_cell_size() const {
	return cell_size;
}

void Terrain3D::set_height_scale(float p_scale) {
	if (height_scale != p_scale) {
		height_scale = p_scale;
		rebuild_terrain();
	}
}

float Terrain3D::get_height_scale() const {
	return height_scale;
}

void Terrain3D::set_chunk_size(int p_size) {
	if (p_size >= 4 && chunk_size != p_size) {
		chunk_size = p_size;
		rebuild_terrain();
	}
}

int Terrain3D::get_chunk_size() const {
	return chunk_size;
}

void Terrain3D::set_center_pivot(bool p_center) {
	if (center_pivot != p_center) {
		center_pivot = p_center;
		rebuild_terrain();
	}
}

bool Terrain3D::is_center_pivot() const {
	return center_pivot;
}

void Terrain3D::set_resample_filter(ResampleFilter p_filter) {
	if (resample_filter != p_filter) {
		resample_filter = p_filter;
		rebuild_terrain();
	}
}

Terrain3D::ResampleFilter Terrain3D::get_resample_filter() const {
	return resample_filter;
}

// LOD Settings
void Terrain3D::set_lod_enabled(bool p_enabled) {
	lod_enabled = p_enabled;
}

bool Terrain3D::is_lod_enabled() const {
	return lod_enabled;
}

void Terrain3D::set_lod_count(int p_count) {
	int c = CLAMP(p_count, 1, 6);
	if (lod_count != c) {
		lod_count = c;
		rebuild_terrain();
	}
}

int Terrain3D::get_lod_count() const {
	return lod_count;
}

void Terrain3D::set_lod_distance_step(float p_step) {
	lod_distance_step = MAX(10.0f, p_step);
}

float Terrain3D::get_lod_distance_step() const {
	return lod_distance_step;
}

void Terrain3D::set_max_view_distance(float p_dist) {
	max_view_distance = MAX(0.0f, p_dist);
}

float Terrain3D::get_max_view_distance() const {
	return max_view_distance;
}

void Terrain3D::set_skirt_height(float p_height) {
	if (skirt_height != p_height) {
		skirt_height = MAX(0.0f, p_height);
		rebuild_terrain();
	}
}

float Terrain3D::get_skirt_height() const {
	return skirt_height;
}

void Terrain3D::set_debug_lod_colors(bool p_debug) {
	if (debug_lod_colors != p_debug) {
		debug_lod_colors = p_debug;
		_update_lod_materials();
	}
}

bool Terrain3D::is_debug_lod_colors() const {
	return debug_lod_colors;
}

// Multi-Layer Material & Splatmap Setters / Getters
void Terrain3D::set_auto_material_enabled(bool p_enabled) {
	if (auto_material_enabled != p_enabled) {
		auto_material_enabled = p_enabled;
		_update_materials();
	}
}

bool Terrain3D::is_auto_material_enabled() const {
	return auto_material_enabled;
}

void Terrain3D::set_splatmap_texture(const Ref<Texture2D> &p_texture) {
	if (splatmap_texture != p_texture) {
		splatmap_texture = p_texture;
		_update_materials();
	}
}

Ref<Texture2D> Terrain3D::get_splatmap_texture() const {
	return splatmap_texture;
}

// Layer 0: Flat / Grass
void Terrain3D::set_layer_0_color(const Color &p_color) {
	layer_0_color = p_color;
	_update_materials();
}

Color Terrain3D::get_layer_0_color() const {
	return layer_0_color;
}

void Terrain3D::set_layer_0_albedo(const Ref<Texture2D> &p_texture) {
	layer_0_albedo = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_0_albedo() const {
	return layer_0_albedo;
}

void Terrain3D::set_layer_0_normal(const Ref<Texture2D> &p_texture) {
	layer_0_normal = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_0_normal() const {
	return layer_0_normal;
}

void Terrain3D::set_layer_0_roughness(float p_roughness) {
	layer_0_roughness = CLAMP(p_roughness, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_layer_0_roughness() const {
	return layer_0_roughness;
}

void Terrain3D::set_layer_0_uv_scale(float p_scale) {
	layer_0_uv_scale = MAX(0.1f, p_scale);
	_update_materials();
}

float Terrain3D::get_layer_0_uv_scale() const {
	return layer_0_uv_scale;
}

// Layer 1: Cliffs / Rock
void Terrain3D::set_layer_1_color(const Color &p_color) {
	layer_1_color = p_color;
	_update_materials();
}

Color Terrain3D::get_layer_1_color() const {
	return layer_1_color;
}

void Terrain3D::set_layer_1_albedo(const Ref<Texture2D> &p_texture) {
	layer_1_albedo = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_1_albedo() const {
	return layer_1_albedo;
}

void Terrain3D::set_layer_1_normal(const Ref<Texture2D> &p_texture) {
	layer_1_normal = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_1_normal() const {
	return layer_1_normal;
}

void Terrain3D::set_layer_1_roughness(float p_roughness) {
	layer_1_roughness = CLAMP(p_roughness, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_layer_1_roughness() const {
	return layer_1_roughness;
}

void Terrain3D::set_layer_1_uv_scale(float p_scale) {
	layer_1_uv_scale = MAX(0.1f, p_scale);
	_update_materials();
}

float Terrain3D::get_layer_1_uv_scale() const {
	return layer_1_uv_scale;
}

// Layer 2: Snow / Peak
void Terrain3D::set_layer_2_color(const Color &p_color) {
	layer_2_color = p_color;
	_update_materials();
}

Color Terrain3D::get_layer_2_color() const {
	return layer_2_color;
}

void Terrain3D::set_layer_2_albedo(const Ref<Texture2D> &p_texture) {
	layer_2_albedo = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_2_albedo() const {
	return layer_2_albedo;
}

void Terrain3D::set_layer_2_normal(const Ref<Texture2D> &p_texture) {
	layer_2_normal = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_2_normal() const {
	return layer_2_normal;
}

void Terrain3D::set_layer_2_roughness(float p_roughness) {
	layer_2_roughness = CLAMP(p_roughness, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_layer_2_roughness() const {
	return layer_2_roughness;
}

void Terrain3D::set_layer_2_uv_scale(float p_scale) {
	layer_2_uv_scale = MAX(0.1f, p_scale);
	_update_materials();
}

float Terrain3D::get_layer_2_uv_scale() const {
	return layer_2_uv_scale;
}

// Layer 3: Sand / Basin
void Terrain3D::set_layer_3_color(const Color &p_color) {
	layer_3_color = p_color;
	_update_materials();
}

Color Terrain3D::get_layer_3_color() const {
	return layer_3_color;
}

void Terrain3D::set_layer_3_albedo(const Ref<Texture2D> &p_texture) {
	layer_3_albedo = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_3_albedo() const {
	return layer_3_albedo;
}

void Terrain3D::set_layer_3_normal(const Ref<Texture2D> &p_texture) {
	layer_3_normal = p_texture;
	_update_materials();
}

Ref<Texture2D> Terrain3D::get_layer_3_normal() const {
	return layer_3_normal;
}

void Terrain3D::set_layer_3_roughness(float p_roughness) {
	layer_3_roughness = CLAMP(p_roughness, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_layer_3_roughness() const {
	return layer_3_roughness;
}

void Terrain3D::set_layer_3_uv_scale(float p_scale) {
	layer_3_uv_scale = MAX(0.1f, p_scale);
	_update_materials();
}

float Terrain3D::get_layer_3_uv_scale() const {
	return layer_3_uv_scale;
}

// Auto Blending Controls
void Terrain3D::set_slope_threshold(float p_val) {
	slope_threshold = CLAMP(p_val, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_slope_threshold() const {
	return slope_threshold;
}

void Terrain3D::set_slope_sharpness(float p_val) {
	slope_sharpness = CLAMP(p_val, 1.0f, 16.0f);
	_update_materials();
}

float Terrain3D::get_slope_sharpness() const {
	return slope_sharpness;
}

void Terrain3D::set_snow_altitude(float p_val) {
	snow_altitude = p_val;
	_update_materials();
}

float Terrain3D::get_snow_altitude() const {
	return snow_altitude;
}

void Terrain3D::set_snow_falloff(float p_val) {
	snow_falloff = MAX(1.0f, p_val);
	_update_materials();
}

float Terrain3D::get_snow_falloff() const {
	return snow_falloff;
}

void Terrain3D::set_sand_altitude(float p_val) {
	sand_altitude = p_val;
	_update_materials();
}

float Terrain3D::get_sand_altitude() const {
	return sand_altitude;
}

void Terrain3D::set_sand_falloff(float p_val) {
	sand_falloff = MAX(1.0f, p_val);
	_update_materials();
}

float Terrain3D::get_sand_falloff() const {
	return sand_falloff;
}

void Terrain3D::set_macro_variation(float p_val) {
	macro_variation = CLAMP(p_val, 0.0f, 1.0f);
	_update_materials();
}

float Terrain3D::get_macro_variation() const {
	return macro_variation;
}

void Terrain3D::set_triplanar_cliffs(bool p_enabled) {
	triplanar_cliffs = p_enabled;
	_update_materials();
}

bool Terrain3D::is_triplanar_cliffs() const {
	return triplanar_cliffs;
}

void Terrain3D::set_material(const Ref<Material> &p_material) {
	material = p_material;
	_update_materials();
}

Ref<Material> Terrain3D::get_material() const {
	return material;
}

void Terrain3D::set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_shadow_casting_setting) {
	cast_shadow = p_shadow_casting_setting;
	_update_materials();
}

GeometryInstance3D::ShadowCastingSetting Terrain3D::get_cast_shadow() const {
	return cast_shadow;
}

void Terrain3D::set_generate_collision(bool p_enabled) {
	generate_collision = p_enabled;
	_sync_physics();
}

bool Terrain3D::is_generate_collision() const {
	return generate_collision;
}

void Terrain3D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	_sync_physics();
}

uint32_t Terrain3D::get_collision_layer() const {
	return collision_layer;
}

void Terrain3D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	_sync_physics();
}

uint32_t Terrain3D::get_collision_mask() const {
	return collision_mask;
}

void Terrain3D::set_show_chunk_bounds(bool p_show) {
	show_chunk_bounds = p_show;
}

bool Terrain3D::is_show_chunk_bounds() const {
	return show_chunk_bounds;
}

void Terrain3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_heightmap_file", "path"), &Terrain3D::set_heightmap_file);
	ClassDB::bind_method(D_METHOD("get_heightmap_file"), &Terrain3D::get_heightmap_file);

	ClassDB::bind_method(D_METHOD("set_heightmap_texture", "texture"), &Terrain3D::set_heightmap_texture);
	ClassDB::bind_method(D_METHOD("get_heightmap_texture"), &Terrain3D::get_heightmap_texture);

	ClassDB::bind_method(D_METHOD("set_smoothing_cycles", "cycles"), &Terrain3D::set_smoothing_cycles);
	ClassDB::bind_method(D_METHOD("get_smoothing_cycles"), &Terrain3D::get_smoothing_cycles);

	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &Terrain3D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &Terrain3D::get_cell_size);

	ClassDB::bind_method(D_METHOD("set_height_scale", "scale"), &Terrain3D::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &Terrain3D::get_height_scale);

	ClassDB::bind_method(D_METHOD("set_chunk_size", "size"), &Terrain3D::set_chunk_size);
	ClassDB::bind_method(D_METHOD("get_chunk_size"), &Terrain3D::get_chunk_size);

	ClassDB::bind_method(D_METHOD("set_center_pivot", "center"), &Terrain3D::set_center_pivot);
	ClassDB::bind_method(D_METHOD("is_center_pivot"), &Terrain3D::is_center_pivot);

	ClassDB::bind_method(D_METHOD("set_resample_filter", "filter"), &Terrain3D::set_resample_filter);
	ClassDB::bind_method(D_METHOD("get_resample_filter"), &Terrain3D::get_resample_filter);

	// LOD bindings
	ClassDB::bind_method(D_METHOD("set_lod_enabled", "enabled"), &Terrain3D::set_lod_enabled);
	ClassDB::bind_method(D_METHOD("is_lod_enabled"), &Terrain3D::is_lod_enabled);

	ClassDB::bind_method(D_METHOD("set_lod_count", "count"), &Terrain3D::set_lod_count);
	ClassDB::bind_method(D_METHOD("get_lod_count"), &Terrain3D::get_lod_count);

	ClassDB::bind_method(D_METHOD("set_lod_distance_step", "step"), &Terrain3D::set_lod_distance_step);
	ClassDB::bind_method(D_METHOD("get_lod_distance_step"), &Terrain3D::get_lod_distance_step);

	ClassDB::bind_method(D_METHOD("set_max_view_distance", "dist"), &Terrain3D::set_max_view_distance);
	ClassDB::bind_method(D_METHOD("get_max_view_distance"), &Terrain3D::get_max_view_distance);

	ClassDB::bind_method(D_METHOD("set_skirt_height", "height"), &Terrain3D::set_skirt_height);
	ClassDB::bind_method(D_METHOD("get_skirt_height"), &Terrain3D::get_skirt_height);

	ClassDB::bind_method(D_METHOD("set_debug_lod_colors", "debug"), &Terrain3D::set_debug_lod_colors);
	ClassDB::bind_method(D_METHOD("is_debug_lod_colors"), &Terrain3D::is_debug_lod_colors);

	// Multi-Layer Material & Splatmap Bindings
	ClassDB::bind_method(D_METHOD("set_auto_material_enabled", "enabled"), &Terrain3D::set_auto_material_enabled);
	ClassDB::bind_method(D_METHOD("is_auto_material_enabled"), &Terrain3D::is_auto_material_enabled);

	ClassDB::bind_method(D_METHOD("set_splatmap_texture", "texture"), &Terrain3D::set_splatmap_texture);
	ClassDB::bind_method(D_METHOD("get_splatmap_texture"), &Terrain3D::get_splatmap_texture);

	// Layer 0
	ClassDB::bind_method(D_METHOD("set_layer_0_color", "color"), &Terrain3D::set_layer_0_color);
	ClassDB::bind_method(D_METHOD("get_layer_0_color"), &Terrain3D::get_layer_0_color);
	ClassDB::bind_method(D_METHOD("set_layer_0_albedo", "texture"), &Terrain3D::set_layer_0_albedo);
	ClassDB::bind_method(D_METHOD("get_layer_0_albedo"), &Terrain3D::get_layer_0_albedo);
	ClassDB::bind_method(D_METHOD("set_layer_0_normal", "texture"), &Terrain3D::set_layer_0_normal);
	ClassDB::bind_method(D_METHOD("get_layer_0_normal"), &Terrain3D::get_layer_0_normal);
	ClassDB::bind_method(D_METHOD("set_layer_0_roughness", "roughness"), &Terrain3D::set_layer_0_roughness);
	ClassDB::bind_method(D_METHOD("get_layer_0_roughness"), &Terrain3D::get_layer_0_roughness);
	ClassDB::bind_method(D_METHOD("set_layer_0_uv_scale", "scale"), &Terrain3D::set_layer_0_uv_scale);
	ClassDB::bind_method(D_METHOD("get_layer_0_uv_scale"), &Terrain3D::get_layer_0_uv_scale);

	// Layer 1
	ClassDB::bind_method(D_METHOD("set_layer_1_color", "color"), &Terrain3D::set_layer_1_color);
	ClassDB::bind_method(D_METHOD("get_layer_1_color"), &Terrain3D::get_layer_1_color);
	ClassDB::bind_method(D_METHOD("set_layer_1_albedo", "texture"), &Terrain3D::set_layer_1_albedo);
	ClassDB::bind_method(D_METHOD("get_layer_1_albedo"), &Terrain3D::get_layer_1_albedo);
	ClassDB::bind_method(D_METHOD("set_layer_1_normal", "texture"), &Terrain3D::set_layer_1_normal);
	ClassDB::bind_method(D_METHOD("get_layer_1_normal"), &Terrain3D::get_layer_1_normal);
	ClassDB::bind_method(D_METHOD("set_layer_1_roughness", "roughness"), &Terrain3D::set_layer_1_roughness);
	ClassDB::bind_method(D_METHOD("get_layer_1_roughness"), &Terrain3D::get_layer_1_roughness);
	ClassDB::bind_method(D_METHOD("set_layer_1_uv_scale", "scale"), &Terrain3D::set_layer_1_uv_scale);
	ClassDB::bind_method(D_METHOD("get_layer_1_uv_scale"), &Terrain3D::get_layer_1_uv_scale);

	// Layer 2
	ClassDB::bind_method(D_METHOD("set_layer_2_color", "color"), &Terrain3D::set_layer_2_color);
	ClassDB::bind_method(D_METHOD("get_layer_2_color"), &Terrain3D::get_layer_2_color);
	ClassDB::bind_method(D_METHOD("set_layer_2_albedo", "texture"), &Terrain3D::set_layer_2_albedo);
	ClassDB::bind_method(D_METHOD("get_layer_2_albedo"), &Terrain3D::get_layer_2_albedo);
	ClassDB::bind_method(D_METHOD("set_layer_2_normal", "texture"), &Terrain3D::set_layer_2_normal);
	ClassDB::bind_method(D_METHOD("get_layer_2_normal"), &Terrain3D::get_layer_2_normal);
	ClassDB::bind_method(D_METHOD("set_layer_2_roughness", "roughness"), &Terrain3D::set_layer_2_roughness);
	ClassDB::bind_method(D_METHOD("get_layer_2_roughness"), &Terrain3D::get_layer_2_roughness);
	ClassDB::bind_method(D_METHOD("set_layer_2_uv_scale", "scale"), &Terrain3D::set_layer_2_uv_scale);
	ClassDB::bind_method(D_METHOD("get_layer_2_uv_scale"), &Terrain3D::get_layer_2_uv_scale);

	// Layer 3
	ClassDB::bind_method(D_METHOD("set_layer_3_color", "color"), &Terrain3D::set_layer_3_color);
	ClassDB::bind_method(D_METHOD("get_layer_3_color"), &Terrain3D::get_layer_3_color);
	ClassDB::bind_method(D_METHOD("set_layer_3_albedo", "texture"), &Terrain3D::set_layer_3_albedo);
	ClassDB::bind_method(D_METHOD("get_layer_3_albedo"), &Terrain3D::get_layer_3_albedo);
	ClassDB::bind_method(D_METHOD("set_layer_3_normal", "texture"), &Terrain3D::set_layer_3_normal);
	ClassDB::bind_method(D_METHOD("get_layer_3_normal"), &Terrain3D::get_layer_3_normal);
	ClassDB::bind_method(D_METHOD("set_layer_3_roughness", "roughness"), &Terrain3D::set_layer_3_roughness);
	ClassDB::bind_method(D_METHOD("get_layer_3_roughness"), &Terrain3D::get_layer_3_roughness);
	ClassDB::bind_method(D_METHOD("set_layer_3_uv_scale", "scale"), &Terrain3D::set_layer_3_uv_scale);
	ClassDB::bind_method(D_METHOD("get_layer_3_uv_scale"), &Terrain3D::get_layer_3_uv_scale);

	// Auto-Blend Controls
	ClassDB::bind_method(D_METHOD("set_slope_threshold", "val"), &Terrain3D::set_slope_threshold);
	ClassDB::bind_method(D_METHOD("get_slope_threshold"), &Terrain3D::get_slope_threshold);
	ClassDB::bind_method(D_METHOD("set_slope_sharpness", "val"), &Terrain3D::set_slope_sharpness);
	ClassDB::bind_method(D_METHOD("get_slope_sharpness"), &Terrain3D::get_slope_sharpness);
	ClassDB::bind_method(D_METHOD("set_snow_altitude", "val"), &Terrain3D::set_snow_altitude);
	ClassDB::bind_method(D_METHOD("get_snow_altitude"), &Terrain3D::get_snow_altitude);
	ClassDB::bind_method(D_METHOD("set_snow_falloff", "val"), &Terrain3D::set_snow_falloff);
	ClassDB::bind_method(D_METHOD("get_snow_falloff"), &Terrain3D::get_snow_falloff);
	ClassDB::bind_method(D_METHOD("set_sand_altitude", "val"), &Terrain3D::set_sand_altitude);
	ClassDB::bind_method(D_METHOD("get_sand_altitude"), &Terrain3D::get_sand_altitude);
	ClassDB::bind_method(D_METHOD("set_sand_falloff", "val"), &Terrain3D::set_sand_falloff);
	ClassDB::bind_method(D_METHOD("get_sand_falloff"), &Terrain3D::get_sand_falloff);
	ClassDB::bind_method(D_METHOD("set_macro_variation", "val"), &Terrain3D::set_macro_variation);
	ClassDB::bind_method(D_METHOD("get_macro_variation"), &Terrain3D::get_macro_variation);
	ClassDB::bind_method(D_METHOD("set_triplanar_cliffs", "enabled"), &Terrain3D::set_triplanar_cliffs);
	ClassDB::bind_method(D_METHOD("is_triplanar_cliffs"), &Terrain3D::is_triplanar_cliffs);

	// Sculpting Methods
	ClassDB::bind_method(D_METHOD("sculpt", "world_pos", "radius", "strength", "mode", "target_height"), &Terrain3D::sculpt, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("sculpt_grid", "gx", "gz", "grid_radius", "strength", "mode", "target_height"), &Terrain3D::sculpt_grid, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("set_height_at_grid", "x", "z", "height"), &Terrain3D::set_height_at_grid);
	ClassDB::bind_method(D_METHOD("update_chunks_in_region", "min_gx", "min_gz", "max_gx", "max_gz"), &Terrain3D::update_chunks_in_region);
	ClassDB::bind_method(D_METHOD("get_heights_raw"), &Terrain3D::get_heights_raw);
	ClassDB::bind_method(D_METHOD("set_heights_raw", "heights"), &Terrain3D::set_heights_raw);

	ClassDB::bind_method(D_METHOD("set_material", "material"), &Terrain3D::set_material);
	ClassDB::bind_method(D_METHOD("get_material"), &Terrain3D::get_material);

	ClassDB::bind_method(D_METHOD("set_cast_shadow", "shadow_casting_setting"), &Terrain3D::set_cast_shadow);
	ClassDB::bind_method(D_METHOD("get_cast_shadow"), &Terrain3D::get_cast_shadow);

	ClassDB::bind_method(D_METHOD("set_generate_collision", "enabled"), &Terrain3D::set_generate_collision);
	ClassDB::bind_method(D_METHOD("is_generate_collision"), &Terrain3D::is_generate_collision);

	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &Terrain3D::set_collision_layer);
	ClassDB::bind_method(D_METHOD("get_collision_layer"), &Terrain3D::get_collision_layer);

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &Terrain3D::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &Terrain3D::get_collision_mask);

	ClassDB::bind_method(D_METHOD("set_show_chunk_bounds", "show"), &Terrain3D::set_show_chunk_bounds);
	ClassDB::bind_method(D_METHOD("is_show_chunk_bounds"), &Terrain3D::is_show_chunk_bounds);

	ClassDB::bind_method(D_METHOD("rebuild_terrain"), &Terrain3D::rebuild_terrain);
	ClassDB::bind_method(D_METHOD("bake_collision"), &Terrain3D::bake_collision);

	ClassDB::bind_method(D_METHOD("sample_height", "world_pos"), &Terrain3D::sample_height);
	ClassDB::bind_method(D_METHOD("get_normal_at", "world_pos"), &Terrain3D::get_normal_at);
	ClassDB::bind_method(D_METHOD("get_height_at_grid", "x", "z"), &Terrain3D::get_height_at_grid);

	ClassDB::bind_method(D_METHOD("get_map_width"), &Terrain3D::get_map_width);
	ClassDB::bind_method(D_METHOD("get_map_height"), &Terrain3D::get_map_height);
	ClassDB::bind_method(D_METHOD("get_chunk_count"), &Terrain3D::get_chunk_count);
	ClassDB::bind_method(D_METHOD("get_terrain_size"), &Terrain3D::get_terrain_size);
	ClassDB::bind_method(D_METHOD("get_total_aabb"), &Terrain3D::get_total_aabb);
	ClassDB::bind_method(D_METHOD("get_chunk_aabbs"), &Terrain3D::get_chunk_aabbs);

	ADD_GROUP("Heightmap", "heightmap_");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "heightmap_file", PROPERTY_HINT_FILE, "*.png,*.exr,*.hdr,*.r16,*.raw,*.r8"), "set_heightmap_file", "get_heightmap_file");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "heightmap_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_heightmap_texture", "get_heightmap_texture");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "smoothing_cycles", PROPERTY_HINT_RANGE, "0,8,1"), "set_smoothing_cycles", "get_smoothing_cycles");

	ADD_GROUP("Dimensions & Mesh", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "0.01,100.0,0.01,or_greater"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "0.0,2000.0,0.1,or_greater"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "chunk_size", PROPERTY_HINT_RANGE, "16,256,16"), "set_chunk_size", "get_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "center_pivot"), "set_center_pivot", "is_center_pivot");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resample_filter", PROPERTY_HINT_ENUM, "Bilinear,Bicubic,Box"), "set_resample_filter", "get_resample_filter");

	ADD_GROUP("Level of Detail (LOD)", "lod_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lod_enabled"), "set_lod_enabled", "is_lod_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "lod_count", PROPERTY_HINT_RANGE, "1,6,1"), "set_lod_count", "get_lod_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod_distance_step", PROPERTY_HINT_RANGE, "10.0,1000.0,5.0,or_greater"), "set_lod_distance_step", "get_lod_distance_step");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_view_distance", PROPERTY_HINT_RANGE, "0.0,10000.0,50.0,or_greater"), "set_max_view_distance", "get_max_view_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "skirt_height", PROPERTY_HINT_RANGE, "0.0,100.0,0.5,or_greater"), "set_skirt_height", "get_skirt_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_lod_colors"), "set_debug_lod_colors", "is_debug_lod_colors");

	ADD_GROUP("Multi-Layer Material & Splatmap", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_material_enabled"), "set_auto_material_enabled", "is_auto_material_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "splatmap_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_splatmap_texture", "get_splatmap_texture");

	ADD_GROUP("Layer 0 (Flat / Grass)", "layer_0_");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "layer_0_color"), "set_layer_0_color", "get_layer_0_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_0_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_0_albedo", "get_layer_0_albedo");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_0_normal", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_0_normal", "get_layer_0_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_0_roughness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_layer_0_roughness", "get_layer_0_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_0_uv_scale", PROPERTY_HINT_RANGE, "0.1,200.0,0.5"), "set_layer_0_uv_scale", "get_layer_0_uv_scale");

	ADD_GROUP("Layer 1 (Cliffs / Rock)", "layer_1_");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "layer_1_color"), "set_layer_1_color", "get_layer_1_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_1_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_1_albedo", "get_layer_1_albedo");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_1_normal", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_1_normal", "get_layer_1_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_1_roughness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_layer_1_roughness", "get_layer_1_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_1_uv_scale", PROPERTY_HINT_RANGE, "0.1,200.0,0.5"), "set_layer_1_uv_scale", "get_layer_1_uv_scale");

	ADD_GROUP("Layer 2 (Snow / Peak)", "layer_2_");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "layer_2_color"), "set_layer_2_color", "get_layer_2_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_2_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_2_albedo", "get_layer_2_albedo");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_2_normal", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_2_normal", "get_layer_2_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_2_roughness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_layer_2_roughness", "get_layer_2_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_2_uv_scale", PROPERTY_HINT_RANGE, "0.1,200.0,0.5"), "set_layer_2_uv_scale", "get_layer_2_uv_scale");

	ADD_GROUP("Layer 3 (Sand / Basin)", "layer_3_");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "layer_3_color"), "set_layer_3_color", "get_layer_3_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_3_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_3_albedo", "get_layer_3_albedo");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layer_3_normal", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_layer_3_normal", "get_layer_3_normal");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_3_roughness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_layer_3_roughness", "get_layer_3_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "layer_3_uv_scale", PROPERTY_HINT_RANGE, "0.1,200.0,0.5"), "set_layer_3_uv_scale", "get_layer_3_uv_scale");

	ADD_GROUP("Auto-Material Rules", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_slope_threshold", "get_slope_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_sharpness", PROPERTY_HINT_RANGE, "1.0,16.0,0.5"), "set_slope_sharpness", "get_slope_sharpness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "snow_altitude", PROPERTY_HINT_RANGE, "-500.0,2000.0,1.0"), "set_snow_altitude", "get_snow_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "snow_falloff", PROPERTY_HINT_RANGE, "1.0,100.0,1.0"), "set_snow_falloff", "get_snow_falloff");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sand_altitude", PROPERTY_HINT_RANGE, "-500.0,500.0,1.0"), "set_sand_altitude", "get_sand_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sand_falloff", PROPERTY_HINT_RANGE, "1.0,50.0,1.0"), "set_sand_falloff", "get_sand_falloff");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "macro_variation", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_macro_variation", "get_macro_variation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "triplanar_cliffs"), "set_triplanar_cliffs", "is_triplanar_cliffs");

	ADD_GROUP("Material Override & Lighting", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cast_shadow", PROPERTY_HINT_ENUM, "Off,On,Double-Sided,Shadows Only"), "set_cast_shadow", "get_cast_shadow");

	ADD_GROUP("Physics & Collision", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "generate_collision"), "set_generate_collision", "is_generate_collision");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ADD_GROUP("Debug", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_chunk_bounds"), "set_show_chunk_bounds", "is_show_chunk_bounds");

	BIND_ENUM_CONSTANT(RESAMPLE_BILINEAR);
	BIND_ENUM_CONSTANT(RESAMPLE_BICUBIC);
	BIND_ENUM_CONSTANT(RESAMPLE_BOX);

	BIND_ENUM_CONSTANT(BRUSH_RAISE);
	BIND_ENUM_CONSTANT(BRUSH_LOWER);
	BIND_ENUM_CONSTANT(BRUSH_SMOOTH);
	BIND_ENUM_CONSTANT(BRUSH_FLATTEN);
}
