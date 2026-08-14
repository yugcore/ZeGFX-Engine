/**************************************************************************/
/*  world_partition_3d.h                                                  */
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

#include "core/io/resource_loader.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/packed_scene.h"

class WorldPartition3D : public Node3D {
	GDCLASS(WorldPartition3D, Node3D);

public:
	struct CellEntry {
		Vector2i coord;
		Node3D *instance = nullptr;
		bool is_loading = false;
		bool is_loaded = false;
		String resource_path;
		AABB aabb;
	};

private:
	float cell_size = 256.0f;
	float loading_distance = 1000.0f;
	float unloading_distance = 1400.0f;
	String cell_path_pattern = "res://world/cells/cell_%d_%d.tscn";
	int max_concurrent_loads = 4;
	bool stream_in_editor = true;
	bool debug_draw_cells = true;
	NodePath target_node_path;

	HashMap<Vector2i, CellEntry> cells;
	Vector<Vector2i> active_loading_queue;
	Node3D *cells_container = nullptr;
	Vector3 last_streaming_pos;
	bool is_first_update = true;

	void _poll_async_loads();
	void _request_cell_load(const Vector2i &p_coord);
	void _finalize_cell_load(const Vector2i &p_coord, const Ref<PackedScene> &p_scene);
	String _get_cell_path(const Vector2i &p_coord) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	WorldPartition3D();
	~WorldPartition3D();

	void set_cell_size(float p_size);
	float get_cell_size() const;

	void set_loading_distance(float p_dist);
	float get_loading_distance() const;

	void set_unloading_distance(float p_dist);
	float get_unloading_distance() const;

	void set_cell_path_pattern(const String &p_pattern);
	String get_cell_path_pattern() const;

	void set_max_concurrent_loads(int p_max);
	int get_max_concurrent_loads() const;

	void set_stream_in_editor(bool p_enabled);
	bool is_stream_in_editor() const;

	void set_debug_draw_cells(bool p_enabled);
	bool is_debug_draw_cells() const;

	void set_target_node_path(const NodePath &p_path);
	NodePath get_target_node_path() const;

	// Runtime API
	void update_streaming(const Vector3 &p_target_pos);
	void force_refresh();

	void load_cell(int p_x, int p_z);
	void unload_cell(int p_x, int p_z);
	void load_all_cells();
	void unload_all_cells();

	bool is_cell_loaded(int p_x, int p_z) const;
	bool is_cell_loading(int p_x, int p_z) const;

	TypedArray<Vector2i> get_loaded_cells() const;
	TypedArray<Vector2i> get_loading_cells() const;
	int get_loaded_cell_count() const;
	int get_loading_cell_count() const;

	Vector2i world_to_cell_coord(const Vector3 &p_world_pos) const;
	Vector3 cell_coord_to_world(int p_x, int p_z) const;
	AABB get_cell_aabb(int p_x, int p_z) const;

	const HashMap<Vector2i, CellEntry> &get_all_cells() const { return cells; }
};
