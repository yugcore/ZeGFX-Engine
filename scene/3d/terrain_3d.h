/**************************************************************************/
/*  terrain_3d.h                                                          */
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

#include "core/io/image.h"
#include "core/templates/local_vector.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/resources/3d/height_map_shape_3d.h"
#endif // PHYSICS_3D_DISABLED

class MeshInstance3D;

class Terrain3D : public Node3D {
	GDCLASS(Terrain3D, Node3D);

public:
	enum ResampleFilter {
		RESAMPLE_BILINEAR,
		RESAMPLE_BICUBIC,
		RESAMPLE_BOX,
	};

	enum BrushMode {
		BRUSH_RAISE,
		BRUSH_LOWER,
		BRUSH_SMOOTH,
		BRUSH_FLATTEN,
	};

	struct TerrainChunk {
		int chunk_x = 0;
		int chunk_z = 0;
		MeshInstance3D *mesh_instance = nullptr;
		Vector<Ref<ArrayMesh>> lod_meshes; // LOD 0 (full detail) down to LOD N-1
		int current_lod = 0;
		AABB aabb;
	};

private:
	// Heightmap & Dimensions
	String heightmap_file;
	Ref<Texture2D> heightmap_texture;
	Ref<Image> heightmap_image;
	Vector<float> heights; // Normalized [0..1]
	int map_width = 0;
	int map_height = 0;

	float cell_size = 1.0f;
	float height_scale = 50.0f;
	int chunk_size = 64; // In quads (e.g. 64x64 quads)
	bool center_pivot = true;
	int smoothing_cycles = 1;
	ResampleFilter resample_filter = RESAMPLE_BICUBIC;

	// Dynamic Multi-Level LOD & Streaming
	bool lod_enabled = true;
	int lod_count = 4;
	float lod_distance_step = 150.0f;
	float max_view_distance = 4000.0f;
	float skirt_height = 10.0f;
	bool debug_lod_colors = false;
	int max_lod_swaps_per_frame = 8; // Throttled budget to prevent main-thread stutter
	float lod_hysteresis_margin = 0.15f; // Margin to prevent boundary thrashing
	float lod_update_distance_threshold = 4.0f; // Minimum camera movement to trigger re-evaluation
	Vector3 last_lod_camera_pos = Vector3(1e9f, 1e9f, 1e9f);

	// Multi-Layer PBR Material Pipeline
	Ref<Material> material; // User custom material override
	Ref<ShaderMaterial> default_terrain_material; // Built-in procedural multi-layer material
	Ref<Shader> default_terrain_shader;
	GeometryInstance3D::ShadowCastingSetting cast_shadow = GeometryInstance3D::SHADOW_CASTING_SETTING_ON;

	bool auto_material_enabled = true;
	Ref<Texture2D> splatmap_texture;

	// Layer 0: Flatland / Grass
	Color layer_0_color = Color(0.28f, 0.48f, 0.20f);
	Ref<Texture2D> layer_0_albedo;
	Ref<Texture2D> layer_0_normal;
	float layer_0_roughness = 0.8f;
	float layer_0_uv_scale = 20.0f;

	// Layer 1: Cliffs / Rock (Triplanar)
	Color layer_1_color = Color(0.44f, 0.42f, 0.40f);
	Ref<Texture2D> layer_1_albedo;
	Ref<Texture2D> layer_1_normal;
	float layer_1_roughness = 0.9f;
	float layer_1_uv_scale = 15.0f;

	// Layer 2: High Altitude / Snow
	Color layer_2_color = Color(0.92f, 0.95f, 0.98f);
	Ref<Texture2D> layer_2_albedo;
	Ref<Texture2D> layer_2_normal;
	float layer_2_roughness = 0.4f;
	float layer_2_uv_scale = 20.0f;

	// Layer 3: Low Altitude / Sand / Mud
	Color layer_3_color = Color(0.68f, 0.58f, 0.42f);
	Ref<Texture2D> layer_3_albedo;
	Ref<Texture2D> layer_3_normal;
	float layer_3_roughness = 0.85f;
	float layer_3_uv_scale = 20.0f;

	// Auto Blending Parameters
	float slope_threshold = 0.65f;
	float slope_sharpness = 6.0f;
	float snow_altitude = 75.0f;
	float snow_falloff = 15.0f;
	float sand_altitude = 5.0f;
	float sand_falloff = 5.0f;
	float macro_variation = 0.25f;
	bool triplanar_cliffs = true;

	// Physics & Collision
	bool generate_collision = true;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;

	// Visual Debugging
	bool show_chunk_bounds = false;

	// Internal State
	Node3D *chunks_container = nullptr;
	Vector<TerrainChunk> chunks;

#ifndef PHYSICS_3D_DISABLED
	StaticBody3D *collision_body = nullptr;
	CollisionShape3D *collision_shape_node = nullptr;
	Ref<HeightMapShape3D> collision_shape;
#endif // PHYSICS_3D_DISABLED

	bool is_dirty = false;

	void _clear_chunks();
	void _load_heightmap_data();
	void _build_chunk_meshes();
	Ref<ArrayMesh> _create_chunk_lod_mesh(int cx, int cz, int p_lod, AABB &r_aabb);
	void _rebuild_chunk(int cx, int cz);
	void _update_lod(const Vector3 &p_camera_pos);
	void _update_materials();
	void _update_lod_materials();
	void _ensure_default_material();
	void _sync_material_uniforms();
	void _sync_physics();
	void _update_chunk_bounds_visibility();
	void _smooth_heights(int p_cycles);

	float _get_height_raw(int x, int z) const;
	Vector3 _calc_normal(int x, int z) const;

	Vector<Ref<StandardMaterial3D>> debug_lod_materials;
	void _init_debug_materials();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Heightmap Source
	void set_heightmap_file(const String &p_path);
	String get_heightmap_file() const;

	void set_heightmap_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_heightmap_texture() const;

	void set_smoothing_cycles(int p_cycles);
	int get_smoothing_cycles() const;

	// Grid & Scale
	void set_cell_size(float p_size);
	float get_cell_size() const;

	void set_height_scale(float p_scale);
	float get_height_scale() const;

	void set_chunk_size(int p_size);
	int get_chunk_size() const;

	void set_center_pivot(bool p_center);
	bool is_center_pivot() const;

	void set_resample_filter(ResampleFilter p_filter);
	ResampleFilter get_resample_filter() const;

	// Dynamic LOD Settings
	void set_lod_enabled(bool p_enabled);
	bool is_lod_enabled() const;

	void set_lod_count(int p_count);
	int get_lod_count() const;

	void set_lod_distance_step(float p_step);
	float get_lod_distance_step() const;

	void set_max_view_distance(float p_dist);
	float get_max_view_distance() const;

	void set_skirt_height(float p_height);
	float get_skirt_height() const;

	void set_debug_lod_colors(bool p_debug);
	bool is_debug_lod_colors() const;

	void set_max_lod_swaps_per_frame(int p_swaps);
	int get_max_lod_swaps_per_frame() const;

	void set_lod_hysteresis_margin(float p_margin);
	float get_lod_hysteresis_margin() const;

	void set_lod_update_distance_threshold(float p_threshold);
	float get_lod_update_distance_threshold() const;

	void update_lod(const Vector3 &p_camera_pos) { _update_lod(p_camera_pos); }

	// Multi-Layer Material & Splatmap Settings
	void set_auto_material_enabled(bool p_enabled);
	bool is_auto_material_enabled() const;

	void set_splatmap_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_splatmap_texture() const;

	// Layer 0: Flat / Grass
	void set_layer_0_color(const Color &p_color);
	Color get_layer_0_color() const;
	void set_layer_0_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_0_albedo() const;
	void set_layer_0_normal(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_0_normal() const;
	void set_layer_0_roughness(float p_roughness);
	float get_layer_0_roughness() const;
	void set_layer_0_uv_scale(float p_scale);
	float get_layer_0_uv_scale() const;

	// Layer 1: Cliffs / Rock
	void set_layer_1_color(const Color &p_color);
	Color get_layer_1_color() const;
	void set_layer_1_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_1_albedo() const;
	void set_layer_1_normal(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_1_normal() const;
	void set_layer_1_roughness(float p_roughness);
	float get_layer_1_roughness() const;
	void set_layer_1_uv_scale(float p_scale);
	float get_layer_1_uv_scale() const;

	// Layer 2: Snow / Peak
	void set_layer_2_color(const Color &p_color);
	Color get_layer_2_color() const;
	void set_layer_2_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_2_albedo() const;
	void set_layer_2_normal(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_2_normal() const;
	void set_layer_2_roughness(float p_roughness);
	float get_layer_2_roughness() const;
	void set_layer_2_uv_scale(float p_scale);
	float get_layer_2_uv_scale() const;

	// Layer 3: Sand / Basin
	void set_layer_3_color(const Color &p_color);
	Color get_layer_3_color() const;
	void set_layer_3_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_3_albedo() const;
	void set_layer_3_normal(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_layer_3_normal() const;
	void set_layer_3_roughness(float p_roughness);
	float get_layer_3_roughness() const;
	void set_layer_3_uv_scale(float p_scale);
	float get_layer_3_uv_scale() const;

	// Auto Blending Controls
	void set_slope_threshold(float p_val);
	float get_slope_threshold() const;
	void set_slope_sharpness(float p_val);
	float get_slope_sharpness() const;
	void set_snow_altitude(float p_val);
	float get_snow_altitude() const;
	void set_snow_falloff(float p_val);
	float get_snow_falloff() const;
	void set_sand_altitude(float p_val);
	float get_sand_altitude() const;
	void set_sand_falloff(float p_val);
	float get_sand_falloff() const;
	void set_macro_variation(float p_val);
	float get_macro_variation() const;
	void set_triplanar_cliffs(bool p_enabled);
	bool is_triplanar_cliffs() const;

	// General Material & Lighting
	void set_material(const Ref<Material> &p_material);
	Ref<Material> get_material() const;

	void set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_shadow_casting_setting);
	GeometryInstance3D::ShadowCastingSetting get_cast_shadow() const;

	// Collision
	void set_generate_collision(bool p_enabled);
	bool is_generate_collision() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	// Debug
	void set_show_chunk_bounds(bool p_show);
	bool is_show_chunk_bounds() const;

	// Sculpting & Editing
	void sculpt(const Vector3 &p_world_pos, float p_radius, float p_strength, BrushMode p_mode, float p_target_height = 0.0f);
	void sculpt_grid(int p_gx, int p_gz, float p_grid_radius, float p_strength, BrushMode p_mode, float p_target_height = 0.0f);
	void set_height_at_grid(int p_x, int p_z, float p_height);
	void update_chunks_in_region(int p_min_gx, int p_min_gz, int p_max_gx, int p_max_gz);

	Vector<float> get_heights_raw() const { return heights; }
	void set_heights_raw(const Vector<float> &p_heights) {
		heights = p_heights;
		int dim = (int)Math::sqrt((double)heights.size());
		if (dim * dim == heights.size() && dim >= 2) {
			map_width = dim;
			map_height = dim;
		}
		rebuild_terrain();
	}

	// Actions & Queries
	void rebuild_terrain();
	void bake_collision();

	float sample_height(const Vector3 &p_world_pos) const;
	Vector3 get_normal_at(const Vector3 &p_world_pos) const;
	float get_height_at_grid(int p_x, int p_z) const;

	int get_map_width() const { return map_width; }
	int get_map_height() const { return map_height; }
	int get_chunk_count() const { return chunks.size(); }
	Vector2 get_terrain_size() const;
	AABB get_total_aabb() const;

	TypedArray<AABB> get_chunk_aabbs() const;

	Terrain3D();
	~Terrain3D();
};

VARIANT_ENUM_CAST(Terrain3D::ResampleFilter);
VARIANT_ENUM_CAST(Terrain3D::BrushMode);
