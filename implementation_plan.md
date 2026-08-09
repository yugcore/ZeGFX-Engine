# Direct3D 12 Migration Plan (`ZeGFXMigration.md`)

This plan outlines the roadmap to transition the engine's primary graphics backend to **Direct3D 12 (DX12)** using the **Thin ZeGFX Abstraction** strategy, demoting Vulkan and GLES3 to secondary/fallback status.

The migration documentation has been updated and saved to [ZeGFXMigration.md](file:///z:/ZeGFX-Engine/ZeGFXMigration.md).

## User Review Required

> [!IMPORTANT]
> **Selected Strategy: Thin ZeGFX Abstraction**: Engine driver interfaces (`drivers/d3d12/zegfx_d3d12_bridge`) will route commands through `ZeGFX::GraphicsBackend` and ZeGFX renderer subsystems (Render Graph, G-Buffer, Volumetric Fog, Virtual Geometry, DXR), while low-level D3D12 calls remain encapsulated inside `ZeGFX/src/dx12/`.

> [!IMPORTANT]
> **Primary Backend Promotion**: On Windows platforms, `d3d12` will become the primary default rendering driver in `main.cpp` and `display_server_windows.cpp`, moving `vulkan` and `gles3` to secondary fallback options.

> [!WARNING]
> **ZeGFX Bottleneck Hardening (Phase 2)**: Before removing the per-frame `waitForFenceValue` stall post-present, ZeGFX's CPU-to-GPU dynamic upload queues (2D vertex buffers, shared constant buffers) must be upgraded to triple-buffered ring buffers (`dx12_upload_ring_buffer.h`).

## Open Questions

1. **DXR Hardware Ray Tracing Scope**: Would you like Phase 8 (DXR hardware `DispatchRays` activation) enabled by default on supported GPUs, or should it remain behind an opt-in project setting flag (`rendering/ray_tracing/enabled`)?
2. **Shader Compilation Pipeline**: Should we maintain live DXC runtime shader compilation (`shader_compiler.cpp`) during development builds and transition to pre-compiled `.zeshaderpack` binary archives for shipping builds?

## Proposed Changes

### Primary Driver Selection & Engine Bridge Setup (Phase 1)

#### [MODIFY] [main.cpp](file:///z:/ZeGFX-Engine/main/main.cpp)
- Change default value of `rendering/rendering_device/driver.windows` from `"vulkan"` to `"d3d12"`.

#### [MODIFY] [display_server_windows.cpp](file:///z:/ZeGFX-Engine/platform/windows/display_server_windows.cpp)
- Prioritize `RenderingContextDriverD3D12` during window creation and rendering context initialization.

#### [MODIFY] [os_windows.cpp](file:///z:/ZeGFX-Engine/platform/windows/os_windows.cpp)
- Set `RenderingContextDriverD3D12` as the default rendering driver on Windows.

#### [MODIFY] [SConstruct](file:///z:/ZeGFX-Engine/SConstruct)
- Set `d3d12=yes` by default for Windows builds.

#### [MODIFY] [SCsub](file:///z:/ZeGFX-Engine/drivers/d3d12/SCsub)
- Include ZeGFX DX12 backend source files and header search paths.

#### [NEW] [zegfx_d3d12_bridge.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_d3d12_bridge.h)
#### [NEW] [zegfx_d3d12_bridge.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_d3d12_bridge.cpp)
- C++/C thin bridge interfacing `RenderingContextDriverD3D12` and `RenderingDeviceDriverD3D12` with `ZeGFX::GraphicsBackend`.

---

### ZeGFX Core DX12 Backend Hardening (Phase 2)

#### [MODIFY] [dx12_backend_core.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_backend_core.cpp)
- Remove mandatory per-frame `waitForFenceValue` stall post-present.

#### [MODIFY] [dx12_backend_resources.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_backend_resources.cpp)
- Implement per-frame deferred resource & descriptor release queues.

#### [MODIFY] [dx12_backend_frame.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_backend_frame.cpp)
- Decouple frame lifecycle sync from pass execution.

#### [MODIFY] [dx12_backend_2d.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_backend_2d.cpp)
- Upgrade 2D vertex upload buffers to use dynamic upload ring-buffers.

#### [NEW] [dx12_upload_ring_buffer.h](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_upload_ring_buffer.h)
#### [NEW] [dx12_upload_ring_buffer.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_upload_ring_buffer.cpp)
- Ring buffer allocator interface for CPU-to-GPU dynamic constants/vertices across in-flight frames.

#### [NEW] [dx12_descriptor_heap.h](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_descriptor_heap.h)
- Modular descriptor heap manager with per-frame free-lists.

---

### Render Graph, Pipeline Caching & Scene Integration (Phases 3 & 4)

#### [MODIFY] [dx12_render_graph.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_render_graph.cpp)
#### [MODIFY] [pipeline_cache.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/pipeline_cache.cpp)
#### [MODIFY] [gbuffer.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/lighting/gbuffer.cpp)
#### [MODIFY] [lighting_system.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/lighting/lighting_system.cpp)

#### [NEW] [zegfx_pipeline_state_manager.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_pipeline_state_manager.h)
#### [NEW] [zegfx_pipeline_state_manager.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_pipeline_state_manager.cpp)
#### [NEW] [renderer_scene_zegfx_dx12.h](file:///z:/ZeGFX-Engine/servers/rendering/renderer_rd/renderer_scene_zegfx_dx12.h)
#### [NEW] [renderer_scene_zegfx_dx12.cpp](file:///z:/ZeGFX-Engine/servers/rendering/renderer_rd/renderer_scene_zegfx_dx12.cpp)

---

### Post-Processing, Volumetrics, Virtual Geometry & DXR (Phases 5 - 8)

#### [MODIFY] [shadow_system.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/lighting/shadow_system.cpp)
#### [MODIFY] [ambient_occlusion.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/post/ambient_occlusion.cpp)
#### [MODIFY] [screen_space_reflections.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/post/screen_space_reflections.cpp)
#### [MODIFY] [virtual_geometry.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/virtual_geometry.cpp)
#### [MODIFY] [dx12_dxr.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/dx12/dx12_dxr.cpp)

#### [NEW] [gtao_pass.cpp](file:///z:/ZeGFX-Engine/ZeGFX/src/post/gtao_pass.cpp)
#### [NEW] [zegfx_volumetrics_pass.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_volumetrics_pass.h)
#### [NEW] [zegfx_volumetrics_pass.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_volumetrics_pass.cpp)
#### [NEW] [zegfx_post_composite.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_post_composite.h)
#### [NEW] [zegfx_post_composite.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_post_composite.cpp)
#### [NEW] [zegfx_gpu_scene_bridge.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_gpu_scene_bridge.h)
#### [NEW] [zegfx_gpu_scene_bridge.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_gpu_scene_bridge.cpp)
#### [NEW] [zegfx_dxr_pipeline.h](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_dxr_pipeline.h)
#### [NEW] [zegfx_dxr_pipeline.cpp](file:///z:/ZeGFX-Engine/drivers/d3d12/zegfx_dxr_pipeline.cpp)

---

## Verification Plan Per Phase

| Phase | Core Mechanism | Verification Method | Pass Criteria / Expected Result |
| :--- | :--- | :--- | :--- |
| **Phase 1** | Primary Driver Selection | Log inspection & RenderDoc capture | Log confirms `Direct3D 12 (ZeGFX)` selected; RenderDoc confirms D3D12 API. |
| **Phase 2** | Backend Hardening & Upload Buffers | Visual dynamic mesh test & D3D12 Debug Layer | 0 vertex flickering; 0 GBV/validation errors; CPU wait drops <0.2ms. |
| **Phase 3** | Render Graph & PSO Caching | GPU-Based Validation & Relaunch timing | 0 barrier mismatch errors; 2nd launch loads PSOs from disk cache hitlessly. |
| **Phase 4** | G-Buffer & Deferred Light Grid | Virtual Environment G-buffer keys (1-4) & 200 light stress test | Clean Albedo/Normal/Roughness/Depth visual debug slices; 60+ FPS with 200 lights. |
| **Phase 5** | Froxel Volumetric Fog & VSM Shadows | Cascade debug colors & God ray visual inspection | 4 distinct cascade splits; smooth volumetric light shafts without banding. |
| **Phase 6** | GTAO, SSR & Post-Processing | Visual AO comparison & reflection trace view | GTAO shows rich crease grounding; SSR ray-marches wet surface reflections. |
| **Phase 7** | Virtual Geometry & GPU Submission | Frozen frustum debug & RenderDoc draw calls | Frustum culling discards 100% off-screen clusters; `ExecuteIndirect` in RenderDoc. |
| **Phase 8** | DXR Hardware Ray Tracing | RenderDoc `DispatchRays` capture & non-DXR fallback test | Active `DispatchRays` call on DXR GPU; graceful compute fallback on non-DXR. |
| **Phase 9** | Headless & Visual Canary Gate | `run_headless_smokes.ps1` & `run_visual_validation.ps1` | **45/45 headless scripts pass**; **0 canary pixel diff mismatches**. |
