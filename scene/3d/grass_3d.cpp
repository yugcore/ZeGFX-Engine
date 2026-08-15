/**************************************************************************/
/*  grass_3d.cpp                                                          */
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

#include "grass_3d.h"

#include "core/config/engine.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"

static const char *DEFAULT_GRASS_SHADER_CODE =
		"shader_type spatial;\n"
		"render_mode cull_disabled, depth_draw_opaque;\n"
		"\n"
		"uniform vec4 color_bottom : source_color = vec4(0.12, 0.24, 0.08, 1.0);\n"
		"uniform vec4 color_top : source_color = vec4(0.46, 0.68, 0.18, 1.0);\n"
		"uniform vec4 translucency_color : source_color = vec4(0.38, 0.55, 0.18, 1.0);\n"
		"uniform sampler2D texture_albedo : source_color, filter_linear_mipmap, hint_default_white;\n"
		"uniform sampler2D texture_normal : hint_normal, filter_linear_mipmap;\n"
		"uniform float alpha_scissor_threshold : hint_range(0.0, 1.0) = 0.5;\n"
		"uniform float roughness : hint_range(0.0, 1.0) = 0.65;\n"
		"uniform float specular : hint_range(0.0, 1.0) = 0.25;\n"
		"\n"
		"// Wind & Dynamics\n"
		"uniform vec2 wind_direction = vec2(1.0, 0.35);\n"
		"uniform float wind_strength : hint_range(0.0, 2.0) = 0.35;\n"
		"uniform float wind_speed : hint_range(0.0, 10.0) = 1.8;\n"
		"uniform float gust_frequency : hint_range(0.0, 2.0) = 0.25;\n"
		"uniform float flutter_strength : hint_range(0.0, 0.5) = 0.08;\n"
		"\n"
		"// Player Interaction Trample\n"
		"uniform vec3 player_position = vec3(0.0, -10000.0, 0.0);\n"
		"uniform float player_radius : hint_range(0.1, 10.0) = 1.8;\n"
		"uniform float trample_strength : hint_range(0.0, 2.0) = 0.8;\n"
		"\n"
		"// Distance Anti-Aliasing & Fade\n"
		"uniform float cull_distance = 140.0;\n"
		"uniform float fade_range = 25.0;\n"
		"uniform bool enable_distance_aa = true;\n"
		"\n"
		"varying float v_camera_dist;\n"
		"\n"
		"void vertex() {\n"
		"    float height_weight = COLOR.r;\n"
		"    vec3 world_vert = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;\n"
		"    vec3 world_root = (MODEL_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;\n"
		"\n"
		"    v_camera_dist = length(world_vert - INV_VIEW_MATRIX[3].xyz);\n"
		"\n"
		"    // 1. Wind Displacement\n"
		"    vec2 wind_dir_norm = normalize(wind_direction + vec2(1e-5, 1e-5));\n"
		"    float time = TIME * wind_speed;\n"
		"    float gust = sin(time * 0.8 + (world_root.x * wind_dir_norm.x + world_root.z * wind_dir_norm.y) * gust_frequency);\n"
		"    gust = gust * 0.5 + 0.5;\n"
		"    float flutter = sin(time * 4.5 + world_root.x * 2.3 + world_root.z * 1.7) * cos(time * 3.1 + world_vert.y * 5.0);\n"
		"    float total_wind = (gust * wind_strength + flutter * flutter_strength) * height_weight;\n"
		"    vec3 wind_disp = vec3(wind_dir_norm.x, -0.2, wind_dir_norm.y) * total_wind;\n"
		"\n"
		"    // 2. Player Trample Displacement\n"
		"    vec3 to_player = world_vert - player_position;\n"
		"    to_player.y *= 0.5;\n"
		"    float dist_sq = dot(to_player, to_player);\n"
		"    float rad_sq = player_radius * player_radius;\n"
		"    if (dist_sq < rad_sq && dist_sq > 1e-4) {\n"
		"        float dist = sqrt(dist_sq);\n"
		"        float push_factor = (1.0 - (dist / player_radius)) * trample_strength * height_weight;\n"
		"        vec3 push_dir = normalize(vec3(to_player.x, -abs(to_player.y) - 0.4, to_player.z));\n"
		"        wind_disp += push_dir * push_factor * 1.5;\n"
		"    }\n"
		"\n"
		"    vec3 local_disp = (inverse(MODEL_MATRIX) * vec4(wind_disp, 0.0)).xyz;\n"
		"    VERTEX += local_disp;\n"
		"\n"
		"    // 3. Normal Softening at distance (Geometric Normal AA)\n"
		"    if (enable_distance_aa) {\n"
		"        float dist_norm = clamp(v_camera_dist / max(1.0, cull_distance), 0.0, 1.0);\n"
		"        vec3 up_dir = vec3(0.0, 1.0, 0.0);\n"
		"        NORMAL = normalize(mix(NORMAL, up_dir, smoothstep(0.12, 0.80, dist_norm) * 0.85));\n"
		"    }\n"
		"}\n"
		"\n"
		"void fragment() {\n"
		"    // 1. Dithered Distance Dissolve / Fade\n"
		"    if (fade_range > 0.01) {\n"
		"        float fade_start = cull_distance - fade_range;\n"
		"        if (v_camera_dist > fade_start) {\n"
		"            float fade_t = clamp((cull_distance - v_camera_dist) / max(0.01, fade_range), 0.0, 1.0);\n"
		"            const mat4 bayer = mat4(\n"
		"                vec4( 0.0/16.0, 12.0/16.0,  3.0/16.0, 15.0/16.0),\n"
		"                vec4( 8.0/16.0,  4.0/16.0, 11.0/16.0,  7.0/16.0),\n"
		"                vec4( 2.0/16.0, 14.0/16.0,  1.0/16.0, 13.0/16.0),\n"
		"                vec4(10.0/16.0,  6.0/16.0,  9.0/16.0,  5.0/16.0)\n"
		"            );\n"
		"            ivec2 sp = ivec2(FRAGCOORD.xy);\n"
		"            float dither_val = bayer[sp.x % 4][sp.y % 4];\n"
		"            if (fade_t < dither_val) {\n"
		"                discard;\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"\n"
		"    vec4 tex_color = texture(texture_albedo, UV);\n"
		"    float height_t = clamp(COLOR.r, 0.0, 1.0);\n"
		"    vec3 gradient_color = mix(color_bottom.rgb, color_top.rgb, height_t);\n"
		"    float ao = mix(0.35, 1.0, clamp(COLOR.g, 0.0, 1.0));\n"
		"\n"
		"    ALBEDO = tex_color.rgb * gradient_color * ao;\n"
		"    ALPHA = tex_color.a;\n"
		"    ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;\n"
		"\n"
		"    // 2. Specular & Roughness Distance Anti-Aliasing (eliminates white shimmering noise)\n"
		"    float dist_norm = clamp(v_camera_dist / max(1.0, cull_distance), 0.0, 1.0);\n"
		"    if (enable_distance_aa) {\n"
		"        ROUGHNESS = mix(roughness, 1.0, smoothstep(0.08, 0.60, dist_norm));\n"
		"        SPECULAR = mix(specular, 0.0, smoothstep(0.12, 0.70, dist_norm));\n"
		"    } else {\n"
		"        ROUGHNESS = roughness;\n"
		"        SPECULAR = specular;\n"
		"    }\n"
		"\n"
		"    BACKLIGHT = translucency_color.rgb * height_t * (1.0 - dist_norm * 0.45);\n"
		"\n"
		"    vec3 norm_sample = texture(texture_normal, UV).rgb;\n"
		"    if (length(norm_sample) > 0.1) {\n"
		"        NORMAL_MAP = norm_sample;\n"
		"    }\n"
		"}\n";

void Grass3D::_ensure_default_material() {
	if (default_grass_shader.is_null()) {
		default_grass_shader.instantiate();
		default_grass_shader->set_code(DEFAULT_GRASS_SHADER_CODE);
	}

	if (default_grass_material.is_null()) {
		default_grass_material.instantiate();
		default_grass_material->set_shader(default_grass_shader);
	}

	_sync_material_uniforms();
}

void Grass3D::_sync_material_uniforms() {
	if (default_grass_material.is_null()) {
		return;
	}

	default_grass_material->set_shader_parameter("color_bottom", color_bottom);
	default_grass_material->set_shader_parameter("color_top", color_top);
	default_grass_material->set_shader_parameter("translucency_color", translucency_color);
	default_grass_material->set_shader_parameter("texture_albedo", texture_albedo);
	default_grass_material->set_shader_parameter("texture_normal", texture_normal);
	default_grass_material->set_shader_parameter("alpha_scissor_threshold", alpha_scissor_threshold);
	default_grass_material->set_shader_parameter("roughness", roughness);
	default_grass_material->set_shader_parameter("specular", specular);

	default_grass_material->set_shader_parameter("wind_direction", wind_direction);
	default_grass_material->set_shader_parameter("wind_strength", wind_strength);
	default_grass_material->set_shader_parameter("wind_speed", wind_speed);
	default_grass_material->set_shader_parameter("gust_frequency", gust_frequency);
	default_grass_material->set_shader_parameter("flutter_strength", flutter_strength);

	default_grass_material->set_shader_parameter("player_position", player_position);
	default_grass_material->set_shader_parameter("player_radius", player_radius);
	default_grass_material->set_shader_parameter("trample_strength", trample_strength);

	default_grass_material->set_shader_parameter("cull_distance", cull_distance);
	default_grass_material->set_shader_parameter("fade_range", fade_range);
	default_grass_material->set_shader_parameter("enable_distance_aa", distance_anti_aliasing);
}

void Grass3D::_build_procedural_mesh() {
	if (mesh_type == MESH_CUSTOM) {
		generated_mesh = custom_mesh;
		is_mesh_dirty = false;
		return;
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedFloat32Array tangents;
	PackedVector2Array uvs;
	PackedColorArray colors;
	PackedInt32Array indices;

	float hw = blade_width * 0.5f;
	float h = blade_height;

	if (mesh_type == MESH_CROSS_QUADS || mesh_type == MESH_TRI_STAR) {
		int quad_count = (mesh_type == MESH_TRI_STAR) ? 3 : 2;
		float angle_step = (float)Math::PI / (float)quad_count;

		for (int q = 0; q < quad_count; q++) {
			float angle = (float)q * angle_step;
			float cos_a = Math::cos(angle);
			float sin_a = Math::sin(angle);

			Vector3 right = Vector3(cos_a, 0.0f, sin_a) * hw;
			Vector3 norm = Vector3(-sin_a, 0.0f, cos_a);

			int base_idx = vertices.size();

			// 4 vertices per quad: BL, BR, TR, TL
			vertices.push_back(-right);
			normals.push_back(norm);
			tangents.push_back(cos_a);
			tangents.push_back(0.0f);
			tangents.push_back(sin_a);
			tangents.push_back(1.0f);
			uvs.push_back(Vector2(0.0f, 1.0f));
			colors.push_back(Color(0.0f, 0.35f, 0.0f, 1.0f)); // Root: height=0.0, AO=0.35

			vertices.push_back(right);
			normals.push_back(norm);
			tangents.push_back(cos_a);
			tangents.push_back(0.0f);
			tangents.push_back(sin_a);
			tangents.push_back(1.0f);
			uvs.push_back(Vector2(1.0f, 1.0f));
			colors.push_back(Color(0.0f, 0.35f, 0.0f, 1.0f));

			vertices.push_back(right + Vector3(0.0f, h, 0.0f));
			normals.push_back(norm);
			tangents.push_back(cos_a);
			tangents.push_back(0.0f);
			tangents.push_back(sin_a);
			tangents.push_back(1.0f);
			uvs.push_back(Vector2(1.0f, 0.0f));
			colors.push_back(Color(1.0f, 1.0f, 0.0f, 1.0f)); // Tip: height=1.0, AO=1.0

			vertices.push_back(-right + Vector3(0.0f, h, 0.0f));
			normals.push_back(norm);
			tangents.push_back(cos_a);
			tangents.push_back(0.0f);
			tangents.push_back(sin_a);
			tangents.push_back(1.0f);
			uvs.push_back(Vector2(0.0f, 0.0f));
			colors.push_back(Color(1.0f, 1.0f, 0.0f, 1.0f));

			// Two triangles: (0, 1, 2) and (0, 2, 3)
			indices.push_back(base_idx + 0);
			indices.push_back(base_idx + 1);
			indices.push_back(base_idx + 2);

			indices.push_back(base_idx + 0);
			indices.push_back(base_idx + 2);
			indices.push_back(base_idx + 3);
		}
	} else if (mesh_type == MESH_CURVED_CLUMP) {
		// 5 curved blade clusters arranged in a circle
		int blade_count = 5;
		int segs = MAX(2, blade_segments);

		for (int b = 0; b < blade_count; b++) {
			float b_angle = (float)b * ((float)Math::TAU / (float)blade_count) + 0.2f;
			float b_radius = hw * 0.4f;
			Vector3 root_pos = Vector3(Math::cos(b_angle) * b_radius, 0.0f, Math::sin(b_angle) * b_radius);
			Vector3 curve_dir = Vector3(Math::cos(b_angle), 0.0f, Math::sin(b_angle));
			Vector3 blade_right = Vector3(-Math::sin(b_angle), 0.0f, Math::cos(b_angle)) * (hw * 0.5f);

			for (int s = 0; s <= segs; s++) {
				float t = (float)s / (float)segs;
				float cur_h = t * h;
				float curve_offset = t * t * blade_curviness * h;
				Vector3 center = root_pos + Vector3(0.0f, cur_h, 0.0f) + curve_dir * curve_offset;
				float width_mult = 1.0f - (t * 0.75f); // Tapering towards the tip

				float ao = Math::lerp(0.3f, 1.0f, t);

				// Left vertex
				vertices.push_back(center - blade_right * width_mult);
				normals.push_back(Vector3(0.0f, 1.0f, 0.0f).lerp(curve_dir, t).normalized());
				tangents.push_back(blade_right.x);
				tangents.push_back(blade_right.y);
				tangents.push_back(blade_right.z);
				tangents.push_back(1.0f);
				uvs.push_back(Vector2(0.0f, 1.0f - t));
				colors.push_back(Color(t, ao, 0.0f, 1.0f));

				// Right vertex
				vertices.push_back(center + blade_right * width_mult);
				normals.push_back(Vector3(0.0f, 1.0f, 0.0f).lerp(curve_dir, t).normalized());
				tangents.push_back(blade_right.x);
				tangents.push_back(blade_right.y);
				tangents.push_back(blade_right.z);
				tangents.push_back(1.0f);
				uvs.push_back(Vector2(1.0f, 1.0f - t));
				colors.push_back(Color(t, ao, 0.0f, 1.0f));
			}

			int base_idx = b * (segs + 1) * 2;
			for (int s = 0; s < segs; s++) {
				int row0 = base_idx + s * 2;
				int row1 = base_idx + (s + 1) * 2;

				indices.push_back(row0 + 0);
				indices.push_back(row0 + 1);
				indices.push_back(row1 + 1);

				indices.push_back(row0 + 0);
				indices.push_back(row1 + 1);
				indices.push_back(row1 + 0);
			}
		}
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	arrays[Mesh::ARRAY_TANGENT] = tangents;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_COLOR] = colors;
	arrays[Mesh::ARRAY_INDEX] = indices;

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	generated_mesh = mesh;
	is_mesh_dirty = false;
}

Ref<Mesh> Grass3D::get_effective_mesh() const {
	if (mesh_type == MESH_CUSTOM) {
		return custom_mesh;
	}
	return generated_mesh;
}

void Grass3D::_clear_chunks() {
	if (chunks_container) {
		for (int i = 0; i < chunks.size(); i++) {
			if (chunks[i].multimesh_instance) {
				chunks[i].multimesh_instance->queue_free();
			}
		}
		chunks_container->queue_free();
		chunks_container = nullptr;
	}
	chunks.clear();
}

void Grass3D::_find_terrain() {
	if (!terrain_path.is_empty()) {
		cached_terrain = Object::cast_to<Terrain3D>(get_node_or_null(terrain_path));
		if (cached_terrain) {
			return;
		}
	}

	// Auto-detect sibling or parent Terrain3D
	Node *parent = get_parent();
	if (parent) {
		cached_terrain = Object::cast_to<Terrain3D>(parent);
		if (cached_terrain) {
			terrain_path = get_path_to(cached_terrain);
			return;
		}

		for (int i = 0; i < parent->get_child_count(); i++) {
			Terrain3D *t = Object::cast_to<Terrain3D>(parent->get_child(i));
			if (t) {
				cached_terrain = t;
				terrain_path = get_path_to(t);
				return;
			}
		}
	}
}

void Grass3D::_load_density_mask_data() {
	if (density_mask.is_valid()) {
		density_mask_image = density_mask->get_image();
	} else {
		density_mask_image.unref();
	}
}

float Grass3D::_sample_density_mask(float p_u, float p_v) const {
	if (density_mask_image.is_null() || density_mask_image->is_empty()) {
		return 1.0f;
	}

	int w = density_mask_image->get_width();
	int h = density_mask_image->get_height();
	int px = CLAMP((int)(p_u * (float)w), 0, w - 1);
	int py = CLAMP((int)(p_v * (float)h), 0, h - 1);

	Color c = density_mask_image->get_pixel(px, py);
	switch (density_mask_channel) {
		case CHANNEL_RED:
			return c.r;
		case CHANNEL_GREEN:
			return c.g;
		case CHANNEL_BLUE:
			return c.b;
		case CHANNEL_ALPHA:
			return c.a;
		default:
			return 1.0f;
	}
}

void Grass3D::_rebuild_all_chunks() {
	if (!is_inside_tree()) {
		is_dirty = true;
		return;
	}

	_clear_chunks();
	_ensure_default_material();

	if (is_mesh_dirty || generated_mesh.is_null()) {
		_build_procedural_mesh();
	}

	Ref<Mesh> mesh = get_effective_mesh();
	if (mesh.is_null()) {
		return;
	}

	_find_terrain();
	_load_density_mask_data();

	if (anchor_to_terrain && cached_terrain && is_inside_tree()) {
		set_global_transform(cached_terrain->get_global_transform());
	}

	chunks_container = memnew(Node3D);
	chunks_container->set_name("GrassChunksContainer");
	add_child(chunks_container);

	AABB bounds;
	if (cached_terrain) {
		bounds = cached_terrain->get_total_aabb();
	} else {
		bounds = AABB(Vector3(-100.0f, 0.0f, -100.0f), Vector3(200.0f, 50.0f, 200.0f));
	}

	float c_size = MAX(8.0f, chunk_size);
	int chunks_x = (int)Math::ceil(bounds.size.x / c_size);
	int chunks_z = (int)Math::ceil(bounds.size.z / c_size);

	if (chunks_x <= 0 || chunks_z <= 0) {
		return;
	}

	for (int cz = 0; cz < chunks_z; cz++) {
		for (int cx = 0; cx < chunks_x; cx++) {
			AABB chunk_aabb(
					Vector3(bounds.position.x + (float)cx * c_size, bounds.position.y - 10.0f, bounds.position.z + (float)cz * c_size),
					Vector3(c_size, bounds.size.y + 20.0f, c_size));

			GrassChunk chunk;
			chunk.chunk_x = cx;
			chunk.chunk_z = cz;
			chunk.aabb = chunk_aabb;

			_rebuild_single_chunk(chunk, chunk_aabb);

			if (chunk.multimesh_instance && chunk.instance_count > 0) {
				chunks.push_back(chunk);
			}
		}
	}

	is_dirty = false;
}

void Grass3D::_rebuild_single_chunk(GrassChunk &p_chunk, const AABB &p_chunk_bounds) {
	Ref<Mesh> mesh = get_effective_mesh();
	if (mesh.is_null()) {
		return;
	}

	float area = p_chunk_bounds.size.x * p_chunk_bounds.size.z;
	int target_count = CLAMP((int)(area * density), 0, max_instances_per_chunk);
	if (target_count <= 0) {
		return;
	}

	// Deterministic PRNG seeded per chunk
	uint32_t seed = (uint32_t)(random_seed + p_chunk.chunk_x * 73856093 ^ p_chunk.chunk_z * 19349663);
	RandomPCG rng(seed);

	LocalVector<Transform3D> valid_transforms;
	valid_transforms.reserve(target_count);

	AABB total_bounds = cached_terrain ? cached_terrain->get_total_aabb() : p_chunk_bounds;

	for (int i = 0; i < target_count; i++) {
		float rx = rng.randf() * p_chunk_bounds.size.x;
		float rz = rng.randf() * p_chunk_bounds.size.z;

		Vector3 local_pos = Vector3(p_chunk_bounds.position.x + rx, 0.0f, p_chunk_bounds.position.z + rz);
		Vector3 world_pos;
		Vector3 ground_normal = Vector3(0.0f, 1.0f, 0.0f);
		float height = 0.0f;

		if (cached_terrain) {
			world_pos = cached_terrain->to_global(local_pos);
			height = cached_terrain->sample_height(world_pos);
			world_pos.y = height;
			ground_normal = cached_terrain->get_normal_at(world_pos);

			// 1. Altitude Filter
			if (height < min_altitude || height > max_altitude) {
				float dist = (height < min_altitude) ? (min_altitude - height) : (height - max_altitude);
				if (dist > altitude_falloff || rng.randf() > (1.0f - (dist / MAX(0.001f, altitude_falloff)))) {
					continue;
				}
			}

			// 2. Slope Filter
			float slope_angle_deg = Math::rad_to_deg(Math::acos(CLAMP(ground_normal.y, -1.0f, 1.0f)));
			if (slope_angle_deg > max_slope_angle) {
				float slope_diff = slope_angle_deg - max_slope_angle;
				if (slope_diff > slope_falloff || rng.randf() > (1.0f - (slope_diff / MAX(0.001f, slope_falloff)))) {
					continue;
				}
			}

			// 3. Density Mask Filter
			if (density_mask_image.is_valid()) {
				float u = CLAMP((local_pos.x - total_bounds.position.x) / MAX(0.001f, total_bounds.size.x), 0.0f, 1.0f);
				float v = CLAMP((local_pos.z - total_bounds.position.z) / MAX(0.001f, total_bounds.size.z), 0.0f, 1.0f);
				float mask_val = _sample_density_mask(u, v);
				if (rng.randf() > mask_val) {
					continue;
				}
			}
		} else {
			world_pos = to_global(local_pos);
			world_pos.y = get_global_position().y;
		}

		// Transform construction
		Vector3 place_pos = to_local(world_pos);

		// Random rotation & scale
		float yaw = rng.randf() * (float)Math::TAU;
		float scale_val = Math::lerp(min_scale, max_scale, rng.randf());

		Basis basis;
		basis.rotate(Vector3(0.0f, 1.0f, 0.0f), yaw);

		// Align to terrain normal
		if (normal_align > 0.001f && ground_normal.length_squared() > 0.01f) {
			Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
			Vector3 target_up = up.lerp(ground_normal, CLAMP(normal_align, 0.0f, 1.0f)).normalized();

			Vector3 axis = up.cross(target_up);
			if (axis.length_squared() > 1e-6) {
				float angle = Math::acos(CLAMP(up.dot(target_up), -1.0f, 1.0f));
				basis = Basis(axis.normalized(), angle) * basis;
			}
		}

		basis.scale(Vector3(scale_val, scale_val, scale_val));

		Transform3D t(basis, place_pos);
		valid_transforms.push_back(t);
	}

	if (valid_transforms.size() == 0) {
		return;
	}

	Ref<MultiMesh> mm;
	mm.instantiate();
	mm->set_transform_format(MultiMesh::TRANSFORM_3D);
	mm->set_use_colors(false);
	mm->set_use_custom_data(false);
	mm->set_mesh(mesh);
	mm->set_instance_count(valid_transforms.size());

	for (uint32_t i = 0; i < valid_transforms.size(); i++) {
		mm->set_instance_transform(i, valid_transforms[i]);
	}

	MultiMeshInstance3D *mmi = memnew(MultiMeshInstance3D);
	mmi->set_name(vformat("GrassChunk_%d_%d", p_chunk.chunk_x, p_chunk.chunk_z));
	mmi->set_multimesh(mm);
	mmi->set_cast_shadows_setting(cast_shadow);

	if (material_override.is_valid()) {
		mmi->set_material_override(material_override);
	} else {
		mmi->set_material_override(default_grass_material);
	}

	chunks_container->add_child(mmi);

	p_chunk.multimesh_instance = mmi;
	p_chunk.multimesh = mm;
	p_chunk.instance_count = valid_transforms.size();
}

void Grass3D::_update_culling(const Vector3 &p_camera_pos) {
	float max_dist_sq = cull_distance * cull_distance;
	float shadow_dist_sq = shadow_cull_distance * shadow_cull_distance;

	for (int i = 0; i < chunks.size(); i++) {
		GrassChunk &c = chunks.write[i];
		if (!c.multimesh_instance) {
			continue;
		}

		Vector3 chunk_center = to_global(c.aabb.position + c.aabb.size * 0.5f);
		float dist_sq = chunk_center.distance_squared_to(p_camera_pos);

		bool should_be_visible = (dist_sq <= max_dist_sq);
		if (c.is_visible != should_be_visible) {
			c.is_visible = should_be_visible;
			c.multimesh_instance->set_visible(should_be_visible);
		}

		if (should_be_visible) {
			// Distance shadow culling eliminates sub-pixel shadow noise on distant hills and boosts FPS
			if (dist_sq > shadow_dist_sq) {
				c.multimesh_instance->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
			} else {
				c.multimesh_instance->set_cast_shadows_setting(cast_shadow);
			}
		}
	}
}

void Grass3D::_update_player_position() {
	if (!auto_player_tracking) {
		return;
	}

	Viewport *vp = get_viewport();
	if (!vp) {
		return;
	}

	Camera3D *cam = vp->get_camera_3d();
	if (cam) {
		Vector3 cam_pos = cam->get_global_position();
		// Place trample position at ground level under camera/player
		player_position = cam_pos;
		if (default_grass_material.is_valid()) {
			default_grass_material->set_shader_parameter("player_position", player_position);
		}
	}
}

HashSet<Grass3D *> Grass3D::active_grass_nodes;

void Grass3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			active_grass_nodes.insert(this);
			set_notify_transform(true);
			_ensure_default_material();
			set_process_internal(true);
		} break;

		case NOTIFICATION_POST_ENTER_TREE: {
			if (chunks.is_empty() || is_dirty) {
				_rebuild_all_chunks();
			}
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (in_transform_update || !is_inside_tree()) {
				return;
			}
			if (anchor_to_terrain && cached_terrain && cached_terrain->is_inside_tree()) {
				in_transform_update = true;
				Transform3D target_xform = cached_terrain->get_global_transform();
				if (!get_global_transform().is_equal_approx(target_xform)) {
					set_global_transform(target_xform);
				}
				in_transform_update = false;
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			Viewport *vp = get_viewport();
			if (vp) {
				Camera3D *cam = vp->get_camera_3d();
				if (cam) {
					Vector3 cam_pos = cam->get_global_position();
					_update_culling(cam_pos);
				}
			}
			if (auto_player_tracking) {
				_update_player_position();
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			active_grass_nodes.erase(this);
			set_process_internal(false);
		} break;
	}
}

void Grass3D::rebuild_grass() {
	_rebuild_all_chunks();
}

void Grass3D::clear_grass() {
	_clear_chunks();
}

void Grass3D::update_culling_from_camera(const Vector3 &p_camera_pos) {
	_update_culling(p_camera_pos);
	if (auto_player_tracking) {
		player_position = p_camera_pos;
		if (default_grass_material.is_valid()) {
			default_grass_material->set_shader_parameter("player_position", player_position);
		}
	}
}

int Grass3D::get_total_instance_count() const {
	int total = 0;
	for (int i = 0; i < chunks.size(); i++) {
		total += chunks[i].instance_count;
	}
	return total;
}

int Grass3D::get_visible_instance_count() const {
	int visible = 0;
	for (int i = 0; i < chunks.size(); i++) {
		if (chunks[i].is_visible) {
			visible += chunks[i].instance_count;
		}
	}
	return visible;
}

// Getters and Setters

void Grass3D::set_mesh_type(MeshType p_type) {
	if (mesh_type == p_type) {
		return;
	}
	mesh_type = p_type;
	is_mesh_dirty = true;
	_rebuild_all_chunks();
}
Grass3D::MeshType Grass3D::get_mesh_type() const { return mesh_type; }

void Grass3D::set_custom_mesh(const Ref<Mesh> &p_mesh) {
	custom_mesh = p_mesh;
	if (mesh_type == MESH_CUSTOM) {
		is_mesh_dirty = true;
		_rebuild_all_chunks();
	}
}
Ref<Mesh> Grass3D::get_custom_mesh() const { return custom_mesh; }

void Grass3D::set_blade_width(float p_width) {
	blade_width = MAX(0.01f, p_width);
	is_mesh_dirty = true;
	_rebuild_all_chunks();
}
float Grass3D::get_blade_width() const { return blade_width; }

void Grass3D::set_blade_height(float p_height) {
	blade_height = MAX(0.05f, p_height);
	is_mesh_dirty = true;
	_rebuild_all_chunks();
}
float Grass3D::get_blade_height() const { return blade_height; }

void Grass3D::set_blade_curviness(float p_curviness) {
	blade_curviness = p_curviness;
	is_mesh_dirty = true;
	_rebuild_all_chunks();
}
float Grass3D::get_blade_curviness() const { return blade_curviness; }

void Grass3D::set_blade_segments(int p_segments) {
	blade_segments = CLAMP(p_segments, 1, 8);
	is_mesh_dirty = true;
	_rebuild_all_chunks();
}
int Grass3D::get_blade_segments() const { return blade_segments; }

void Grass3D::set_material_override(const Ref<Material> &p_material) {
	material_override = p_material;
	for (int i = 0; i < chunks.size(); i++) {
		if (chunks[i].multimesh_instance) {
			chunks[i].multimesh_instance->set_material_override(material_override.is_valid() ? material_override : Ref<Material>(default_grass_material));
		}
	}
}
Ref<Material> Grass3D::get_material_override() const { return material_override; }

void Grass3D::set_custom_shader(const Ref<Shader> &p_shader) {
	custom_shader = p_shader;
	if (default_grass_material.is_valid()) {
		default_grass_material->set_shader(custom_shader.is_valid() ? custom_shader : default_grass_shader);
		_sync_material_uniforms();
	}
}
Ref<Shader> Grass3D::get_custom_shader() const { return custom_shader; }

void Grass3D::set_texture_albedo(const Ref<Texture2D> &p_texture) {
	texture_albedo = p_texture;
	_sync_material_uniforms();
}
Ref<Texture2D> Grass3D::get_texture_albedo() const { return texture_albedo; }

void Grass3D::set_texture_normal(const Ref<Texture2D> &p_texture) {
	texture_normal = p_texture;
	_sync_material_uniforms();
}
Ref<Texture2D> Grass3D::get_texture_normal() const { return texture_normal; }

void Grass3D::set_color_bottom(const Color &p_color) {
	color_bottom = p_color;
	_sync_material_uniforms();
}
Color Grass3D::get_color_bottom() const { return color_bottom; }

void Grass3D::set_color_top(const Color &p_color) {
	color_top = p_color;
	_sync_material_uniforms();
}
Color Grass3D::get_color_top() const { return color_top; }

void Grass3D::set_translucency_color(const Color &p_color) {
	translucency_color = p_color;
	_sync_material_uniforms();
}
Color Grass3D::get_translucency_color() const { return translucency_color; }

void Grass3D::set_alpha_scissor_threshold(float p_threshold) {
	alpha_scissor_threshold = p_threshold;
	_sync_material_uniforms();
}
float Grass3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void Grass3D::set_roughness(float p_roughness) {
	roughness = p_roughness;
	_sync_material_uniforms();
}
float Grass3D::get_roughness() const { return roughness; }

void Grass3D::set_specular(float p_specular) {
	specular = p_specular;
	_sync_material_uniforms();
}
float Grass3D::get_specular() const { return specular; }

void Grass3D::set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_cast_shadow) {
	cast_shadow = p_cast_shadow;
	for (int i = 0; i < chunks.size(); i++) {
		if (chunks[i].multimesh_instance) {
			chunks[i].multimesh_instance->set_cast_shadows_setting(cast_shadow);
		}
	}
}
GeometryInstance3D::ShadowCastingSetting Grass3D::get_cast_shadow() const { return cast_shadow; }

void Grass3D::set_terrain_path(const NodePath &p_path) {
	terrain_path = p_path;
	cached_terrain = nullptr;
	_rebuild_all_chunks();
}
NodePath Grass3D::get_terrain_path() const { return terrain_path; }

void Grass3D::set_density(float p_density) {
	density = MAX(0.01f, p_density);
	_rebuild_all_chunks();
}
float Grass3D::get_density() const { return density; }

void Grass3D::set_chunk_size(float p_size) {
	chunk_size = MAX(4.0f, p_size);
	_rebuild_all_chunks();
}
float Grass3D::get_chunk_size() const { return chunk_size; }

void Grass3D::set_max_instances_per_chunk(int p_max) {
	max_instances_per_chunk = MAX(10, p_max);
	_rebuild_all_chunks();
}
int Grass3D::get_max_instances_per_chunk() const { return max_instances_per_chunk; }

void Grass3D::set_cull_distance(float p_distance) {
	cull_distance = MAX(10.0f, p_distance);
	_sync_material_uniforms();
}
float Grass3D::get_cull_distance() const { return cull_distance; }

void Grass3D::set_fade_range(float p_range) {
	fade_range = MAX(0.0f, p_range);
	_sync_material_uniforms();
}
float Grass3D::get_fade_range() const { return fade_range; }

void Grass3D::set_shadow_cull_distance(float p_distance) {
	shadow_cull_distance = MAX(0.0f, p_distance);
}
float Grass3D::get_shadow_cull_distance() const { return shadow_cull_distance; }

void Grass3D::set_distance_anti_aliasing(bool p_enable) {
	distance_anti_aliasing = p_enable;
	_sync_material_uniforms();
}
bool Grass3D::is_distance_anti_aliasing() const { return distance_anti_aliasing; }

void Grass3D::set_anchor_to_terrain(bool p_anchor) {
	anchor_to_terrain = p_anchor;
	if (anchor_to_terrain && cached_terrain && is_inside_tree()) {
		set_global_transform(cached_terrain->get_global_transform());
	}
}
bool Grass3D::is_anchor_to_terrain() const { return anchor_to_terrain; }

void Grass3D::set_min_scale(float p_scale) {
	min_scale = MAX(0.01f, p_scale);
	_rebuild_all_chunks();
}
float Grass3D::get_min_scale() const { return min_scale; }

void Grass3D::set_max_scale(float p_scale) {
	max_scale = MAX(min_scale, p_scale);
	_rebuild_all_chunks();
}
float Grass3D::get_max_scale() const { return max_scale; }

void Grass3D::set_normal_align(float p_align) {
	normal_align = CLAMP(p_align, 0.0f, 1.0f);
	_rebuild_all_chunks();
}
float Grass3D::get_normal_align() const { return normal_align; }

void Grass3D::set_random_seed(int p_seed) {
	random_seed = p_seed;
	_rebuild_all_chunks();
}
int Grass3D::get_random_seed() const { return random_seed; }

void Grass3D::set_min_altitude(float p_alt) {
	min_altitude = p_alt;
	_rebuild_all_chunks();
}
float Grass3D::get_min_altitude() const { return min_altitude; }

void Grass3D::set_max_altitude(float p_alt) {
	max_altitude = p_alt;
	_rebuild_all_chunks();
}
float Grass3D::get_max_altitude() const { return max_altitude; }

void Grass3D::set_altitude_falloff(float p_falloff) {
	altitude_falloff = MAX(0.01f, p_falloff);
	_rebuild_all_chunks();
}
float Grass3D::get_altitude_falloff() const { return altitude_falloff; }

void Grass3D::set_max_slope_angle(float p_angle) {
	max_slope_angle = CLAMP(p_angle, 0.0f, 90.0f);
	_rebuild_all_chunks();
}
float Grass3D::get_max_slope_angle() const { return max_slope_angle; }

void Grass3D::set_slope_falloff(float p_falloff) {
	slope_falloff = MAX(0.01f, p_falloff);
	_rebuild_all_chunks();
}
float Grass3D::get_slope_falloff() const { return slope_falloff; }

void Grass3D::set_density_mask(const Ref<Texture2D> &p_texture) {
	density_mask = p_texture;
	_rebuild_all_chunks();
}
Ref<Texture2D> Grass3D::get_density_mask() const { return density_mask; }

void Grass3D::set_density_mask_channel(DensityMaskChannel p_channel) {
	density_mask_channel = p_channel;
	_rebuild_all_chunks();
}
Grass3D::DensityMaskChannel Grass3D::get_density_mask_channel() const { return density_mask_channel; }

void Grass3D::set_wind_direction(const Vector2 &p_dir) {
	wind_direction = p_dir;
	_sync_material_uniforms();
}
Vector2 Grass3D::get_wind_direction() const { return wind_direction; }

void Grass3D::set_wind_strength(float p_strength) {
	wind_strength = p_strength;
	_sync_material_uniforms();
}
float Grass3D::get_wind_strength() const { return wind_strength; }

void Grass3D::set_wind_speed(float p_speed) {
	wind_speed = p_speed;
	_sync_material_uniforms();
}
float Grass3D::get_wind_speed() const { return wind_speed; }

void Grass3D::set_gust_frequency(float p_freq) {
	gust_frequency = p_freq;
	_sync_material_uniforms();
}
float Grass3D::get_gust_frequency() const { return gust_frequency; }

void Grass3D::set_flutter_strength(float p_flutter) {
	flutter_strength = p_flutter;
	_sync_material_uniforms();
}
float Grass3D::get_flutter_strength() const { return flutter_strength; }

void Grass3D::set_player_position(const Vector3 &p_pos) {
	player_position = p_pos;
	_sync_material_uniforms();
}
Vector3 Grass3D::get_player_position() const { return player_position; }

void Grass3D::set_player_radius(float p_radius) {
	player_radius = p_radius;
	_sync_material_uniforms();
}
float Grass3D::get_player_radius() const { return player_radius; }

void Grass3D::set_trample_strength(float p_strength) {
	trample_strength = p_strength;
	_sync_material_uniforms();
}
float Grass3D::get_trample_strength() const { return trample_strength; }

void Grass3D::set_auto_player_tracking(bool p_track) {
	auto_player_tracking = p_track;
}
bool Grass3D::is_auto_player_tracking() const { return auto_player_tracking; }

void Grass3D::set_show_chunk_bounds(bool p_show) {
	show_chunk_bounds = p_show;
}
bool Grass3D::is_show_chunk_bounds() const { return show_chunk_bounds; }

void Grass3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh_type", "type"), &Grass3D::set_mesh_type);
	ClassDB::bind_method(D_METHOD("get_mesh_type"), &Grass3D::get_mesh_type);

	ClassDB::bind_method(D_METHOD("set_custom_mesh", "mesh"), &Grass3D::set_custom_mesh);
	ClassDB::bind_method(D_METHOD("get_custom_mesh"), &Grass3D::get_custom_mesh);

	ClassDB::bind_method(D_METHOD("set_blade_width", "width"), &Grass3D::set_blade_width);
	ClassDB::bind_method(D_METHOD("get_blade_width"), &Grass3D::get_blade_width);

	ClassDB::bind_method(D_METHOD("set_blade_height", "height"), &Grass3D::set_blade_height);
	ClassDB::bind_method(D_METHOD("get_blade_height"), &Grass3D::get_blade_height);

	ClassDB::bind_method(D_METHOD("set_blade_curviness", "curviness"), &Grass3D::set_blade_curviness);
	ClassDB::bind_method(D_METHOD("get_blade_curviness"), &Grass3D::get_blade_curviness);

	ClassDB::bind_method(D_METHOD("set_blade_segments", "segments"), &Grass3D::set_blade_segments);
	ClassDB::bind_method(D_METHOD("get_blade_segments"), &Grass3D::get_blade_segments);

	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &Grass3D::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &Grass3D::get_material_override);

	ClassDB::bind_method(D_METHOD("set_custom_shader", "shader"), &Grass3D::set_custom_shader);
	ClassDB::bind_method(D_METHOD("get_custom_shader"), &Grass3D::get_custom_shader);

	ClassDB::bind_method(D_METHOD("set_texture_albedo", "texture"), &Grass3D::set_texture_albedo);
	ClassDB::bind_method(D_METHOD("get_texture_albedo"), &Grass3D::get_texture_albedo);

	ClassDB::bind_method(D_METHOD("set_texture_normal", "texture"), &Grass3D::set_texture_normal);
	ClassDB::bind_method(D_METHOD("get_texture_normal"), &Grass3D::get_texture_normal);

	ClassDB::bind_method(D_METHOD("set_color_bottom", "color"), &Grass3D::set_color_bottom);
	ClassDB::bind_method(D_METHOD("get_color_bottom"), &Grass3D::get_color_bottom);

	ClassDB::bind_method(D_METHOD("set_color_top", "color"), &Grass3D::set_color_top);
	ClassDB::bind_method(D_METHOD("get_color_top"), &Grass3D::get_color_top);

	ClassDB::bind_method(D_METHOD("set_translucency_color", "color"), &Grass3D::set_translucency_color);
	ClassDB::bind_method(D_METHOD("get_translucency_color"), &Grass3D::get_translucency_color);

	ClassDB::bind_method(D_METHOD("set_alpha_scissor_threshold", "threshold"), &Grass3D::set_alpha_scissor_threshold);
	ClassDB::bind_method(D_METHOD("get_alpha_scissor_threshold"), &Grass3D::get_alpha_scissor_threshold);

	ClassDB::bind_method(D_METHOD("set_roughness", "roughness"), &Grass3D::set_roughness);
	ClassDB::bind_method(D_METHOD("get_roughness"), &Grass3D::get_roughness);

	ClassDB::bind_method(D_METHOD("set_specular", "specular"), &Grass3D::set_specular);
	ClassDB::bind_method(D_METHOD("get_specular"), &Grass3D::get_specular);

	ClassDB::bind_method(D_METHOD("set_cast_shadow", "cast_shadow"), &Grass3D::set_cast_shadow);
	ClassDB::bind_method(D_METHOD("get_cast_shadow"), &Grass3D::get_cast_shadow);

	ClassDB::bind_method(D_METHOD("set_terrain_path", "path"), &Grass3D::set_terrain_path);
	ClassDB::bind_method(D_METHOD("get_terrain_path"), &Grass3D::get_terrain_path);

	ClassDB::bind_method(D_METHOD("set_density", "density"), &Grass3D::set_density);
	ClassDB::bind_method(D_METHOD("get_density"), &Grass3D::get_density);

	ClassDB::bind_method(D_METHOD("set_chunk_size", "size"), &Grass3D::set_chunk_size);
	ClassDB::bind_method(D_METHOD("get_chunk_size"), &Grass3D::get_chunk_size);

	ClassDB::bind_method(D_METHOD("set_max_instances_per_chunk", "max"), &Grass3D::set_max_instances_per_chunk);
	ClassDB::bind_method(D_METHOD("get_max_instances_per_chunk"), &Grass3D::get_max_instances_per_chunk);

	ClassDB::bind_method(D_METHOD("set_cull_distance", "distance"), &Grass3D::set_cull_distance);
	ClassDB::bind_method(D_METHOD("get_cull_distance"), &Grass3D::get_cull_distance);

	ClassDB::bind_method(D_METHOD("set_fade_range", "range"), &Grass3D::set_fade_range);
	ClassDB::bind_method(D_METHOD("get_fade_range"), &Grass3D::get_fade_range);

	ClassDB::bind_method(D_METHOD("set_shadow_cull_distance", "distance"), &Grass3D::set_shadow_cull_distance);
	ClassDB::bind_method(D_METHOD("get_shadow_cull_distance"), &Grass3D::get_shadow_cull_distance);

	ClassDB::bind_method(D_METHOD("set_distance_anti_aliasing", "enable"), &Grass3D::set_distance_anti_aliasing);
	ClassDB::bind_method(D_METHOD("is_distance_anti_aliasing"), &Grass3D::is_distance_anti_aliasing);

	ClassDB::bind_method(D_METHOD("set_anchor_to_terrain", "anchor"), &Grass3D::set_anchor_to_terrain);
	ClassDB::bind_method(D_METHOD("is_anchor_to_terrain"), &Grass3D::is_anchor_to_terrain);

	ClassDB::bind_method(D_METHOD("set_min_scale", "scale"), &Grass3D::set_min_scale);
	ClassDB::bind_method(D_METHOD("get_min_scale"), &Grass3D::get_min_scale);

	ClassDB::bind_method(D_METHOD("set_max_scale", "scale"), &Grass3D::set_max_scale);
	ClassDB::bind_method(D_METHOD("get_max_scale"), &Grass3D::get_max_scale);

	ClassDB::bind_method(D_METHOD("set_normal_align", "align"), &Grass3D::set_normal_align);
	ClassDB::bind_method(D_METHOD("get_normal_align"), &Grass3D::get_normal_align);

	ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &Grass3D::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &Grass3D::get_random_seed);

	ClassDB::bind_method(D_METHOD("set_min_altitude", "alt"), &Grass3D::set_min_altitude);
	ClassDB::bind_method(D_METHOD("get_min_altitude"), &Grass3D::get_min_altitude);

	ClassDB::bind_method(D_METHOD("set_max_altitude", "alt"), &Grass3D::set_max_altitude);
	ClassDB::bind_method(D_METHOD("get_max_altitude"), &Grass3D::get_max_altitude);

	ClassDB::bind_method(D_METHOD("set_altitude_falloff", "falloff"), &Grass3D::set_altitude_falloff);
	ClassDB::bind_method(D_METHOD("get_altitude_falloff"), &Grass3D::get_altitude_falloff);

	ClassDB::bind_method(D_METHOD("set_max_slope_angle", "angle"), &Grass3D::set_max_slope_angle);
	ClassDB::bind_method(D_METHOD("get_max_slope_angle"), &Grass3D::get_max_slope_angle);

	ClassDB::bind_method(D_METHOD("set_slope_falloff", "falloff"), &Grass3D::set_slope_falloff);
	ClassDB::bind_method(D_METHOD("get_slope_falloff"), &Grass3D::get_slope_falloff);

	ClassDB::bind_method(D_METHOD("set_density_mask", "texture"), &Grass3D::set_density_mask);
	ClassDB::bind_method(D_METHOD("get_density_mask"), &Grass3D::get_density_mask);

	ClassDB::bind_method(D_METHOD("set_density_mask_channel", "channel"), &Grass3D::set_density_mask_channel);
	ClassDB::bind_method(D_METHOD("get_density_mask_channel"), &Grass3D::get_density_mask_channel);

	ClassDB::bind_method(D_METHOD("set_wind_direction", "dir"), &Grass3D::set_wind_direction);
	ClassDB::bind_method(D_METHOD("get_wind_direction"), &Grass3D::get_wind_direction);

	ClassDB::bind_method(D_METHOD("set_wind_strength", "strength"), &Grass3D::set_wind_strength);
	ClassDB::bind_method(D_METHOD("get_wind_strength"), &Grass3D::get_wind_strength);

	ClassDB::bind_method(D_METHOD("set_wind_speed", "speed"), &Grass3D::set_wind_speed);
	ClassDB::bind_method(D_METHOD("get_wind_speed"), &Grass3D::get_wind_speed);

	ClassDB::bind_method(D_METHOD("set_gust_frequency", "freq"), &Grass3D::set_gust_frequency);
	ClassDB::bind_method(D_METHOD("get_gust_frequency"), &Grass3D::get_gust_frequency);

	ClassDB::bind_method(D_METHOD("set_flutter_strength", "strength"), &Grass3D::set_flutter_strength);
	ClassDB::bind_method(D_METHOD("get_flutter_strength"), &Grass3D::get_flutter_strength);

	ClassDB::bind_method(D_METHOD("set_player_position", "pos"), &Grass3D::set_player_position);
	ClassDB::bind_method(D_METHOD("get_player_position"), &Grass3D::get_player_position);

	ClassDB::bind_method(D_METHOD("set_player_radius", "radius"), &Grass3D::set_player_radius);
	ClassDB::bind_method(D_METHOD("get_player_radius"), &Grass3D::get_player_radius);

	ClassDB::bind_method(D_METHOD("set_trample_strength", "strength"), &Grass3D::set_trample_strength);
	ClassDB::bind_method(D_METHOD("get_trample_strength"), &Grass3D::get_trample_strength);

	ClassDB::bind_method(D_METHOD("set_auto_player_tracking", "track"), &Grass3D::set_auto_player_tracking);
	ClassDB::bind_method(D_METHOD("is_auto_player_tracking"), &Grass3D::is_auto_player_tracking);

	ClassDB::bind_method(D_METHOD("set_show_chunk_bounds", "show"), &Grass3D::set_show_chunk_bounds);
	ClassDB::bind_method(D_METHOD("is_show_chunk_bounds"), &Grass3D::is_show_chunk_bounds);

	ClassDB::bind_method(D_METHOD("rebuild_grass"), &Grass3D::rebuild_grass);
	ClassDB::bind_method(D_METHOD("clear_grass"), &Grass3D::clear_grass);

	ClassDB::bind_method(D_METHOD("get_chunk_count"), &Grass3D::get_chunk_count);
	ClassDB::bind_method(D_METHOD("get_total_instance_count"), &Grass3D::get_total_instance_count);
	ClassDB::bind_method(D_METHOD("get_visible_instance_count"), &Grass3D::get_visible_instance_count);

	// Group: Mesh & Geometry
	ADD_GROUP("Mesh & Geometry", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_type", PROPERTY_HINT_ENUM, "Cross Quads,Tri-Star (6-Point),Curved Clump,Custom Mesh"), "set_mesh_type", "get_mesh_type");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_custom_mesh", "get_custom_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blade_width", PROPERTY_HINT_RANGE, "0.01,2.0,0.01"), "set_blade_width", "get_blade_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blade_height", PROPERTY_HINT_RANGE, "0.05,5.0,0.05"), "set_blade_height", "get_blade_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blade_curviness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_blade_curviness", "get_blade_curviness");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "blade_segments", PROPERTY_HINT_RANGE, "1,8,1"), "set_blade_segments", "get_blade_segments");

	// Group: Material & Shading
	ADD_GROUP("Material & Shading", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material_override", "get_material_override");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_shader", PROPERTY_HINT_RESOURCE_TYPE, "Shader"), "set_custom_shader", "get_custom_shader");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_albedo", "get_texture_albedo");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture_normal", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_normal", "get_texture_normal");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_bottom"), "set_color_bottom", "get_color_bottom");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_top"), "set_color_top", "get_color_top");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "translucency_color"), "set_translucency_color", "get_translucency_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alpha_scissor_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_alpha_scissor_threshold", "get_alpha_scissor_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roughness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_roughness", "get_roughness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "specular", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_specular", "get_specular");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cast_shadow", PROPERTY_HINT_ENUM, "Off,On,Double-Sided,Shadows Only"), "set_cast_shadow", "get_cast_shadow");

	// Group: Placement & Density
	ADD_GROUP("Placement & Density", "");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "terrain_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Terrain3D"), "set_terrain_path", "get_terrain_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anchor_to_terrain"), "set_anchor_to_terrain", "is_anchor_to_terrain");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "0.01,50.0,0.1"), "set_density", "get_density");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chunk_size", PROPERTY_HINT_RANGE, "8.0,128.0,4.0"), "set_chunk_size", "get_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_instances_per_chunk", PROPERTY_HINT_RANGE, "100,50000,100"), "set_max_instances_per_chunk", "get_max_instances_per_chunk");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cull_distance", PROPERTY_HINT_RANGE, "10.0,1000.0,5.0"), "set_cull_distance", "get_cull_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_range", PROPERTY_HINT_RANGE, "0.0,200.0,1.0"), "set_fade_range", "get_fade_range");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shadow_cull_distance", PROPERTY_HINT_RANGE, "0.0,500.0,5.0"), "set_shadow_cull_distance", "get_shadow_cull_distance");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "distance_anti_aliasing"), "set_distance_anti_aliasing", "is_distance_anti_aliasing");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_scale", PROPERTY_HINT_RANGE, "0.1,5.0,0.05"), "set_min_scale", "get_min_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_scale", PROPERTY_HINT_RANGE, "0.1,5.0,0.05"), "set_max_scale", "get_max_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_align", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_normal_align", "get_normal_align");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");

	// Group: Environmental Filters
	ADD_GROUP("Environmental Filters", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_altitude", PROPERTY_HINT_RANGE, "-10000.0,10000.0,1.0"), "set_min_altitude", "get_min_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_altitude", PROPERTY_HINT_RANGE, "-10000.0,10000.0,1.0"), "set_max_altitude", "get_max_altitude");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "altitude_falloff", PROPERTY_HINT_RANGE, "0.1,100.0,0.5"), "set_altitude_falloff", "get_altitude_falloff");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_slope_angle", PROPERTY_HINT_RANGE, "0.0,90.0,1.0"), "set_max_slope_angle", "get_max_slope_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "slope_falloff", PROPERTY_HINT_RANGE, "0.1,45.0,0.5"), "set_slope_falloff", "get_slope_falloff");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "density_mask", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_density_mask", "get_density_mask");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "density_mask_channel", PROPERTY_HINT_ENUM, "Red,Green,Blue,Alpha"), "set_density_mask_channel", "get_density_mask_channel");

	// Group: Wind & Dynamics
	ADD_GROUP("Wind & Dynamics", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "wind_direction"), "set_wind_direction", "get_wind_direction");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_strength", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_wind_strength", "get_wind_strength");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wind_speed", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_wind_speed", "get_wind_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gust_frequency", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_gust_frequency", "get_gust_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flutter_strength", PROPERTY_HINT_RANGE, "0.0,0.5,0.005"), "set_flutter_strength", "get_flutter_strength");

	// Group: Player Interaction
	ADD_GROUP("Player Interaction", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "player_position"), "set_player_position", "get_player_position");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "player_radius", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_player_radius", "get_player_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trample_strength", PROPERTY_HINT_RANGE, "0.0,2.0,0.05"), "set_trample_strength", "get_trample_strength");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_player_tracking"), "set_auto_player_tracking", "is_auto_player_tracking");

	// Group: Diagnostics
	ADD_GROUP("Diagnostics", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_chunk_bounds"), "set_show_chunk_bounds", "is_show_chunk_bounds");

	BIND_ENUM_CONSTANT(MESH_CROSS_QUADS);
	BIND_ENUM_CONSTANT(MESH_TRI_STAR);
	BIND_ENUM_CONSTANT(MESH_CURVED_CLUMP);
	BIND_ENUM_CONSTANT(MESH_CUSTOM);

	BIND_ENUM_CONSTANT(CHANNEL_RED);
	BIND_ENUM_CONSTANT(CHANNEL_GREEN);
	BIND_ENUM_CONSTANT(CHANNEL_BLUE);
	BIND_ENUM_CONSTANT(CHANNEL_ALPHA);
}

Grass3D::Grass3D() {
}

Grass3D::~Grass3D() {
	active_grass_nodes.erase(this);
	_clear_chunks();
}
