/**************************************************************************/
/*  foliage_3d.h                                                          */
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

#include "core/templates/local_vector.h"
#include "foliage_type_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/terrain_3d.h"
#include "scene/resources/multimesh.h"

class Foliage3D : public Node3D {
	GDCLASS(Foliage3D, Node3D);

public:
	struct FoliageInstanceData {
		Vector3 position;
		Basis rotation;
		Vector3 scale = Vector3(1, 1, 1);
		int type_index = 0;
	};

	struct FoliageChunk {
		int chunk_x = 0;
		int chunk_z = 0;
		Node3D *chunk_node = nullptr;
		Vector<MultiMeshInstance3D *> multimesh_instances; // One per FoliageType3D
		Vector<FoliageInstanceData> instances;
		AABB aabb;
		bool is_dirty = false;
	};

private:
	TypedArray<FoliageType3D> foliage_types;
	NodePath terrain_path;
	Terrain3D *cached_terrain = nullptr;

	float chunk_size = 32.0f; // in meters
	int random_seed = 1337;
	bool show_chunk_bounds = false;

	// Internal chunk management
	Node3D *chunks_container = nullptr;
	HashMap<uint64_t, FoliageChunk> chunk_map;
	bool is_dirty = false;
	bool is_transform_dirty = false;

	uint64_t _chunk_key(int p_cx, int p_cz) const {
		return ((uint64_t)(uint32_t)p_cx << 32) | (uint64_t)(uint32_t)p_cz;
	}

	void _clear_all_chunks();
	void _resolve_terrain();
	void _update_all_multimeshes();
	void _update_chunk_multimesh(FoliageChunk &r_chunk);
	FoliageChunk &_get_or_create_chunk(int p_cx, int p_cz);
	void _sample_surface(const Vector3 &p_xz_pos, Vector3 &r_pos, Vector3 &r_normal, bool &r_valid);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Foliage Types
	void set_foliage_types(const TypedArray<FoliageType3D> &p_types);
	TypedArray<FoliageType3D> get_foliage_types() const;

	int get_foliage_type_count() const;
	Ref<FoliageType3D> get_foliage_type(int p_index) const;
	void add_foliage_type(const Ref<FoliageType3D> &p_type);
	void remove_foliage_type(int p_index);

	// Terrain Linking
	void set_terrain_path(const NodePath &p_path);
	NodePath get_terrain_path() const;

	void set_chunk_size(float p_size);
	float get_chunk_size() const;

	void set_random_seed(int p_seed);
	int get_random_seed() const;

	void set_show_chunk_bounds(bool p_show);
	bool is_show_chunk_bounds() const;

	// Painting & Editing API
	int paint_instances(const Vector3 &p_world_center, float p_radius, float p_density_multiplier, const Vector<int> &p_type_filter);
	int erase_instances(const Vector3 &p_world_center, float p_radius, const Vector<int> &p_type_filter);
	void conform_to_terrain();
	void scatter_all();
	void scatter_type(int p_type_index);
	void clear_all();
	void clear_type(int p_type_index);

	// Statistics
	int get_total_instance_count() const;
	int get_type_instance_count(int p_type_index) const;
	int get_chunk_count() const;
	AABB get_total_aabb() const;

	// Serialization
	void set_foliage_data(const PackedFloat32Array &p_data);
	PackedFloat32Array get_foliage_data() const;

	void rebuild_multimeshes();

	Foliage3D();
	~Foliage3D();
};
