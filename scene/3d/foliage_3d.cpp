/**************************************************************************/
/*  foliage_3d.cpp                                                        */
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

#include "foliage_3d.h"

#include "core/config/engine.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/random_pcg.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"

void Foliage3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_foliage_types", "types"), &Foliage3D::set_foliage_types);
	ClassDB::bind_method(D_METHOD("get_foliage_types"), &Foliage3D::get_foliage_types);

	ClassDB::bind_method(D_METHOD("get_foliage_type_count"), &Foliage3D::get_foliage_type_count);
	ClassDB::bind_method(D_METHOD("get_foliage_type", "index"), &Foliage3D::get_foliage_type);
	ClassDB::bind_method(D_METHOD("add_foliage_type", "type"), &Foliage3D::add_foliage_type);
	ClassDB::bind_method(D_METHOD("remove_foliage_type", "index"), &Foliage3D::remove_foliage_type);

	ClassDB::bind_method(D_METHOD("set_terrain_path", "path"), &Foliage3D::set_terrain_path);
	ClassDB::bind_method(D_METHOD("get_terrain_path"), &Foliage3D::get_terrain_path);

	ClassDB::bind_method(D_METHOD("set_chunk_size", "size"), &Foliage3D::set_chunk_size);
	ClassDB::bind_method(D_METHOD("get_chunk_size"), &Foliage3D::get_chunk_size);

	ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &Foliage3D::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &Foliage3D::get_random_seed);

	ClassDB::bind_method(D_METHOD("set_show_chunk_bounds", "show"), &Foliage3D::set_show_chunk_bounds);
	ClassDB::bind_method(D_METHOD("is_show_chunk_bounds"), &Foliage3D::is_show_chunk_bounds);

	ClassDB::bind_method(D_METHOD("paint_instances", "world_center", "radius", "density_multiplier", "type_filter"), &Foliage3D::paint_instances);
	ClassDB::bind_method(D_METHOD("erase_instances", "world_center", "radius", "type_filter"), &Foliage3D::erase_instances);
	ClassDB::bind_method(D_METHOD("conform_to_terrain"), &Foliage3D::conform_to_terrain);
	ClassDB::bind_method(D_METHOD("scatter_all"), &Foliage3D::scatter_all);
	ClassDB::bind_method(D_METHOD("scatter_type", "type_index"), &Foliage3D::scatter_type);
	ClassDB::bind_method(D_METHOD("clear_all"), &Foliage3D::clear_all);
	ClassDB::bind_method(D_METHOD("clear_type", "type_index"), &Foliage3D::clear_type);

	ClassDB::bind_method(D_METHOD("get_total_instance_count"), &Foliage3D::get_total_instance_count);
	ClassDB::bind_method(D_METHOD("get_type_instance_count", "type_index"), &Foliage3D::get_type_instance_count);
	ClassDB::bind_method(D_METHOD("get_chunk_count"), &Foliage3D::get_chunk_count);
	ClassDB::bind_method(D_METHOD("get_total_aabb"), &Foliage3D::get_total_aabb);

	ClassDB::bind_method(D_METHOD("set_foliage_data", "data"), &Foliage3D::set_foliage_data);
	ClassDB::bind_method(D_METHOD("get_foliage_data"), &Foliage3D::get_foliage_data);
	ClassDB::bind_method(D_METHOD("rebuild_multimeshes"), &Foliage3D::rebuild_multimeshes);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "foliage_types", PROPERTY_HINT_TYPE_STRING, vformat("%d/%d:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "FoliageType3D")), "set_foliage_types", "get_foliage_types");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "terrain_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Terrain3D"), "set_terrain_path", "get_terrain_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chunk_size", PROPERTY_HINT_RANGE, "8.0,256.0,4.0,suffix:m"), "set_chunk_size", "get_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_chunk_bounds"), "set_show_chunk_bounds", "is_show_chunk_bounds");

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "foliage_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL | PROPERTY_USAGE_STORAGE), "set_foliage_data", "get_foliage_data");
}

Foliage3D::Foliage3D() {
	chunks_container = memnew(Node3D);
	chunks_container->set_name("FoliageChunks");
	add_child(chunks_container, false, INTERNAL_MODE_FRONT);
}

Foliage3D::~Foliage3D() {
	_clear_all_chunks();
}

void Foliage3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_resolve_terrain();
			_update_all_multimeshes();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (!is_transform_dirty) {
				is_transform_dirty = true;
				callable_mp(this, &Foliage3D::_update_all_multimeshes).call_deferred();
			}
		} break;
	}
}

void Foliage3D::_resolve_terrain() {
	if (!terrain_path.is_empty()) {
		cached_terrain = Object::cast_to<Terrain3D>(get_node_or_null(terrain_path));
	} else {
		// Search siblings or parent
		Node *parent = get_parent();
		if (parent) {
			for (int i = 0; i < parent->get_child_count(); ++i) {
				Terrain3D *t = Object::cast_to<Terrain3D>(parent->get_child(i));
				if (t) {
					cached_terrain = t;
					break;
				}
			}
		}
	}
}

void Foliage3D::_clear_all_chunks() {
	for (auto &kv : chunk_map) {
		if (kv.value.chunk_node && kv.value.chunk_node->get_parent()) {
			kv.value.chunk_node->queue_free();
		}
	}
	chunk_map.clear();
}

Foliage3D::FoliageChunk &Foliage3D::_get_or_create_chunk(int p_cx, int p_cz) {
	uint64_t key = _chunk_key(p_cx, p_cz);
	auto it = chunk_map.find(key);
	if (it != chunk_map.end()) {
		return it->value;
	}

	FoliageChunk chunk;
	chunk.chunk_x = p_cx;
	chunk.chunk_z = p_cz;
	chunk.chunk_node = memnew(Node3D);
	chunk.chunk_node->set_name(vformat("Chunk_%d_%d", p_cx, p_cz));
	chunks_container->add_child(chunk.chunk_node);

	chunk_map[key] = chunk;
	return chunk_map[key];
}

void Foliage3D::_sample_surface(const Vector3 &p_world_xz, Vector3 &r_pos, Vector3 &r_normal, bool &r_valid) {
	r_valid = false;
	if (cached_terrain) {
		float h = cached_terrain->sample_height(p_world_xz);
		r_pos = Vector3(p_world_xz.x, h, p_world_xz.z);
		r_normal = cached_terrain->get_normal_at(p_world_xz);
		r_valid = true;
	} else {
		r_pos = p_world_xz;
		r_normal = Vector3(0, 1, 0);
		r_valid = true;
	}
}

void Foliage3D::set_foliage_types(const TypedArray<FoliageType3D> &p_types) {
	foliage_types = p_types;
	_update_all_multimeshes();
}

TypedArray<FoliageType3D> Foliage3D::get_foliage_types() const {
	return foliage_types;
}

int Foliage3D::get_foliage_type_count() const {
	return foliage_types.size();
}

Ref<FoliageType3D> Foliage3D::get_foliage_type(int p_index) const {
	if (p_index >= 0 && p_index < foliage_types.size()) {
		return foliage_types[p_index];
	}
	return Ref<FoliageType3D>();
}

void Foliage3D::add_foliage_type(const Ref<FoliageType3D> &p_type) {
	if (p_type.is_valid()) {
		foliage_types.push_back(p_type);
		_update_all_multimeshes();
	}
}

void Foliage3D::remove_foliage_type(int p_index) {
	if (p_index >= 0 && p_index < foliage_types.size()) {
		foliage_types.remove_at(p_index);
		// Clean up instances with this type index or adjust them
		clear_type(p_index);
		_update_all_multimeshes();
	}
}

void Foliage3D::set_terrain_path(const NodePath &p_path) {
	terrain_path = p_path;
	_resolve_terrain();
}

NodePath Foliage3D::get_terrain_path() const {
	return terrain_path;
}

void Foliage3D::set_chunk_size(float p_size) {
	chunk_size = MAX(4.0f, p_size);
}

float Foliage3D::get_chunk_size() const {
	return chunk_size;
}

void Foliage3D::set_random_seed(int p_seed) {
	random_seed = p_seed;
}

int Foliage3D::get_random_seed() const {
	return random_seed;
}

void Foliage3D::set_show_chunk_bounds(bool p_show) {
	show_chunk_bounds = p_show;
}

bool Foliage3D::is_show_chunk_bounds() const {
	return show_chunk_bounds;
}

int Foliage3D::paint_instances(const Vector3 &p_world_center, float p_radius, float p_density_multiplier, const Vector<int> &p_type_filter) {
	_resolve_terrain();
	if (foliage_types.is_empty() || p_radius <= 0.01f) {
		return 0;
	}

	RandomPCG rng;
	rng.seed(Math::rand());

	int total_spawned = 0;
	Vector<int> active_types;
	if (p_type_filter.is_empty()) {
		for (int i = 0; i < foliage_types.size(); ++i) {
			Ref<FoliageType3D> ft = foliage_types[i];
			if (ft.is_valid() && ft->is_enabled()) {
				active_types.push_back(i);
			}
		}
	} else {
		for (int idx : p_type_filter) {
			if (idx >= 0 && idx < foliage_types.size()) {
				Ref<FoliageType3D> ft = foliage_types[idx];
				if (ft.is_valid() && ft->is_enabled()) {
					active_types.push_back(idx);
				}
			}
		}
	}

	if (active_types.is_empty()) return 0;

	float area = Math::PI * p_radius * p_radius;

	for (int type_idx : active_types) {
		Ref<FoliageType3D> ft = foliage_types[type_idx];
		if (!ft.is_valid() || !ft->get_mesh().is_valid()) continue;

		float count_float = (area / 100.0f) * ft->get_density() * p_density_multiplier;
		int candidate_count = (int)count_float;
		if (rng.randf() < (count_float - (float)candidate_count)) {
			candidate_count++;
		}

		for (int c = 0; c < candidate_count; ++c) {
			// Random point in circle
			float r = p_radius * Math::sqrt(rng.randf());
			float theta = rng.randf() * Math::TAU;
			Vector3 offset(r * Math::cos(theta), 0.0f, r * Math::sin(theta));
			Vector3 cand_pos = p_world_center + offset;

			Vector3 surface_pos, surface_normal;
			bool valid = false;
			_sample_surface(cand_pos, surface_pos, surface_normal, valid);
			if (!valid) continue;

			// Altitude check
			if (surface_pos.y < ft->get_min_altitude() || surface_pos.y > ft->get_max_altitude()) {
				continue;
			}

			// Slope check
			float slope_deg = Math::rad_to_deg(Math::acos(CLAMP(surface_normal.dot(Vector3(0, 1, 0)), -1.0f, 1.0f)));
			if (slope_deg > ft->get_max_slope_angle()) {
				continue;
			}

			// Find chunk
			int cx = Math::floor(surface_pos.x / chunk_size);
			int cz = Math::floor(surface_pos.z / chunk_size);
			FoliageChunk &chunk = _get_or_create_chunk(cx, cz);

			// Minimum spacing check
			float min_sp = ft->get_min_spacing();
			float min_sp_sq = min_sp * min_sp;
			bool too_close = false;
			for (const FoliageInstanceData &inst : chunk.instances) {
				if (inst.type_index == type_idx) {
					if (inst.position.distance_squared_to(surface_pos) < min_sp_sq) {
						too_close = true;
						break;
					}
				}
			}
			if (too_close) continue;

			// Generate scale
			Vector3 scale;
			if (ft->is_uniform_scale()) {
				float s = Math::lerp(ft->get_min_scale().y, ft->get_max_scale().y, rng.randf());
				scale = Vector3(s, s, s);
			} else {
				scale.x = Math::lerp(ft->get_min_scale().x, ft->get_max_scale().x, rng.randf());
				scale.y = Math::lerp(ft->get_min_scale().y, ft->get_max_scale().y, rng.randf());
				scale.z = Math::lerp(ft->get_min_scale().z, ft->get_max_scale().z, rng.randf());
			}

			// Generate rotation
			Basis rot;
			if (ft->is_random_rotation_y()) {
				rot.rotate(Vector3(0, 1, 0), rng.randf() * Math::TAU);
			}

			// Normal alignment
			if (ft->get_normal_align() > 0.001f) {
				Vector3 up = Vector3(0, 1, 0);
				Vector3 target_normal = up.lerp(surface_normal, ft->get_normal_align()).normalized();
				Vector3 axis = up.cross(target_normal);
				if (axis.length_squared() > 1e-6f) {
					float angle = Math::acos(CLAMP(up.dot(target_normal), -1.0f, 1.0f));
					rot = Basis(axis.normalized(), angle) * rot;
				}
			}

			// Random pitch / roll
			if (ft->get_random_pitch_roll() > 0.001f) {
				float max_rad = Math::deg_to_rad(ft->get_random_pitch_roll());
				float rx = (rng.randf() * 2.0f - 1.0f) * max_rad;
				float rz = (rng.randf() * 2.0f - 1.0f) * max_rad;
				rot = Basis::from_euler(Vector3(rx, 0, rz)) * rot;
			}

			// Ground offset
			Vector3 final_pos = surface_pos + Vector3(0, ft->get_ground_offset(), 0);

			FoliageInstanceData inst;
			inst.position = final_pos;
			inst.rotation = rot;
			inst.scale = scale;
			inst.type_index = type_idx;

			chunk.instances.push_back(inst);
			chunk.is_dirty = true;
			total_spawned++;
		}
	}

	if (total_spawned > 0) {
		for (auto &kv : chunk_map) {
			if (kv.value.is_dirty) {
				_update_chunk_multimesh(kv.value);
			}
		}
	}

	return total_spawned;
}

int Foliage3D::erase_instances(const Vector3 &p_world_center, float p_radius, const Vector<int> &p_type_filter) {
	if (chunk_map.is_empty() || p_radius <= 0.01f) return 0;

	float rad_sq = p_radius * p_radius;
	int erased_count = 0;

	int min_cx = Math::floor((p_world_center.x - p_radius) / chunk_size);
	int max_cx = Math::floor((p_world_center.x + p_radius) / chunk_size);
	int min_cz = Math::floor((p_world_center.z - p_radius) / chunk_size);
	int max_cz = Math::floor((p_world_center.z + p_radius) / chunk_size);

	for (int cx = min_cx; cx <= max_cx; ++cx) {
		for (int cz = min_cz; cz <= max_cz; ++cz) {
			uint64_t key = _chunk_key(cx, cz);
			auto it = chunk_map.find(key);
			if (it == chunk_map.end()) continue;

			FoliageChunk &chunk = it->value;
			Vector<FoliageInstanceData> remaining;

			for (const FoliageInstanceData &inst : chunk.instances) {
				bool match_type = p_type_filter.is_empty();
				if (!match_type) {
					for (int t : p_type_filter) {
						if (t == inst.type_index) {
							match_type = true;
							break;
						}
					}
				}

				if (match_type) {
					Vector2 diff(inst.position.x - p_world_center.x, inst.position.z - p_world_center.z);
					if (diff.length_squared() <= rad_sq) {
						erased_count++;
						chunk.is_dirty = true;
						continue; // Erase this instance
					}
				}
				remaining.push_back(inst);
			}

			if (chunk.is_dirty) {
				chunk.instances = remaining;
				_update_chunk_multimesh(chunk);
			}
		}
	}

	return erased_count;
}

void Foliage3D::conform_to_terrain() {
	_resolve_terrain();
	if (!cached_terrain) return;

	for (auto &kv : chunk_map) {
		FoliageChunk &chunk = kv.value;
		bool chunk_updated = false;

		for (FoliageInstanceData &inst : chunk.instances) {
			float h = cached_terrain->sample_height(inst.position);
			Vector3 norm = cached_terrain->get_normal_at(inst.position);

			Ref<FoliageType3D> ft;
			if (inst.type_index >= 0 && inst.type_index < foliage_types.size()) {
				ft = foliage_types[inst.type_index];
			}
			float g_offset = ft.is_valid() ? ft->get_ground_offset() : 0.0f;

			inst.position.y = h + g_offset;
			chunk_updated = true;
		}

		if (chunk_updated) {
			_update_chunk_multimesh(chunk);
		}
	}
}

void Foliage3D::scatter_all() {
	for (int i = 0; i < foliage_types.size(); ++i) {
		scatter_type(i);
	}
}

void Foliage3D::scatter_type(int p_type_index) {
	_resolve_terrain();
	if (!cached_terrain || p_type_index < 0 || p_type_index >= foliage_types.size()) return;

	Ref<FoliageType3D> ft = foliage_types[p_type_index];
	if (!ft.is_valid() || !ft->is_enabled() || !ft->get_mesh().is_valid()) return;

	AABB terrain_aabb = cached_terrain->get_total_aabb();
	Vector3 center = terrain_aabb.get_center();
	float radius = MAX(terrain_aabb.size.x, terrain_aabb.size.z) * 0.5f;

	Vector<int> filter;
	filter.push_back(p_type_index);
	paint_instances(center, radius, 1.0f, filter);
}

void Foliage3D::clear_all() {
	_clear_all_chunks();
}

void Foliage3D::clear_type(int p_type_index) {
	for (auto &kv : chunk_map) {
		FoliageChunk &chunk = kv.value;
		Vector<FoliageInstanceData> remaining;
		bool changed = false;

		for (const FoliageInstanceData &inst : chunk.instances) {
			if (inst.type_index == p_type_index) {
				changed = true;
			} else {
				remaining.push_back(inst);
			}
		}

		if (changed) {
			chunk.instances = remaining;
			_update_chunk_multimesh(chunk);
		}
	}
}

void Foliage3D::_update_chunk_multimesh(FoliageChunk &r_chunk) {
	r_chunk.is_dirty = false;
	if (!r_chunk.chunk_node) return;

	// Count instances per type
	HashMap<int, Vector<FoliageInstanceData>> grouped;
	for (const FoliageInstanceData &inst : r_chunk.instances) {
		grouped[inst.type_index].push_back(inst);
	}

	// Remove excess multimesh instances
	while (r_chunk.multimesh_instances.size() > foliage_types.size()) {
		int last_idx = r_chunk.multimesh_instances.size() - 1;
		MultiMeshInstance3D *mmi = r_chunk.multimesh_instances[last_idx];
		r_chunk.multimesh_instances.remove_at(last_idx);
		if (mmi) mmi->queue_free();
	}

	// Ensure multimesh instance per foliage type
	while (r_chunk.multimesh_instances.size() < foliage_types.size()) {
		MultiMeshInstance3D *mmi = memnew(MultiMeshInstance3D);
		r_chunk.chunk_node->add_child(mmi);
		r_chunk.multimesh_instances.push_back(mmi);
	}

	AABB total_chunk_aabb;
	bool has_aabb = false;

	for (int i = 0; i < foliage_types.size(); ++i) {
		MultiMeshInstance3D *mmi = r_chunk.multimesh_instances[i];
		Ref<FoliageType3D> ft = foliage_types[i];

		if (!ft.is_valid() || !ft->get_mesh().is_valid() || !grouped.has(i) || grouped[i].is_empty()) {
			mmi->set_multimesh(Ref<MultiMesh>());
			mmi->set_visible(false);
			continue;
		}

		const Vector<FoliageInstanceData> &inst_list = grouped[i];
		Ref<MultiMesh> mm = memnew(MultiMesh);
		mm->set_transform_format(MultiMesh::TRANSFORM_3D);
		mm->set_mesh(ft->get_mesh());
		mm->set_instance_count(inst_list.size());

		Transform3D inv_global = get_global_transform().affine_inverse();

		for (int idx = 0; idx < inst_list.size(); ++idx) {
			const FoliageInstanceData &inst = inst_list[idx];
			Transform3D t(inst.rotation.scaled(inst.scale), inst.position);
			// Localize to Foliage3D node transform
			Transform3D local_t = inv_global * t;
			mm->set_instance_transform(idx, local_t);

			AABB inst_aabb = ft->get_mesh()->get_aabb();
			inst_aabb.position += inst.position;
			if (!has_aabb) {
				total_chunk_aabb = inst_aabb;
				has_aabb = true;
			} else {
				total_chunk_aabb = total_chunk_aabb.merge(inst_aabb);
			}
		}

		mmi->set_multimesh(mm);
		mmi->set_material_override(ft->get_material_override());
		mmi->set_cast_shadows_setting(ft->get_cast_shadow());
		mmi->set_visible(true);
	}

	r_chunk.aabb = total_chunk_aabb;
}

void Foliage3D::_update_all_multimeshes() {
	for (auto &kv : chunk_map) {
		_update_chunk_multimesh(kv.value);
	}
	is_transform_dirty = false;
}

void Foliage3D::rebuild_multimeshes() {
	_update_all_multimeshes();
}

int Foliage3D::get_total_instance_count() const {
	int total = 0;
	for (const auto &kv : chunk_map) {
		total += kv.value.instances.size();
	}
	return total;
}

int Foliage3D::get_type_instance_count(int p_type_index) const {
	int count = 0;
	for (const auto &kv : chunk_map) {
		for (const FoliageInstanceData &inst : kv.value.instances) {
			if (inst.type_index == p_type_index) count++;
		}
	}
	return count;
}

int Foliage3D::get_chunk_count() const {
	return chunk_map.size();
}

AABB Foliage3D::get_total_aabb() const {
	AABB total;
	bool first = true;
	for (const auto &kv : chunk_map) {
		if (kv.value.instances.is_empty()) continue;
		if (first) {
			total = kv.value.aabb;
			first = false;
		} else {
			total = total.merge(kv.value.aabb);
		}
	}
	return total;
}

void Foliage3D::set_foliage_data(const PackedFloat32Array &p_data) {
	_clear_all_chunks();
	// Format per instance: 16 floats
	// [type_index, px, py, pz, r00, r01, r02, r10, r11, r12, r20, r21, r22, sx, sy, sz]
	int stride = 16;
	int total_instances = p_data.size() / stride;

	for (int i = 0; i < total_instances; ++i) {
		int base = i * stride;
		int type_idx = (int)p_data[base + 0];
		Vector3 pos(p_data[base + 1], p_data[base + 2], p_data[base + 3]);
		Basis rot(
			Vector3(p_data[base + 4], p_data[base + 5], p_data[base + 6]),
			Vector3(p_data[base + 7], p_data[base + 8], p_data[base + 9]),
			Vector3(p_data[base + 10], p_data[base + 11], p_data[base + 12])
		);
		Vector3 scale(p_data[base + 13], p_data[base + 14], p_data[base + 15]);

		int cx = Math::floor(pos.x / chunk_size);
		int cz = Math::floor(pos.z / chunk_size);
		FoliageChunk &chunk = _get_or_create_chunk(cx, cz);

		FoliageInstanceData inst;
		inst.type_index = type_idx;
		inst.position = pos;
		inst.rotation = rot;
		inst.scale = scale;

		chunk.instances.push_back(inst);
	}

	_update_all_multimeshes();
}

PackedFloat32Array Foliage3D::get_foliage_data() const {
	PackedFloat32Array foliage_stream;
	int total_count = get_total_instance_count();
	if (total_count == 0) return foliage_stream;

	int stride = 16;
	foliage_stream.resize(total_count * stride);
	float *w = foliage_stream.ptrw();
	int offset = 0;

	for (const auto &kv : chunk_map) {
		for (const FoliageInstanceData &inst : kv.value.instances) {
			w[offset + 0] = (float)inst.type_index;
			w[offset + 1] = inst.position.x;
			w[offset + 2] = inst.position.y;
			w[offset + 3] = inst.position.z;

			w[offset + 4] = inst.rotation[0][0];
			w[offset + 5] = inst.rotation[0][1];
			w[offset + 6] = inst.rotation[0][2];

			w[offset + 7] = inst.rotation[1][0];
			w[offset + 8] = inst.rotation[1][1];
			w[offset + 9] = inst.rotation[1][2];

			w[offset + 10] = inst.rotation[2][0];
			w[offset + 11] = inst.rotation[2][1];
			w[offset + 12] = inst.rotation[2][2];

			w[offset + 13] = inst.scale.x;
			w[offset + 14] = inst.scale.y;
			w[offset + 15] = inst.scale.z;

			offset += stride;
		}
	}

	return foliage_stream;
}
