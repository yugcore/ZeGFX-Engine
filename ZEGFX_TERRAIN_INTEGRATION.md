# ZeGFX Terrain Integration Plan: Replacing Godot's Heightmap & Terrain System

## Executive Summary

The objective of this architectural change is to replace Godot Engine's generic heightmap handling and third-party plugin dependency with **ZeGFX Renderer's native C++ `zegfx::TerrainSystem` and `HeightmapImporter`**.

By integrating ZeGFX's native landscape architecture directly into the engine core, we unlock:
- **16-bit Multi-Format Heightmap Loading:** Native support for 16-bit PNG, RAW_8/16/32F, and HDR files (`stbi_load_16`).
- **Multi-Tile Directory Stitching:** Automatic seamless tile stitching with edge-pixel overlap blending (`averageSharedEdges`).
- **High-Quality Resampling:** Bicubic, Bilinear, and Box-filter Mipmap generation (`ResampleBicubic`).
- **Hardware-Accelerated Chunking & Culling:** Automatic $64 \times 64$ quad chunking with per-chunk AABB frustum culling directly in the DX12 renderer pass (`dx12_backend_frame.cpp`).
- **Synchronized Physics:** Zero-copy C++ floating-point data bridge into `PhysicsServer3D` and `HeightMapShape3D`.

---

## 🏗️ Architectural Overview

```
                        +-----------------------------------------+
                        |      ZeGFX Terrain Editor Node          |
                        |          (ZeGFXTerrain3D)               |
                        +--------------------+--------------------+
                                             |
                   +-------------------------+-------------------------+
                   |                                                   |
                   v                                                   v
     +---------------------------+                           +-------------------+
     |    ZeGFX Importer         |                           | Physics Sync      |
     | (HeightmapImporter C++)   |                           | (HeightMapShape3D)|
     +-------------+-------------+                           +---------+---------+
                   |                                                   |
                   v                                                   v
     +---------------------------+                           +-------------------+
     |  ZeGFX::TerrainSystem     |                           | PhysicsServer3D   |
     | (Chunking & Height Sample)|                           | (Jolt / Godot)    |
     +-------------+-------------+                           +-------------------+
                   |
                   v
     +---------------------------+
     | ZeGFX DX12 Render Backend |
     | (procedural_landscape)    |
     +---------------------------+
```

---

## 🔄 Component Comparison & Replacement Mapping

| Domain | Legacy Godot System | ZeGFX Renderer Replacement |
| :--- | :--- | :--- |
| **Heightmap Import** | `ResourceImporterTexture` (8-bit / lossy PNG / EXR) | `zegfx::HeightmapImporter::LoadFromFile()` / `LoadTilesFromDirectory()` |
| **Tile Stitching** | External scripts / manual Blender prep | Native `StitchTiles()` with `TileMapSettings` edge blending |
| **Mesh Generation** | GDScript `SurfaceTool` or custom shader displacement | `zegfx::TerrainSystem::generateChunkMeshes()` ($64 \times 64$ quad chunks) |
| **Render Backend** | `RenderingServer` mesh instances | ZeGFX DX12 Backend `createProceduralModel()` + `procedural_landscape` pass |
| **Frustum Culling** | Scene graph node-level bounding box | Sub-chunk AABB frustum culling (`boundsMin` / `boundsMax` per chunk) |
| **CPU Height Queries**| Reading `Image.get_pixel()` | `zegfx::terrainSampleHeightInternal()` fast C++ bilinear sampler |
| **Physics Collision**| Manual `HeightMapShape3D` image updating | Automated C++ bridge populating `HeightMapShape3D` float buffer |

---

## 📋 Implementation Phases

### Phase 1: Engine Core Wrapper Class (`ZeGFXTerrain3D`)
**Files to add:** `scene/3d/zegfx_terrain_3d.h`, `scene/3d/zegfx_terrain_3d.cpp`

Create a specialized engine node subclassing `Node3D` (or `VisualInstance3D`) that acts as the high-level scene representation of a ZeGFX terrain.

```cpp
class ZeGFXTerrain3D : public Node3D {
    GDCLASS(ZeGFXTerrain3D, Node3D);

private:
    String heightmap_path;
    String tile_directory_path;
    float cell_size = 2.0f;
    float height_scale = 45.0f;
    int chunk_size = 64;
    bool enable_stitching = false;
    bool resample_bicubic = true;

    int terrain_id = -1;
    Ref<HeightMapShape3D> collision_shape;
    StaticBody3D *static_body = nullptr;

protected:
    static void _bind_methods();

public:
    void set_heightmap_path(const String &p_path);
    String get_heightmap_path() const;

    void rebuild_terrain();
    float sample_height_at(Vector3 p_world_pos) const;
};
```

---

### Phase 2: Editor Import Plugin (`ResourceImporterZeGFXTerrain`)
**Files to modify/add:** `editor/import/zegfx_terrain_importer.h`, `editor/import/zegfx_terrain_importer.cpp`

Extend Godot's asset import pipeline to recognize heightmap files (`.png`, `.raw`, `.r16`, `.exr`, `.hdr`) and directory tile manifests.

1. **Import Hook:** When a heightmap texture or directory manifest is dropped into the project, `ResourceImporterZeGFXTerrain` intercepts it.
2. **Invoke ZeGFX Importer:**
   ```cpp
   zegfx::HeightmapImportSettings settings;
   settings.cellSize = p_options["cell_size"];
   settings.heightScale = p_options["height_scale"];
   settings.resample = p_options["resample_bicubic"];
   
   zegfx::HeightmapImportResult result = zegfx::HeightmapImporter::LoadFromFile(filePath, settings);
   ```
3. **Bake Binary Resource (`.zegfx_terrain`):** Serialize the resulting `TerrainDescriptor`, float heights vector, and per-chunk AABB bounds into a compressed engine binary resource.

---

### Phase 3: DX12 Renderer Integration & Procedural Pass
**Files modified:** `ZeGFX/src/dx12/dx12_backend_frame.cpp`, `ZeGFX/src/dx12/dx12_backend_models.cpp`

1. **Direct Sub-Chunk Frustum Culling:**
   Ensure each `TerrainChunk` registered under `procedural_landscape` participates in the early depth/frustum culling pass:
   ```cpp
   if (modelData->path == "procedural_landscape") {
       // Frustum test chunk AABB against hostFrustum
       res = testAABBFrustum(hostFrustum, chunk.boundsMin, chunk.boundsMax);
   }
   ```
2. **Batching & Draw Calls:**
   Bind chunk vertex buffers dynamically during `executeOpaquePass` without triggering expensive state rebinds.

---

### Phase 4: Physics & Collision Synchronization
**Files modified:** `scene/3d/zegfx_terrain_3d.cpp`

Automatically synchronize ZeGFX's terrain data with Godot's 3D physics server:

```cpp
void ZeGFXTerrain3D::_sync_physics() {
    zegfx::TerrainResource *res = zegfx_system->getTerrain(terrain_id);
    if (!res) return;

    if (!collision_shape.is_valid()) {
        collision_shape.instantiate();
    }

    collision_shape->set_map_width(res->descriptor.width);
    collision_shape->set_map_depth(res->descriptor.height);

    // Direct zero-copy or fast vector float swap to PhysicsServer3D
    Vector<real_t> godot_heights;
    godot_heights.resize(res->heights.size());
    memcpy(godot_heights.ptrw(), res->heights.data(), res->heights.size() * sizeof(float));

    collision_shape->set_map_data(godot_heights);
}
```

---

## 🧪 Verification & Benchmarking Plan

### 1. Unit Tests (`tests/heightmap_pipeline_tests.cpp`)
- Validate 16-bit raw import accuracy and datum conversions (`ConvertRawToWorldSpace`).
- Test multi-tile stitching (`LoadTilesFromDirectory`) for seam elimination.
- Verify bicubic vs box-filter downsampling consistency.

### 2. Integration Verification
- Load a $4096 \times 4096$ tiled terrain dataset into `ZeGFXTerrain3D`.
- Verify GPU Memory footprint, chunk frustum culling statistics in `dx12_backend_frame.cpp` log output:
  `[DX12 executeOpaquePass] DRAWING procedural_landscape!`
- Perform character collision tests using `CharacterBody3D` traversing rugged terrain slopes.

---

## 🚀 Execution Checklist

- [ ] Add `ZeGFXTerrain3D` core node bindings (`scene/3d/zegfx_terrain_3d.cpp`).
- [ ] Register `.zegfx_terrain` asset importer in `editor/register_editor_types.cpp`.
- [ ] Wire `zegfx::TerrainSystem` chunk handles into DX12 frame renderer (`dx12_backend_frame.cpp`).
- [ ] Bridge `HeightMapShape3D` float data array to `PhysicsServer3D`.
- [ ] Run benchmark validation on $2048 \times 2048$ and $4096 \times 4096$ landscape maps.
