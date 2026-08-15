/**************************************************************************/
/*  grass_3d.h                                                            */
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
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/terrain_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"

class Grass3D : public Node3D {
	GDCLASS(Grass3D, Node3D);

public:
	enum MeshType {
		MESH_CROSS_QUADS,
		MESH_TRI_STAR,
		MESH_CURVED_CLUMP,
		MESH_CUSTOM,
	};

	enum DensityMaskChannel {
		CHANNEL_RED,
		CHANNEL_GREEN,
		CHANNEL_BLUE,
		CHANNEL_ALPHA,
	};

	struct GrassChunk {
		int chunk_x = 0;
		int chunk_z = 0;
		MultiMeshInstance3D *multimesh_instance = nullptr;
		Ref<MultiMesh> multimesh;
		AABB aabb;
		int instance_count = 0;
		bool is_visible = true;
	};

private:
	// Mesh & Geometry Settings
	MeshType mesh_type = MESH_CROSS_QUADS;
	Ref<Mesh> custom_mesh;
	float blade_width = 0.35f;
	float blade_height = 1.1f;
	float blade_curviness = 0.2f;
	int blade_segments = 3;

	// Material & Shading
	Ref<Material> material_override;
	Ref<Shader> custom_shader;
	Ref<ShaderMaterial> default_grass_material;
	Ref<Shader> default_grass_shader;

	Ref<Texture2D> texture_albedo;
	Ref<Texture2D> texture_normal;
	Color color_bottom = Color(0.12f, 0.24f, 0.08f);
	Color color_top = Color(0.46f, 0.68f, 0.18f);
	Color translucency_color = Color(0.38f, 0.55f, 0.18f);
	float alpha_scissor_threshold = 0.5f;
	float roughness = 0.65f;
	float specular = 0.25f;
	GeometryInstance3D::ShadowCastingSetting cast_shadow = GeometryInstance3D::SHADOW_CASTING_SETTING_ON;

	// Placement & Spatial Settings
	NodePath terrain_path;
	Terrain3D *cached_terrain = nullptr;
	bool anchor_to_terrain = true;
	float density = 3.0f; // Instances per m^2
	float chunk_size = 32.0f; // Chunk dimension in meters
	int max_instances_per_chunk = 6000;
	float cull_distance = 140.0f;
	float fade_range = 25.0f;
	float shadow_cull_distance = 45.0f;
	bool distance_anti_aliasing = true;
	float min_scale = 0.75f;
	float max_scale = 1.35f;
	float normal_align = 0.6f; // 0 = straight up, 1 = terrain normal
	int random_seed = 1337;

	// Environmental / Altitude / Slope Filtering
	float min_altitude = -10000.0f;
	float max_altitude = 10000.0f;
	float altitude_falloff = 10.0f;
	float max_slope_angle = 40.0f; // in degrees
	float slope_falloff = 8.0f; // in degrees

	// Density Mask
	Ref<Texture2D> density_mask;
	DensityMaskChannel density_mask_channel = CHANNEL_RED;
	Ref<Image> density_mask_image;

	// Wind & Dynamics
	Vector2 wind_direction = Vector2(1.0f, 0.35f);
	float wind_strength = 0.35f;
	float wind_speed = 1.8f;
	float gust_frequency = 0.25f;
	float flutter_strength = 0.08f;

	// Dynamic Interactor / Trample
	Vector3 player_position = Vector3(0.0f, -10000.0f, 0.0f);
	float player_radius = 1.8f;
	float trample_strength = 0.8f;
	bool auto_player_tracking = true;

	// Debug
	bool show_chunk_bounds = false;

	// Internal State & Sub-nodes
	Node3D *chunks_container = nullptr;
	Vector<GrassChunk> chunks;
	Ref<ArrayMesh> generated_mesh;
	bool is_dirty = false;
	bool is_mesh_dirty = false;
	bool in_transform_update = false;

	void _clear_chunks();
	void _build_procedural_mesh();
	void _ensure_default_material();
	void _sync_material_uniforms();
	void _find_terrain();
	void _rebuild_all_chunks();
	void _rebuild_single_chunk(GrassChunk &p_chunk, const AABB &p_chunk_bounds);
	void _update_culling(const Vector3 &p_camera_pos);
	void _update_player_position();
	void _load_density_mask_data();
	float _sample_density_mask(float p_u, float p_v) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Mesh & Geometry
	void set_mesh_type(MeshType p_type);
	MeshType get_mesh_type() const;

	void set_custom_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_custom_mesh() const;

	void set_blade_width(float p_width);
	float get_blade_width() const;

	void set_blade_height(float p_height);
	float get_blade_height() const;

	void set_blade_curviness(float p_curviness);
	float get_blade_curviness() const;

	void set_blade_segments(int p_segments);
	int get_blade_segments() const;

	Ref<Mesh> get_effective_mesh() const;

	// Material & Color
	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const;

	void set_custom_shader(const Ref<Shader> &p_shader);
	Ref<Shader> get_custom_shader() const;

	void set_texture_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture_albedo() const;

	void set_texture_normal(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture_normal() const;

	void set_color_bottom(const Color &p_color);
	Color get_color_bottom() const;

	void set_color_top(const Color &p_color);
	Color get_color_top() const;

	void set_translucency_color(const Color &p_color);
	Color get_translucency_color() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_roughness(float p_roughness);
	float get_roughness() const;

	void set_specular(float p_specular);
	float get_specular() const;

	void set_cast_shadow(GeometryInstance3D::ShadowCastingSetting p_cast_shadow);
	GeometryInstance3D::ShadowCastingSetting get_cast_shadow() const;

	// Terrain & Placement
	void set_terrain_path(const NodePath &p_path);
	NodePath get_terrain_path() const;

	void set_density(float p_density);
	float get_density() const;

	void set_chunk_size(float p_size);
	float get_chunk_size() const;

	void set_max_instances_per_chunk(int p_max);
	int get_max_instances_per_chunk() const;

	void set_cull_distance(float p_distance);
	float get_cull_distance() const;

	void set_fade_range(float p_range);
	float get_fade_range() const;

	void set_shadow_cull_distance(float p_distance);
	float get_shadow_cull_distance() const;

	void set_distance_anti_aliasing(bool p_enable);
	bool is_distance_anti_aliasing() const;

	void set_anchor_to_terrain(bool p_anchor);
	bool is_anchor_to_terrain() const;

	void set_min_scale(float p_scale);
	float get_min_scale() const;

	void set_max_scale(float p_scale);
	float get_max_scale() const;

	void set_normal_align(float p_align);
	float get_normal_align() const;

	void set_random_seed(int p_seed);
	int get_random_seed() const;

	// Altitude & Slope
	void set_min_altitude(float p_alt);
	float get_min_altitude() const;

	void set_max_altitude(float p_alt);
	float get_max_altitude() const;

	void set_altitude_falloff(float p_falloff);
	float get_altitude_falloff() const;

	void set_max_slope_angle(float p_angle);
	float get_max_slope_angle() const;

	void set_slope_falloff(float p_falloff);
	float get_slope_falloff() const;

	// Density Mask
	void set_density_mask(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_density_mask() const;

	void set_density_mask_channel(DensityMaskChannel p_channel);
	DensityMaskChannel get_density_mask_channel() const;

	// Wind & Dynamics
	void set_wind_direction(const Vector2 &p_dir);
	Vector2 get_wind_direction() const;

	void set_wind_strength(float p_strength);
	float get_wind_strength() const;

	void set_wind_speed(float p_speed);
	float get_wind_speed() const;

	void set_gust_frequency(float p_freq);
	float get_gust_frequency() const;

	void set_flutter_strength(float p_flutter);
	float get_flutter_strength() const;

	// Player Interaction
	void set_player_position(const Vector3 &p_pos);
	Vector3 get_player_position() const;

	void set_player_radius(float p_radius);
	float get_player_radius() const;

	void set_trample_strength(float p_strength);
	float get_trample_strength() const;

	void set_auto_player_tracking(bool p_track);
	bool is_auto_player_tracking() const;

	// Debug & Diagnostic Queries
	void set_show_chunk_bounds(bool p_show);
	bool is_show_chunk_bounds() const;

	void rebuild_grass();
	void clear_grass();
	void update_culling_from_camera(const Vector3 &p_camera_pos);

	static HashSet<Grass3D *> active_grass_nodes;

	int get_chunk_count() const { return chunks.size(); }
	int get_total_instance_count() const;
	int get_visible_instance_count() const;

	Grass3D();
	~Grass3D();
};

VARIANT_ENUM_CAST(Grass3D::MeshType);
VARIANT_ENUM_CAST(Grass3D::DensityMaskChannel);
