# Architectural Decision: Terrain LOD Architecture & Virtual Texture Streaming

**Status**: Accepted  
**Date**: 2026-08-25  
**Scope**: `Terrain3D` chunk LOD system, `VirtualTexture2D` page-table streaming pipeline.

---

## 1. Terrain3D Custom LOD vs. Godot GeometryInstance3D LOD

### Architectural Analysis & Decision
Godot Engine features a built-in mesh LOD system managed through `GeometryInstance3D` / `ImporterMesh` surface decimation (using `meshoptimizer`). In standard static prop meshes, decimation operates independently per asset.

However, continuous terrain heightfields cannot use standard independent mesh decimation:
1. **Crack & Seam Prevention**: Independent decimation on adjacent chunk meshes produces mismatched border vertices along chunk boundaries, creating visible seam cracks and void holes.
2. **Edge Skirts Integration**: `Terrain3D` generates regular-stride quad meshes ($2^k$ decimation per LOD level) paired with **vertical perimeter skirts** (`skirt_height`) along north, south, east, and west boundaries. These vertical skirts drop down below the surface, visually masking any height discrepancy between adjacent chunks at differing LODs without cracks.
3. **Collision & Height Query Synchronization**: Terrain physics shapes (`HeightMapShape3D`) and gameplay raycasts (`sample_height`, `get_normal_at`) depend on the exact grid coordinate topology of `Terrain3D`.

**Decision**: `Terrain3D` running its own CPU distance-based LOD with edge skirts is **intentional, necessary, and correct**.

### Main-Thread Stutter Elimination
To eliminate synchronous frame spikes when many chunks change LOD simultaneously during fast camera motion:
- **Frame Swap Throttling**: `max_lod_swaps_per_frame` (default: 8 swaps/frame) caps the number of `set_mesh()` operations executed per frame.
- **Distance-Weighted Prioritization**: Pending chunk swaps are sorted by proximity to the camera so pop-in closest to the player resolves first.
- **Hysteresis Banding**: `lod_hysteresis_margin` (default: 15%) prevents edge fluttering when the camera moves along chunk boundaries.
- **Movement Thresholding**: `lod_update_distance_threshold` skips distance re-computations when the camera is stationary or moves sub-threshold distances.

---

## 2. VirtualTexture2D Streaming & Feedback Driver

### Architectural Analysis & Decision
`VirtualTexture2D` provides sparse virtual texturing via:
1. **Page Table Texture**: Maps virtual tile coordinates `(tile_x, tile_y, mip)` to physical cache slot indices.
2. **Physical Cache Texture**: Fixed-size texture atlas (e.g. 2048x2048) allocated into uniform physical slots (e.g. 128x128).
3. **LRU Cache Eviction**: Tracks `last_access_tick` to evict least-recently-used tiles within a configurable VRAM budget.

### Current Driver Status: Script & CPU Region Driven
- **Current State**: Driven via CPU/script queries:
  - `request_region(Rect2 uv_rect, int mip_level)`
  - `request_tiles_around_point(Vector2 uv_point, float radius_uv, int mip_level)`
  - `signal tile_requested(int tile_x, int tile_y, int mip_level)`
  - `upload_tile_data(int tile_x, int tile_y, int mip_level, Ref<Image> image)`
- **Future GPU Hardware Feedback Buffer**: Hardware GPU feedback buffer readback (binding a feedback UAV texture in pixel shaders and reading back tile requests to CPU) is explicitly documented as a **planned future GPU compute pass** and is not currently active. The active engine implementation relies on the script-driven streaming interface.
