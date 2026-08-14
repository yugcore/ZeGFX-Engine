/**************************************************************************/
/*  world_partition_3d.cpp                                                */
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

#include "world_partition_3d.h"

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"

WorldPartition3D::WorldPartition3D() {
	cells_container = memnew(Node3D);
	cells_container->set_name("CellsContainer");
	add_child(cells_container, false, INTERNAL_MODE_FRONT);
}

WorldPartition3D::~WorldPartition3D() {
	unload_all_cells();
}

void WorldPartition3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_internal(true);
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			_poll_async_loads();

			if (Engine::get_singleton()->is_editor_hint() && !stream_in_editor) {
				return;
			}

			Vector3 stream_center;
			bool has_pos = false;

			if (!target_node_path.is_empty()) {
				Node3D *target = Object::cast_to<Node3D>(get_node_or_null(target_node_path));
				if (target) {
					stream_center = target->get_global_position();
					has_pos = true;
				}
			}

			if (!has_pos) {
				Viewport *vp = get_viewport();
				if (vp) {
					Camera3D *cam = vp->get_camera_3d();
					if (cam) {
						stream_center = cam->get_global_position();
						has_pos = true;
					}
				}
			}

			if (has_pos) {
				if (is_first_update || stream_center.distance_squared_to(last_streaming_pos) > (cell_size * cell_size * 0.05f)) {
					update_streaming(stream_center);
					last_streaming_pos = stream_center;
					is_first_update = false;
				}
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			set_process_internal(false);
		} break;
	}
}

String WorldPartition3D::_get_cell_path(const Vector2i &p_coord) const {
	return vformat(cell_path_pattern, p_coord.x, p_coord.y);
}

void WorldPartition3D::_poll_async_loads() {
	for (int i = active_loading_queue.size() - 1; i >= 0; --i) {
		Vector2i coord = active_loading_queue[i];
		if (!cells.has(coord)) {
			active_loading_queue.remove_at(i);
			continue;
		}

		String path = cells[coord].resource_path;
		List<String> sub_res;
		float progress = 0.0f;
		ResourceLoader::ThreadLoadStatus status = ResourceLoader::load_threaded_get_status(path, &progress);

		if (status == ResourceLoader::THREAD_LOAD_LOADED) {
			Ref<PackedScene> scene = ResourceLoader::load_threaded_get(path);
			_finalize_cell_load(coord, scene);
			active_loading_queue.remove_at(i);
		} else if (status == ResourceLoader::THREAD_LOAD_FAILED || status == ResourceLoader::THREAD_LOAD_INVALID_RESOURCE) {
			cells[coord].is_loading = false;
			cells[coord].is_loaded = false;
			emit_signal("cell_load_failed", coord, vformat("Failed to load scene at %s", path));
			active_loading_queue.remove_at(i);
		}
	}
}

void WorldPartition3D::_request_cell_load(const Vector2i &p_coord) {
	if (cells.has(p_coord) && (cells[p_coord].is_loaded || cells[p_coord].is_loading)) {
		return;
	}

	if (active_loading_queue.size() >= max_concurrent_loads) {
		return;
	}

	String path = _get_cell_path(p_coord);
	if (!ResourceLoader::exists(path)) {
		// Cell file does not exist on disk, create empty registered entry
		CellEntry entry;
		entry.coord = p_coord;
		entry.is_loading = false;
		entry.is_loaded = true;
		entry.resource_path = path;
		entry.aabb = get_cell_aabb(p_coord.x, p_coord.y);
		cells[p_coord] = entry;
		return;
	}

	Error err = ResourceLoader::load_threaded_request(path, "PackedScene");
	if (err == OK) {
		CellEntry entry;
		entry.coord = p_coord;
		entry.is_loading = true;
		entry.is_loaded = false;
		entry.resource_path = path;
		entry.aabb = get_cell_aabb(p_coord.x, p_coord.y);
		cells[p_coord] = entry;
		active_loading_queue.push_back(p_coord);
	}
}

void WorldPartition3D::_finalize_cell_load(const Vector2i &p_coord, const Ref<PackedScene> &p_scene) {
	if (!cells.has(p_coord)) return;

	if (p_scene.is_valid()) {
		Node *inst = p_scene->instantiate();
		Node3D *inst_3d = Object::cast_to<Node3D>(inst);
		if (inst_3d) {
			inst_3d->set_position(cell_coord_to_world(p_coord.x, p_coord.y));
			cells_container->add_child(inst_3d);
			cells[p_coord].instance = inst_3d;
		} else if (inst) {
			cells_container->add_child(inst);
		}
	}

	cells[p_coord].is_loaded = true;
	cells[p_coord].is_loading = false;
	emit_signal("cell_loaded", p_coord, cells[p_coord].instance);
}

void WorldPartition3D::update_streaming(const Vector3 &p_target_pos) {
	if (cell_size <= 0.0f || loading_distance <= 0.0f) return;

	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local_pos = xform.affine_inverse().xform(p_target_pos);

	Vector2i center_coord = world_to_cell_coord(p_target_pos);
	int r_cells = (int)Math::ceil(loading_distance / cell_size);

	float load_dist_sq = loading_distance * loading_distance;
	float unload_dist_sq = unloading_distance * unloading_distance;

	// 1. Request loading for cells in range
	for (int cz = center_coord.y - r_cells; cz <= center_coord.y + r_cells; ++cz) {
		for (int cx = center_coord.x - r_cells; cx <= center_coord.x + r_cells; ++cx) {
			Vector3 cell_center = cell_coord_to_world(cx, cz) + Vector3(cell_size * 0.5f, 0.0f, cell_size * 0.5f);
			Vector3 cell_local = xform.affine_inverse().xform(cell_center);
			float d2 = (Vector2(cell_local.x, cell_local.z) - Vector2(local_pos.x, local_pos.z)).length_squared();

			if (d2 <= load_dist_sq) {
				Vector2i coord(cx, cz);
				if (!cells.has(coord) || (!cells[coord].is_loaded && !cells[coord].is_loading)) {
					_request_cell_load(coord);
				}
			}
		}
	}

	// 2. Unload cells beyond unloading_distance
	Vector<Vector2i> to_unload;
	for (const KeyValue<Vector2i, CellEntry> &E : cells) {
		Vector3 cell_center = cell_coord_to_world(E.key.x, E.key.y) + Vector3(cell_size * 0.5f, 0.0f, cell_size * 0.5f);
		Vector3 cell_local = xform.affine_inverse().xform(cell_center);
		float d2 = (Vector2(cell_local.x, cell_local.z) - Vector2(local_pos.x, local_pos.z)).length_squared();

		if (d2 > unload_dist_sq) {
			to_unload.push_back(E.key);
		}
	}

	for (int i = 0; i < to_unload.size(); ++i) {
		unload_cell(to_unload[i].x, to_unload[i].y);
	}
}

void WorldPartition3D::force_refresh() {
	is_first_update = true;
}

void WorldPartition3D::load_cell(int p_x, int p_z) {
	_request_cell_load(Vector2i(p_x, p_z));
}

void WorldPartition3D::unload_cell(int p_x, int p_z) {
	Vector2i coord(p_x, p_z);
	if (!cells.has(coord)) return;

	if (cells[coord].instance) {
		cells[coord].instance->queue_free();
		cells[coord].instance = nullptr;
	}

	cells.erase(coord);

	for (int i = 0; i < active_loading_queue.size(); ++i) {
		if (active_loading_queue[i] == coord) {
			active_loading_queue.remove_at(i);
			break;
		}
	}

	emit_signal("cell_unloaded", coord);
}

void WorldPartition3D::load_all_cells() {
	Vector2i center = world_to_cell_coord(get_global_position());
	int r_cells = (int)Math::ceil(loading_distance / cell_size);

	for (int cz = center.y - r_cells; cz <= center.y + r_cells; ++cz) {
		for (int cx = center.x - r_cells; cx <= center.x + r_cells; ++cx) {
			load_cell(cx, cz);
		}
	}
}

void WorldPartition3D::unload_all_cells() {
	Vector<Vector2i> keys;
	for (const KeyValue<Vector2i, CellEntry> &E : cells) {
		keys.push_back(E.key);
	}
	for (int i = 0; i < keys.size(); ++i) {
		unload_cell(keys[i].x, keys[i].y);
	}
}

bool WorldPartition3D::is_cell_loaded(int p_x, int p_z) const {
	Vector2i c(p_x, p_z);
	return cells.has(c) && cells[c].is_loaded;
}

bool WorldPartition3D::is_cell_loading(int p_x, int p_z) const {
	Vector2i c(p_x, p_z);
	return cells.has(c) && cells[c].is_loading;
}

TypedArray<Vector2i> WorldPartition3D::get_loaded_cells() const {
	TypedArray<Vector2i> arr;
	for (const KeyValue<Vector2i, CellEntry> &E : cells) {
		if (E.value.is_loaded) {
			arr.push_back(E.key);
		}
	}
	return arr;
}

TypedArray<Vector2i> WorldPartition3D::get_loading_cells() const {
	TypedArray<Vector2i> arr;
	for (int i = 0; i < active_loading_queue.size(); ++i) {
		arr.push_back(active_loading_queue[i]);
	}
	return arr;
}

int WorldPartition3D::get_loaded_cell_count() const {
	int count = 0;
	for (const KeyValue<Vector2i, CellEntry> &E : cells) {
		if (E.value.is_loaded) count++;
	}
	return count;
}

int WorldPartition3D::get_loading_cell_count() const {
	return active_loading_queue.size();
}

Vector2i WorldPartition3D::world_to_cell_coord(const Vector3 &p_world_pos) const {
	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local = xform.affine_inverse().xform(p_world_pos);
	int cx = (int)Math::floor(local.x / cell_size);
	int cz = (int)Math::floor(local.z / cell_size);
	return Vector2i(cx, cz);
}

Vector3 WorldPartition3D::cell_coord_to_world(int p_x, int p_z) const {
	Transform3D xform = is_inside_tree() ? get_global_transform() : get_transform();
	Vector3 local(p_x * cell_size, 0.0f, p_z * cell_size);
	return xform.xform(local);
}

AABB WorldPartition3D::get_cell_aabb(int p_x, int p_z) const {
	Vector3 pos = cell_coord_to_world(p_x, p_z);
	return AABB(pos, Vector3(cell_size, 200.0f, cell_size));
}

void WorldPartition3D::set_cell_size(float p_size) {
	if (cell_size != p_size && p_size > 1.0f) {
		cell_size = p_size;
		force_refresh();
	}
}

float WorldPartition3D::get_cell_size() const {
	return cell_size;
}

void WorldPartition3D::set_loading_distance(float p_dist) {
	if (loading_distance != p_dist && p_dist > 1.0f) {
		loading_distance = p_dist;
		if (unloading_distance <= loading_distance) {
			unloading_distance = loading_distance * 1.3f;
		}
		force_refresh();
	}
}

float WorldPartition3D::get_loading_distance() const {
	return loading_distance;
}

void WorldPartition3D::set_unloading_distance(float p_dist) {
	if (unloading_distance != p_dist && p_dist >= loading_distance) {
		unloading_distance = p_dist;
		force_refresh();
	}
}

float WorldPartition3D::get_unloading_distance() const {
	return unloading_distance;
}

void WorldPartition3D::set_cell_path_pattern(const String &p_pattern) {
	cell_path_pattern = p_pattern;
}

String WorldPartition3D::get_cell_path_pattern() const {
	return cell_path_pattern;
}

void WorldPartition3D::set_max_concurrent_loads(int p_max) {
	max_concurrent_loads = CLAMP(p_max, 1, 32);
}

int WorldPartition3D::get_max_concurrent_loads() const {
	return max_concurrent_loads;
}

void WorldPartition3D::set_stream_in_editor(bool p_enabled) {
	stream_in_editor = p_enabled;
}

bool WorldPartition3D::is_stream_in_editor() const {
	return stream_in_editor;
}

void WorldPartition3D::set_debug_draw_cells(bool p_enabled) {
	debug_draw_cells = p_enabled;
}

bool WorldPartition3D::is_debug_draw_cells() const {
	return debug_draw_cells;
}

void WorldPartition3D::set_target_node_path(const NodePath &p_path) {
	target_node_path = p_path;
}

NodePath WorldPartition3D::get_target_node_path() const {
	return target_node_path;
}

void WorldPartition3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &WorldPartition3D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &WorldPartition3D::get_cell_size);

	ClassDB::bind_method(D_METHOD("set_loading_distance", "distance"), &WorldPartition3D::set_loading_distance);
	ClassDB::bind_method(D_METHOD("get_loading_distance"), &WorldPartition3D::get_loading_distance);

	ClassDB::bind_method(D_METHOD("set_unloading_distance", "distance"), &WorldPartition3D::set_unloading_distance);
	ClassDB::bind_method(D_METHOD("get_unloading_distance"), &WorldPartition3D::get_unloading_distance);

	ClassDB::bind_method(D_METHOD("set_cell_path_pattern", "pattern"), &WorldPartition3D::set_cell_path_pattern);
	ClassDB::bind_method(D_METHOD("get_cell_path_pattern"), &WorldPartition3D::get_cell_path_pattern);

	ClassDB::bind_method(D_METHOD("set_max_concurrent_loads", "max_loads"), &WorldPartition3D::set_max_concurrent_loads);
	ClassDB::bind_method(D_METHOD("get_max_concurrent_loads"), &WorldPartition3D::get_max_concurrent_loads);

	ClassDB::bind_method(D_METHOD("set_stream_in_editor", "enabled"), &WorldPartition3D::set_stream_in_editor);
	ClassDB::bind_method(D_METHOD("is_stream_in_editor"), &WorldPartition3D::is_stream_in_editor);

	ClassDB::bind_method(D_METHOD("set_debug_draw_cells", "enabled"), &WorldPartition3D::set_debug_draw_cells);
	ClassDB::bind_method(D_METHOD("is_debug_draw_cells"), &WorldPartition3D::is_debug_draw_cells);

	ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &WorldPartition3D::set_target_node_path);
	ClassDB::bind_method(D_METHOD("get_target_node_path"), &WorldPartition3D::get_target_node_path);

	ClassDB::bind_method(D_METHOD("update_streaming", "target_position"), &WorldPartition3D::update_streaming);
	ClassDB::bind_method(D_METHOD("force_refresh"), &WorldPartition3D::force_refresh);

	ClassDB::bind_method(D_METHOD("load_cell", "x", "z"), &WorldPartition3D::load_cell);
	ClassDB::bind_method(D_METHOD("unload_cell", "x", "z"), &WorldPartition3D::unload_cell);
	ClassDB::bind_method(D_METHOD("load_all_cells"), &WorldPartition3D::load_all_cells);
	ClassDB::bind_method(D_METHOD("unload_all_cells"), &WorldPartition3D::unload_all_cells);

	ClassDB::bind_method(D_METHOD("is_cell_loaded", "x", "z"), &WorldPartition3D::is_cell_loaded);
	ClassDB::bind_method(D_METHOD("is_cell_loading", "x", "z"), &WorldPartition3D::is_cell_loading);

	ClassDB::bind_method(D_METHOD("get_loaded_cells"), &WorldPartition3D::get_loaded_cells);
	ClassDB::bind_method(D_METHOD("get_loading_cells"), &WorldPartition3D::get_loading_cells);
	ClassDB::bind_method(D_METHOD("get_loaded_cell_count"), &WorldPartition3D::get_loaded_cell_count);
	ClassDB::bind_method(D_METHOD("get_loading_cell_count"), &WorldPartition3D::get_loading_cell_count);

	ClassDB::bind_method(D_METHOD("world_to_cell_coord", "world_position"), &WorldPartition3D::world_to_cell_coord);
	ClassDB::bind_method(D_METHOD("cell_coord_to_world", "x", "z"), &WorldPartition3D::cell_coord_to_world);
	ClassDB::bind_method(D_METHOD("get_cell_aabb", "x", "z"), &WorldPartition3D::get_cell_aabb);

	ADD_GROUP("Grid Settings", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size", PROPERTY_HINT_RANGE, "16.0,4096.0,1.0,suffix:m"), "set_cell_size", "get_cell_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loading_distance", PROPERTY_HINT_RANGE, "50.0,10000.0,10.0,suffix:m"), "set_loading_distance", "get_loading_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unloading_distance", PROPERTY_HINT_RANGE, "50.0,15000.0,10.0,suffix:m"), "set_unloading_distance", "get_unloading_distance");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "cell_path_pattern"), "set_cell_path_pattern", "get_cell_path_pattern");

	ADD_GROUP("Streaming & Performance", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_concurrent_loads", PROPERTY_HINT_RANGE, "1,16,1"), "set_max_concurrent_loads", "get_max_concurrent_loads");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stream_in_editor"), "set_stream_in_editor", "is_stream_in_editor");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_draw_cells"), "set_debug_draw_cells", "is_debug_draw_cells");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");

	ADD_SIGNAL(MethodInfo("cell_loaded", PropertyInfo(Variant::VECTOR2I, "cell_coord"), PropertyInfo(Variant::OBJECT, "cell_root", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
	ADD_SIGNAL(MethodInfo("cell_unloaded", PropertyInfo(Variant::VECTOR2I, "cell_coord")));
	ADD_SIGNAL(MethodInfo("cell_load_failed", PropertyInfo(Variant::VECTOR2I, "cell_coord"), PropertyInfo(Variant::STRING, "error")));
}
