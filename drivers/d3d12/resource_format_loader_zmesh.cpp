#include "resource_format_loader_zmesh.h"

#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "core/io/file_access.h"

#include "ZeGFX/include/cooked_asset_serialization.h"

// ============================================================================
// ResourceFormatLoaderZMesh — loads .zmesh cooked binary as ArrayMesh
// ============================================================================

Ref<Resource> ResourceFormatLoaderZMesh::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = OK;
	}

	if (!FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		return Ref<Resource>();
	}

	// Read the entire file into memory.
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	uint64_t file_len = f->get_length();
	PackedByteArray file_data;
	file_data.resize(file_len);
	f->get_buffer(file_data.ptrw(), file_len);
	f.unref();

	const uint8_t *data = file_data.ptr();
	uint64_t offset = 0;

	// Parse ZeCookHeader
	if (file_len < sizeof(zegfx::cooked::ZeCookHeader)) {
		ERR_PRINT(vformat("[ZeGFX] .zmesh file too small: '%s'", p_path));
		if (r_error) *r_error = ERR_FILE_CORRUPT;
		return Ref<Resource>();
	}

	const auto *header = reinterpret_cast<const zegfx::cooked::ZeCookHeader *>(data);
	if (memcmp(header->Magic, "ZMSH", 4) != 0) {
		ERR_PRINT(vformat("[ZeGFX] .zmesh bad magic in '%s'", p_path));
		if (r_error) *r_error = ERR_FILE_CORRUPT;
		return Ref<Resource>();
	}
	offset = sizeof(zegfx::cooked::ZeCookHeader);

	// Skip dependency UUIDs
	offset += header->DependencyCount * 36;

	// Parse ZeMeshMetadata
	if (offset + sizeof(zegfx::cooked::ZeMeshMetadata) > file_len) {
		ERR_PRINT(vformat("[ZeGFX] .zmesh truncated metadata in '%s'", p_path));
		if (r_error) *r_error = ERR_FILE_CORRUPT;
		return Ref<Resource>();
	}
	const auto *meta = reinterpret_cast<const zegfx::cooked::ZeMeshMetadata *>(data + offset);
	offset += sizeof(zegfx::cooked::ZeMeshMetadata);

	// Parse LOD headers
	uint32_t lod_count = meta->LodCount;
	const auto *lods = reinterpret_cast<const zegfx::cooked::ZeMeshLodHeader *>(data + offset);
	offset += lod_count * sizeof(zegfx::cooked::ZeMeshLodHeader);

	// Parse primitives
	uint32_t prim_count = meta->PrimitiveCount;
	const auto *prims = reinterpret_cast<const zegfx::cooked::ZeMeshPrimitive *>(data + offset);
	offset += prim_count * sizeof(zegfx::cooked::ZeMeshPrimitive);

	// Skip material slots
	offset += prim_count * sizeof(zegfx::cooked::ZeMeshMaterialSlot); // slots count == material count, but let's use what's there

	// Skip meshlets
	offset += meta->MeshletCount * sizeof(zegfx::cooked::ZeMeshMeshlet);

	// Now we're at the vertex payload + index payload
	uint64_t payload_offset = offset;
	uint32_t vertex_stride = meta->VertexStride;

	// Build the ArrayMesh from LOD 0 data
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->set_name(p_path.get_file().get_basename());

	if (lod_count > 0 && prim_count > 0) {
		const uint8_t *vertex_data = data + payload_offset;
		const uint8_t *index_data = vertex_data + lods[0].VertexBytes + lods[0].IndexOffset;

		// Build per-primitive surfaces for LOD 0
		for (uint32_t p = 0; p < prim_count; ++p) {
			uint64_t base_vertex = prims[p].BaseVertex;
			uint64_t start_index = prims[p].StartIndex;
			uint64_t index_count = prims[p].IndexCount;

			if (index_count == 0) continue;

			// Calculate number of vertices belonging to this primitive block
			uint64_t num_vertices = 0;
			if (p + 1 < prim_count) {
				num_vertices = prims[p + 1].BaseVertex - base_vertex;
			} else {
				num_vertices = (lods[0].VertexBytes / vertex_stride) - base_vertex;
			}

			if (num_vertices == 0) continue;

			// Extract vertices for this primitive
			PackedVector3Array positions;
			PackedVector3Array normals;
			PackedFloat32Array tangents_arr;
			PackedVector2Array uvs;
			PackedColorArray colors;

			positions.resize(num_vertices);
			normals.resize(num_vertices);
			tangents_arr.resize(num_vertices * 4);
			uvs.resize(num_vertices);
			colors.resize(num_vertices);

			for (uint64_t vi = 0; vi < num_vertices; ++vi) {
				const auto *v = reinterpret_cast<const zegfx::DX12Vertex3DTextured *>(
					vertex_data + (base_vertex + vi) * vertex_stride);

				positions.set(vi, Vector3(v->x, v->y, v->z));
				normals.set(vi, Vector3(v->nx, v->ny, v->nz));

				// Tangent (4-component: xyz + handedness w)
				tangents_arr.write[vi * 4 + 0] = v->tx;
				tangents_arr.write[vi * 4 + 1] = v->ty;
				tangents_arr.write[vi * 4 + 2] = v->tz;
				tangents_arr.write[vi * 4 + 3] = 1.0f;

				uvs.set(vi, Vector2(v->u, v->v));
				colors.set(vi, Color(v->r, v->g, v->b, v->a));
			}

			// Read indices directly (indices are local 0..num_vertices-1)
			const uint32_t *indices_ptr = reinterpret_cast<const uint32_t *>(index_data) + start_index;

			PackedInt32Array local_indices;
			local_indices.resize(index_count);
			for (uint64_t i = 0; i < index_count; ++i) {
				local_indices.set(i, indices_ptr[i]);
			}

			// Build Godot surface arrays
			Array arrays;
			arrays.resize(Mesh::ARRAY_MAX);
			arrays[Mesh::ARRAY_VERTEX] = positions;
			arrays[Mesh::ARRAY_NORMAL] = normals;
			arrays[Mesh::ARRAY_TANGENT] = tangents_arr;
			arrays[Mesh::ARRAY_TEX_UV] = uvs;
			arrays[Mesh::ARRAY_COLOR] = colors;
			arrays[Mesh::ARRAY_INDEX] = local_indices;

			mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
		}
	}

	mesh->set_meta("zegfx_zmesh_path", p_path);
	mesh->set_meta("zegfx_cooked", true);

	print_line(vformat("[ZeGFX] Loaded .zmesh: '%s' (%d primitives, %d LODs)", p_path, prim_count, lod_count));

	if (r_progress) {
		*r_progress = 1.0;
	}

	return mesh;
}

void ResourceFormatLoaderZMesh::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("zmesh");
}

bool ResourceFormatLoaderZMesh::handles_type(const String &p_type) const {
	return p_type == "ArrayMesh" || p_type == "Mesh";
}

String ResourceFormatLoaderZMesh::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "zmesh") {
		return "ArrayMesh";
	}
	return "";
}

// ============================================================================
// ResourceFormatLoaderZMat — loads .zmat cooked binary as StandardMaterial3D
// ============================================================================

Ref<Resource> ResourceFormatLoaderZMat::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = OK;
	}

	if (!FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		return Ref<Resource>();
	}

	// Read the .zmat file
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) *r_error = ERR_CANT_OPEN;
		return Ref<Resource>();
	}

	uint64_t file_len = f->get_length();
	PackedByteArray file_data;
	file_data.resize(file_len);
	f->get_buffer(file_data.ptrw(), file_len);
	f.unref();

	const uint8_t *data = file_data.ptr();

	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_name(p_path.get_file().get_basename());

	// Parse header + material metadata to extract PBR constants
	if (file_len >= sizeof(zegfx::cooked::ZeCookHeader)) {
		const auto *header = reinterpret_cast<const zegfx::cooked::ZeCookHeader *>(data);
		if (memcmp(header->Magic, "ZMAT", 4) == 0) {
			uint64_t offset = sizeof(zegfx::cooked::ZeCookHeader) + header->DependencyCount * 36;

			if (offset + sizeof(zegfx::cooked::ZeMaterialMetadata) <= file_len) {
				const auto *meta = reinterpret_cast<const zegfx::cooked::ZeMaterialMetadata *>(data + offset);
				offset += sizeof(zegfx::cooked::ZeMaterialMetadata);

				// Read constant buffer (PBR parameters)
				if (meta->ConstantBufferBytes >= 36 && offset + meta->ConstantBufferBytes <= file_len) {
					const float *cb = reinterpret_cast<const float *>(data + offset);
					// cb[0..3] = baseColorFactor RGBA
					// cb[4..6] = emissiveFactor RGB
					// cb[7] = metallicFactor
					// cb[8] = roughnessFactor
					mat->set_albedo(Color(cb[0], cb[1], cb[2], cb[3]));
					mat->set_emission(Color(cb[4], cb[5], cb[6]));
					mat->set_metallic(cb[7]);
					mat->set_roughness(cb[8]);

					if (meta->BlendMode == 2) {
						mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
					}
					if (meta->RasterizerState == 1) {
						mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
					}
				}
			}
		}
	}

	mat->set_meta("zegfx_zmat_path", p_path);
	mat->set_meta("zegfx_cooked", true);

	print_line(vformat("[ZeGFX] Loaded .zmat: '%s'", p_path));

	if (r_progress) {
		*r_progress = 1.0;
	}

	return mat;
}

void ResourceFormatLoaderZMat::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("zmat");
}

bool ResourceFormatLoaderZMat::handles_type(const String &p_type) const {
	return p_type == "StandardMaterial3D" || p_type == "Material";
}

String ResourceFormatLoaderZMat::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "zmat") {
		return "StandardMaterial3D";
	}
	return "";
}
