#include "resource_format_loader_zmesh.h"

#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"
#include "scene/resources/image_texture.h"
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "zegfx_d3d12_bridge.h"

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
	if (header->Version < zegfx::cooked::ZE_COOK_FORMAT_VERSION_MIN || header->Version > zegfx::cooked::ZE_MESH_VERSION_CURRENT) {
		ERR_PRINT(vformat("[ZeGFX] .zmesh unsupported version %d in '%s' (expected <= %d)", header->Version, p_path, zegfx::cooked::ZE_MESH_VERSION_CURRENT));
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

	// Parse material slots (count may differ from prim_count)
	uint32_t mat_slot_count = meta->MaterialSlotCount;
	// Backwards compat: if MaterialSlotCount is 0 (old files where it was Pad=0), fall back to prim_count
	if (mat_slot_count == 0) {
		mat_slot_count = prim_count;
	}
	const auto *slots = reinterpret_cast<const zegfx::cooked::ZeMeshMaterialSlot *>(data + offset);
	offset += mat_slot_count * sizeof(zegfx::cooked::ZeMeshMaterialSlot);
	print_line(vformat("[ZeGFX ZMESH] prim_count=%d, mat_slot_count=%d", prim_count, mat_slot_count));

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
			PackedVector2Array uvs2;
			PackedColorArray colors;

			positions.resize(num_vertices);
			normals.resize(num_vertices);
			tangents_arr.resize(num_vertices * 4);
			uvs.resize(num_vertices);
			uvs2.resize(num_vertices);
			colors.resize(num_vertices);

			bool has_custom_colors = false;
			for (uint64_t vi = 0; vi < num_vertices; ++vi) {
				const auto *v = reinterpret_cast<const zegfx::DX12Vertex3DTextured *>(
					vertex_data + (base_vertex + vi) * vertex_stride);

				positions.set(vi, Vector3(v->x, v->y, v->z));

				float nx = 0.0f, ny = 1.0f, nz = 0.0f;
				zegfx::decodeOctahedralSNORM16(v->octNormal, nx, ny, nz);
				Vector3 norm(nx, ny, nz);
				if (norm.length_squared() < 0.001f) {
					norm = Vector3(0, 1, 0);
				} else {
					norm.normalize();
				}
				normals.set(vi, norm);

				// Tangent (4-component: xyz + handedness w)
				float tx = 1.0f, ty = 0.0f, tz = 0.0f;
				zegfx::decodeOctahedralSNORM16(v->octTangent, tx, ty, tz);
				Vector3 tan(tx, ty, tz);

				// Orthonormalize tangent against normal (Gram-Schmidt)
				tan = tan - norm * norm.dot(tan);
				if (tan.length_squared() < 0.001f) {
					// Compute valid perpendicular tangent vector
					Vector3 up = Math::abs(norm.y) < 0.999f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
					tan = norm.cross(up).normalized();
				} else {
					tan.normalize();
				}

				tangents_arr.write[vi * 4 + 0] = tan.x;
				tangents_arr.write[vi * 4 + 1] = tan.y;
				tangents_arr.write[vi * 4 + 2] = tan.z;
				tangents_arr.write[vi * 4 + 3] = 1.0f;

				float u0 = zegfx::halfToFloat(v->u);
				float v0 = zegfx::halfToFloat(v->v);
				float u1 = zegfx::halfToFloat(v->u1);
				float v1 = zegfx::halfToFloat(v->v1);

				bool is_dummy_0 = (u0 == 0.0f && v0 == 0.0f) || (u0 == 0.0f && v0 == 1.0f);
				bool is_valid_1 = (u1 != 0.0f || v1 != 0.0f) && !(u1 == 0.0f && v1 == 1.0f);

				if (is_dummy_0 && is_valid_1) {
					uvs.set(vi, Vector2(u1, v1));
					uvs2.set(vi, Vector2(u0, v0));
				} else {
					uvs.set(vi, Vector2(u0, v0));
					uvs2.set(vi, Vector2(u1, v1));
				}

				uint32_t c = v->colorRgba;
				if (c != 0 && c != 0xFFFFFFFF) {
					has_custom_colors = true;
				}
				float r = (c == 0) ? 1.0f : static_cast<float>((c) & 0xFF) / 255.0f;
				float g = (c == 0) ? 1.0f : static_cast<float>((c >> 8) & 0xFF) / 255.0f;
				float b = (c == 0) ? 1.0f : static_cast<float>((c >> 16) & 0xFF) / 255.0f;
				float a = (c == 0) ? 1.0f : static_cast<float>((c >> 24) & 0xFF) / 255.0f;
				colors.set(vi, Color(r, g, b, a));
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
			arrays[Mesh::ARRAY_TEX_UV2] = uvs2;
			if (has_custom_colors) {
				arrays[Mesh::ARRAY_COLOR] = colors;
			}
			arrays[Mesh::ARRAY_INDEX] = local_indices;

			mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

			// Automatically resolve and bind companion .zmat material
			int32_t slot_idx = prims[p].MaterialSlotIndex;
			String slot_name;
			if (slot_idx >= 0 && slot_idx < (int32_t)mat_slot_count) {
				const char *raw = slots[slot_idx].SlotName;
				int slen = 0;
				while (slen < 64 && raw[slen] != '\0') slen++;
				if (slen > 0) {
					slot_name = String::utf8(raw, slen).strip_edges();
				}
			}

			String base_dir = p_path.get_base_dir();
			String mesh_base = p_path.get_file().get_basename();
			Ref<Material> mat;

			Vector<String> zmat_search_paths;
			if (!slot_name.is_empty()) {
				zmat_search_paths.push_back(base_dir.path_join(slot_name + ".zmat"));
				zmat_search_paths.push_back(base_dir.path_join("Materials").path_join(slot_name + ".zmat"));
			}
			if (slot_idx >= 0) {
				zmat_search_paths.push_back(base_dir.path_join(vformat("Material_%d.zmat", slot_idx)));
				zmat_search_paths.push_back(base_dir.path_join("Materials").path_join(vformat("Material_%d.zmat", slot_idx)));
			}
			zmat_search_paths.push_back(base_dir.path_join(mesh_base + ".zmat"));
			zmat_search_paths.push_back(base_dir.path_join("Materials").path_join(mesh_base + ".zmat"));
			zmat_search_paths.push_back(base_dir.path_join("Material_0.zmat"));

			for (int i = 0; i < zmat_search_paths.size(); ++i) {
				if (FileAccess::exists(zmat_search_paths[i])) {
					mat = ResourceLoader::load(zmat_search_paths[i]);
					if (mat.is_valid()) {
						print_line(vformat("[ZeGFX ZMESH] Found zmat via direct path: '%s'", zmat_search_paths[i]));
						break;
					}
				}
			}

			if (mat.is_null()) {
				// Scan base_dir and Materials subfolder
				Vector<String> scan_dirs;
				scan_dirs.push_back(base_dir);
				scan_dirs.push_back(base_dir.path_join("Materials"));
				for (int d = 0; d < scan_dirs.size(); ++d) {
					Ref<DirAccess> da = DirAccess::open(scan_dirs[d]);
					if (da.is_null()) continue;
					da->list_dir_begin();
					String f = da->get_next();
					String first_zmat;
					while (!f.is_empty()) {
						if (!da->current_is_dir() && f.get_extension().to_lower() == "zmat") {
							if (first_zmat.is_empty()) first_zmat = scan_dirs[d].path_join(f);
							String fn = f.get_basename().to_lower();
							String sn = slot_name.to_lower();
							String mb = mesh_base.to_lower();
							if ((!sn.is_empty() && (fn.contains(sn) || sn.contains(fn))) ||
								(!mb.is_empty() && (fn.contains(mb) || mb.contains(fn)))) {
								mat = ResourceLoader::load(scan_dirs[d].path_join(f));
								if (mat.is_valid()) break;
							}
						}
						f = da->get_next();
					}
					if (mat.is_valid()) break;
					if (mat.is_null() && !first_zmat.is_empty()) {
						mat = ResourceLoader::load(first_zmat);
						if (mat.is_valid()) break;
					}
				}
			}

			if (mat.is_valid()) {
				mesh->surface_set_material(p, mat);
				print_line(vformat("[ZeGFX ZMESH] Bound material '%s' to surface %d", slot_name.is_empty() ? mesh_base : slot_name, p));
			}
		}
	}

	mesh->set_meta("zegfx_zmesh_path", p_path);
	mesh->set_meta("zegfx_cooked", true);
	mesh->set_meta("zegfx_meshlet_count", (int64_t)meta->MeshletCount);
	mesh->set_meta("zegfx_lod_count", (int64_t)lod_count);
	mesh->set_meta("zegfx_primitive_count", (int64_t)prim_count);

	if (ZeGFXD3D12Bridge::get_singleton()) {
		ZeGFXD3D12Bridge::get_singleton()->register_zmesh_metadata(p_path, meta->MeshletCount, lod_count, prim_count, meta->VertexStride);
	}

	print_line(vformat("[ZeGFX] Loaded .zmesh: '%s' (%d primitives, %d meshlets, %d LODs)", p_path, prim_count, meta->MeshletCount, lod_count));

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

static Ref<Texture2D> _load_zetex_file(const String &p_path) {
	String global_p = ProjectSettings::get_singleton()->globalize_path(p_path);
	if (!FileAccess::exists(global_p)) {
		return Ref<Texture2D>();
	}

	Ref<FileAccess> f = FileAccess::open(global_p, FileAccess::READ);
	if (f.is_null()) return Ref<Texture2D>();

	uint64_t file_len = f->get_length();
	if (file_len < sizeof(zegfx::cooked::ZeCookHeader) + sizeof(zegfx::cooked::ZeTextureMetadata)) return Ref<Texture2D>();

	PackedByteArray bytes;
	bytes.resize(file_len);
	f->get_buffer(bytes.ptrw(), file_len);
	f.unref();

	const uint8_t *data = bytes.ptr();
	const auto *header = reinterpret_cast<const zegfx::cooked::ZeCookHeader *>(data);
	if (memcmp(header->Magic, "ZTEX", 4) != 0) return Ref<Texture2D>();

	uint64_t offset = header->HeaderSize;
	if (offset + sizeof(zegfx::cooked::ZeTextureMetadata) > file_len) return Ref<Texture2D>();

	const auto *meta = reinterpret_cast<const zegfx::cooked::ZeTextureMetadata *>(data + offset);
	offset += sizeof(zegfx::cooked::ZeTextureMetadata);

	uint32_t mip_count = meta->MipCount;
	offset += mip_count * sizeof(zegfx::cooked::ZeTextureMipInfo);

	if (offset >= file_len) return Ref<Texture2D>();

	Image::Format img_format = Image::FORMAT_RGBA8;
	switch (static_cast<zegfx::TextureStorageFormat>(meta->StorageFormat)) {
		case zegfx::TextureStorageFormat::RawRGBA8: img_format = Image::FORMAT_RGBA8; break;
		case zegfx::TextureStorageFormat::BC1: img_format = Image::FORMAT_DXT1; break;
		case zegfx::TextureStorageFormat::BC3: img_format = Image::FORMAT_DXT5; break;
		case zegfx::TextureStorageFormat::BC5: img_format = Image::FORMAT_RGTC_RG; break;
		case zegfx::TextureStorageFormat::BC7: img_format = Image::FORMAT_BPTC_RGBA; break;
		case zegfx::TextureStorageFormat::ASTC_4x4: img_format = Image::FORMAT_ASTC_4x4; break;
		case zegfx::TextureStorageFormat::ASTC_6x6: img_format = Image::FORMAT_ASTC_6x6; break;
		case zegfx::TextureStorageFormat::ETC2_RGB: img_format = Image::FORMAT_ETC2_RGB8; break;
		case zegfx::TextureStorageFormat::ETC2_RGBA: img_format = Image::FORMAT_ETC2_RGBA8; break;
		default: img_format = Image::FORMAT_RGBA8; break;
	}

	int payload_size = static_cast<int>(file_len - offset);
	Vector<uint8_t> mip_data;
	mip_data.resize(payload_size);
	memcpy(mip_data.ptrw(), data + offset, payload_size);

	Ref<Image> img = Image::create_from_data(meta->Width, meta->Height, mip_count > 1, img_format, mip_data);
	if (img.is_valid() && !img->is_empty()) {
		return ImageTexture::create_from_image(img);
	}
	return Ref<Texture2D>();
}

static Ref<Texture2D> _load_texture_robust(const String &p_path) {
	String ext = p_path.get_extension().to_lower();
	if (ext == "ztex" || ext == "zetex") {
		Ref<Texture2D> ztex = _load_zetex_file(p_path);
		if (ztex.is_valid()) return ztex;
	}

	if (ResourceLoader::exists(p_path)) {
		Ref<Resource> res = ResourceLoader::load(p_path);
		if (res.is_valid()) {
			Ref<Texture2D> tex = res;
			if (tex.is_valid()) return tex;
		}
	}
	String global_p = ProjectSettings::get_singleton()->globalize_path(p_path);
	String global_ext = global_p.get_extension().to_lower();
	if (global_ext == "ztex" || global_ext == "zetex") {
		Ref<Texture2D> ztex = _load_zetex_file(global_p);
		if (ztex.is_valid()) return ztex;
	}

	// Also check if companion .ztex exists with same base name
	String ztex_alt = global_p.get_basename() + ".ztex";
	if (FileAccess::exists(ztex_alt)) {
		Ref<Texture2D> ztex = _load_zetex_file(ztex_alt);
		if (ztex.is_valid()) return ztex;
	}

	if (FileAccess::exists(global_p)) {
		Ref<Image> img;
		img.instantiate();
		if (img->load(global_p) == OK && !img->is_empty()) {
			if (img->get_mipmap_count() == 0 && img->get_width() > 1 && img->get_height() > 1) {
				img->generate_mipmaps();
			}
			return ImageTexture::create_from_image(img);
		}
	}
	return Ref<Texture2D>();
}

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
	Vector<String> search_dirs;
	String local_dir = p_path.get_base_dir();
	search_dirs.push_back(local_dir);
	search_dirs.push_back(local_dir.path_join("Textures"));
	search_dirs.push_back(local_dir.path_join("textures"));
	String parent_dir = local_dir.get_base_dir();
	if (parent_dir != local_dir && !search_dirs.has(parent_dir)) {
		search_dirs.push_back(parent_dir);
		search_dirs.push_back(parent_dir.path_join("Textures"));
		search_dirs.push_back(parent_dir.path_join("textures"));
	}
	if (!search_dirs.has("res://")) {
		search_dirs.push_back("res://");
	}
	if (DirAccess::dir_exists_absolute("res://assets")) {
		search_dirs.push_back("res://assets");
		Ref<DirAccess> da_assets = DirAccess::open("res://assets");
		if (da_assets.is_valid()) {
			da_assets->list_dir_begin();
			String adir = da_assets->get_next();
			while (!adir.is_empty()) {
				if (da_assets->current_is_dir() && !adir.begins_with(".")) {
					search_dirs.push_back("res://assets/" + adir);
					search_dirs.push_back("res://assets/" + adir + "/Textures");
					search_dirs.push_back("res://assets/" + adir + "/textures");
				}
				adir = da_assets->get_next();
			}
			da_assets->list_dir_end();
		}
	}

	Ref<Texture2D> albedo_tex;
	Ref<Texture2D> normal_tex;
	Ref<Texture2D> rough_tex;
	Ref<Texture2D> metal_tex;
	Ref<Texture2D> emit_tex;
	Ref<Texture2D> opacity_tex;
	Ref<Texture2D> ao_tex;

	if (file_len >= sizeof(zegfx::cooked::ZeCookHeader)) {
		const auto *header = reinterpret_cast<const zegfx::cooked::ZeCookHeader *>(data);
		if (memcmp(header->Magic, "ZMAT", 4) == 0) {
			if (header->Version < zegfx::cooked::ZE_COOK_FORMAT_VERSION_MIN || header->Version > zegfx::cooked::ZE_MAT_VERSION_CURRENT) {
				ERR_PRINT(vformat("[ZeGFX] .zmat unsupported version %d in '%s'", header->Version, p_path));
				if (r_error) *r_error = ERR_FILE_CORRUPT;
				return Ref<Resource>();
			}
			uint64_t offset = sizeof(zegfx::cooked::ZeCookHeader) + header->DependencyCount * 36;

			if (offset + sizeof(zegfx::cooked::ZeMaterialMetadata) <= file_len) {
				const auto *meta = reinterpret_cast<const zegfx::cooked::ZeMaterialMetadata *>(data + offset);
				offset += sizeof(zegfx::cooked::ZeMaterialMetadata);

				// Read constant buffer (PBR parameters)
				if (meta->ConstantBufferBytes >= 36 && offset + meta->ConstantBufferBytes <= file_len) {
					const float *cb = reinterpret_cast<const float *>(data + offset);
					// Constant buffer parameters from GLTF/FBX are ALREADY in linear space.
					Color albedo_col = Color(cb[0], cb[1], cb[2], cb[3]);
					if (albedo_col.r <= 0.001f && albedo_col.g <= 0.001f && albedo_col.b <= 0.001f) {
						albedo_col = Color(1.0f, 1.0f, 1.0f, 1.0f);
					}
					mat->set_albedo(albedo_col);
					if (cb[4] > 0.001f || cb[5] > 0.001f || cb[6] > 0.001f) {
						mat->set_emission(Color(cb[4], cb[5], cb[6]));
						mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
					}
					mat->set_metallic(cb[7]);
					float roughness_val = cb[8];
					if (roughness_val <= 0.001f) {
						roughness_val = 1.0f;
					}
					mat->set_roughness(roughness_val);

					if (meta->BlendMode == 2) {
						mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
					} else if (meta->BlendMode == 1) {
						mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
						mat->set_alpha_scissor_threshold(0.5f);
					}
					if (meta->RasterizerState == 1) {
						mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
					}
				}

				// 1. Read Texture Bindings directly from the .zmat binary!
				offset += meta->ConstantBufferBytes;
				for (uint32_t b = 0; b < meta->TextureParameterCount; ++b) {
					if (offset + sizeof(zegfx::cooked::ZeMaterialTextureBinding) <= file_len) {
						const auto *binding = reinterpret_cast<const zegfx::cooked::ZeMaterialTextureBinding *>(data + offset);
						offset += sizeof(zegfx::cooked::ZeMaterialTextureBinding);

						String param_name = String::utf8(binding->ParameterName).strip_edges();
						String tex_filename = String::utf8(binding->TexturePath).strip_edges();

						print_line(vformat("[ZeGFX ZMAT] Parsed Binding #%d: param='%s', tex='%s'", b, param_name, tex_filename));

						Ref<Texture2D> tex;
						if (!tex_filename.is_empty()) {
							for (int d = 0; d < search_dirs.size(); ++d) {
								String full_p = search_dirs[d].path_join(tex_filename);
								tex = _load_texture_robust(full_p);
								if (tex.is_valid()) {
									print_line(vformat("[ZeGFX ZMAT] Found & loaded bound texture: '%s'", full_p));
									break;
								}
								// Try with .ztex / .zetex / .png / .jpg / .tga / .dds / .webp
								String base_t = tex_filename.get_basename();
								for (const char* ext : {".ztex", ".zetex", ".png", ".jpg", ".jpeg", ".tga", ".dds", ".webp", ".exr"}) {
									String alt_p = search_dirs[d].path_join(base_t + ext);
									tex = _load_texture_robust(alt_p);
									if (tex.is_valid()) {
										print_line(vformat("[ZeGFX ZMAT] Found & loaded bound texture: '%s'", alt_p));
										break;
									}
								}
								if (tex.is_valid()) break;
							}
						}

						if (tex.is_valid()) {
							String p_lower = param_name.to_lower();
							bool is_albedo = (param_name == "Albedo" || p_lower.contains("albedo") || p_lower.contains("diffuse") || p_lower.contains("basecolor") || p_lower.contains("base_color") || param_name == "#D2077977");
							bool is_normal = (param_name == "Normal" || p_lower.contains("normal") || param_name == "#459A0837");
							bool is_orm = (param_name == "OcclusionRoughnessMetallic" || param_name == "ORM" || param_name == "#F6A8B084");
							bool is_rough_metal = (param_name == "RoughnessMetallic" || p_lower.contains("rough") || p_lower.contains("arm") || p_lower.contains("orm") || param_name == "#4F1BF470");
							bool is_metal = (param_name == "Metallic" || param_name == "Metalness" || p_lower.contains("metal") || param_name == "#E1A17BA2");
							bool is_ao = (param_name == "AmbientOcclusion" || param_name == "occlusionTexture" || p_lower.contains("occlusion") || p_lower == "ao" || param_name == "#3D7A4F23");
							bool is_emiss = (param_name == "Emissive" || p_lower.contains("emiss") || param_name == "#E821C8A1");

							if (is_albedo) {
								albedo_tex = tex;
							} else if (is_normal) {
								normal_tex = tex;
							} else if (is_orm) {
								rough_tex = tex;
								ao_tex = tex;
							} else if (is_metal) {
								metal_tex = tex;
							} else if (is_rough_metal) {
								rough_tex = tex;
							} else if (is_ao) {
								ao_tex = tex;
							} else if (is_emiss) {
								emit_tex = tex;
							}
						}
					}
				}
			}
		}
	}

	// 2. Fallback fuzzy token scanner if textures were not bound in header
	String mat_base_name = p_path.get_file().get_basename().to_lower();
	int dot_pos = mat_base_name.rfind(".");
	if (dot_pos != -1 && dot_pos > 0) {
		mat_base_name = mat_base_name.substr(0, dot_pos);
	}
	String mat_clean = mat_base_name.replace(" ", "_").replace(".", "_").to_lower();

	Vector<String> all_candidate_images;

	for (int d = 0; d < search_dirs.size(); ++d) {
		Ref<DirAccess> da = DirAccess::open(search_dirs[d]);
		if (da.is_null()) continue;

		da->list_dir_begin();
		String f_name = da->get_next();
		while (!f_name.is_empty()) {
			if (!da->current_is_dir()) {
				String ext = f_name.get_extension().to_lower();
				if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "exr" || ext == "webp" || ext == "dds" || ext == "tga" || ext == "ztex" || ext == "zetex") {
					String fn_lower = f_name.to_lower();
					String full_path = search_dirs[d].path_join(f_name);
					all_candidate_images.push_back(full_path);

					bool matches_mat = false;
					if (fn_lower.contains(mat_clean)) {
						matches_mat = true;
					} else {
						PackedStringArray tokens = mat_clean.split("_");
						int matched_tokens = 0;
						for (int t = 0; t < tokens.size(); ++t) {
							String tok = tokens[t];
							if (tok.length() >= 3 && fn_lower.contains(tok)) {
								matched_tokens++;
							} else if (tok == "01" || tok == "02" || tok == "23" || tok == "001" || tok == "002" || tok == "003") {
								String short_num = tok.lstrip("0");
								if (fn_lower.contains(tok) || (!short_num.is_empty() && fn_lower.contains(short_num))) {
									matched_tokens++;
								}
							}
						}
						if (matched_tokens >= 1) {
							matches_mat = true;
						}
					}

					if (matches_mat) {
						if (albedo_tex.is_null() && (fn_lower.contains("basecolor") || fn_lower.contains("base_color") || fn_lower.contains("diff") || fn_lower.contains("albedo") || fn_lower.contains("color") || fn_lower.contains("alb"))) {
							albedo_tex = _load_texture_robust(full_path);
						} else if (normal_tex.is_null() && (fn_lower.contains("normal") || fn_lower.contains("nor_gl") || fn_lower.contains("nor") || fn_lower.contains("nrm"))) {
							normal_tex = _load_texture_robust(full_path);
						} else if (rough_tex.is_null() && (fn_lower.contains("roughness") || fn_lower.contains("rough") || fn_lower.contains("arm") || fn_lower.contains("orm") || fn_lower.contains("rgh"))) {
							rough_tex = _load_texture_robust(full_path);
						} else if (metal_tex.is_null() && (fn_lower.contains("metallic") || fn_lower.contains("metal") || fn_lower.contains("met"))) {
							metal_tex = _load_texture_robust(full_path);
						} else if (emit_tex.is_null() && (fn_lower.contains("emissive") || fn_lower.contains("emission") || fn_lower.contains("emit"))) {
							emit_tex = _load_texture_robust(full_path);
						} else if (ao_tex.is_null() && (fn_lower.contains("occlusion") || fn_lower.contains("_ao") || fn_lower.contains("ao_") || fn_lower.contains("ao."))) {
							ao_tex = _load_texture_robust(full_path);
						} else if (opacity_tex.is_null() && (fn_lower.contains("opacity") || fn_lower.contains("alpha") || fn_lower.contains("mask"))) {
							opacity_tex = _load_texture_robust(full_path);
						}
					}
				}
			}
			f_name = da->get_next();
		}
		da->list_dir_end();
	}

	// Fallback for generic materials (e.g. Material.001, Checker, Door, Window) if no specific texture match
	if (!all_candidate_images.is_empty()) {
		for (int i = 0; i < all_candidate_images.size(); ++i) {
			String fn = all_candidate_images[i].to_lower();
			if (albedo_tex.is_null() && (fn.contains("image_") || fn.contains("diff") || fn.contains("basecolor") || fn.contains("base_color") || fn.contains("albedo") || fn.contains("color") || fn.contains("alb"))) {
				albedo_tex = _load_texture_robust(all_candidate_images[i]);
			} else if (normal_tex.is_null() && (fn.contains("normal") || fn.contains("nor_gl") || fn.contains("nor") || fn.contains("nrm"))) {
				normal_tex = _load_texture_robust(all_candidate_images[i]);
			} else if (rough_tex.is_null() && (fn.contains("roughness") || fn.contains("rough") || fn.contains("arm") || fn.contains("orm") || fn.contains("rgh"))) {
				rough_tex = _load_texture_robust(all_candidate_images[i]);
			} else if (metal_tex.is_null() && (fn.contains("metallic") || fn.contains("metal") || fn.contains("met"))) {
				metal_tex = _load_texture_robust(all_candidate_images[i]);
			} else if (ao_tex.is_null() && (fn.contains("occlusion") || fn.contains("_ao") || fn.contains("ao_") || fn.contains("ao."))) {
				ao_tex = _load_texture_robust(all_candidate_images[i]);
			} else if (emit_tex.is_null() && (fn.contains("emissive") || fn.contains("emission") || fn.contains("emit"))) {
				emit_tex = _load_texture_robust(all_candidate_images[i]);
			}
		}
	}

	if (albedo_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, albedo_tex);
	}
	if (normal_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_NORMAL, normal_tex);
		mat->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
	}
	if (rough_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, rough_tex);
		mat->set_roughness_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_GREEN);
		if (mat->get_texture(BaseMaterial3D::TEXTURE_METALLIC).is_null()) {
			mat->set_texture(BaseMaterial3D::TEXTURE_METALLIC, rough_tex);
			mat->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_BLUE);
		}
		// Legacy fallback for .zmat files cooked before the explicit AO binding
		// existed: infer packed AO from the roughness texture's filename. Only
		// used if nothing more explicit (packed-ORM binding or dedicated AO
		// binding/filename match) already supplied one.
		if (ao_tex.is_null() && (rough_tex->get_path().to_lower().contains("arm") || rough_tex->get_path().to_lower().contains("orm"))) {
			ao_tex = rough_tex;
		}
	}
	if (metal_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_METALLIC, metal_tex);
		mat->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
	}
	if (emit_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_EMISSION, emit_tex);
		mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
	}
	if (opacity_tex.is_valid()) {
		mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
	}
	if (ao_tex.is_valid()) {
		mat->set_texture(BaseMaterial3D::TEXTURE_AMBIENT_OCCLUSION, ao_tex);
		mat->set_ao_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
		mat->set_feature(BaseMaterial3D::FEATURE_AMBIENT_OCCLUSION, true);
	}

	mat->set_meta("zegfx_zmat_path", p_path);
	mat->set_meta("zegfx_cooked", true);

	print_line(vformat("[ZeGFX] Loaded .zmat: '%s' (Albedo: %s, Normal: %s, AO: %s)", p_path.get_file(), albedo_tex.is_valid() ? "YES" : "NO", normal_tex.is_valid() ? "YES" : "NO", ao_tex.is_valid() ? "YES" : "NO"));

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

Ref<Resource> ResourceFormatLoaderZTex::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = OK;
	}
	Ref<Texture2D> tex = _load_zetex_file(p_path);
	if (tex.is_valid()) {
		if (r_progress) *r_progress = 1.0;
		return tex;
	}
	if (r_error) *r_error = ERR_FILE_CANT_OPEN;
	return Ref<Resource>();
}

void ResourceFormatLoaderZTex::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("ztex");
	p_extensions->push_back("zetex");
}

bool ResourceFormatLoaderZTex::handles_type(const String &p_type) const {
	return p_type == "Texture2D" || p_type == "ImageTexture" || p_type == "Texture";
}

String ResourceFormatLoaderZTex::get_resource_type(const String &p_path) const {
	String ext = p_path.get_extension().to_lower();
	if (ext == "ztex" || ext == "zetex") {
		return "ImageTexture";
	}
	return "";
}
