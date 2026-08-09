# ZeGFX Direct3D 12 Engine — Technical Manual & Architecture Guide

Welcome to the official engine documentation for the **ZeGFX Direct3D 12 Graphics Engine** powering Velvet Engine.

ZeGFX is a modern, high-performance C++20 rendering backend built on **Direct3D 12 (DX12)** and **DirectX Raytracing (DXR 1.1)**. It features an asynchronous Render Graph DAG compiler, zero-allocation upload ring buffers, clustered deferred PBR shading, 3D froxel volumetric fog, ground-truth occlusion, virtual geometry meshlet culling, and real-time probe-based Global Illumination (ZGI).

---

## 1. High-Level Engine Architecture

ZeGFX interfaces directly with the native Win32 window subsystem and replaces legacy rendering abstractions with a unified Direct3D 12 hardware pipeline:

```
+-------------------------------------------------------------------------+
|                        VELVET ENGINE EDITOR & APP                       |
|      (Editor UI / Viewports / Node3D / WorldEnvironment / Inspector)    |
+------------------------------------+------------------------------------+
                                     |
                                     v (Scene Snapshot & HWND Window)
+------------------------------------+------------------------------------+
|                      ZEGFX ENGINE BRIDGE LAYER                          |
|         (zegfx_d3d12_bridge / renderer_scene_d3d12 / volumetrics)       |
+------------------------------------+------------------------------------+
                                     |
                                     v (Asynchronous Pass Dispatch)
+------------------------------------+------------------------------------+
|                       ZEGFX D3D12 GRAPHICS ENGINE                       |
|  - Low-Level D3D12 Hardware RHI & Command Queues (ID3D12Device5)        |
|  - Asynchronous Render Graph & Automatic Subresource Barriers           |
|  - Triple-Buffered Upload Ring Allocators (FRAME_COUNT = 3)             |
|  - Descriptor Heap Managers & Delayed Free-List Release                 |
|  - Pipeline State Object (PSO) Binary Disk Caching (dx12_psos.bin)      |
|  - PBR Clustered Deferred G-Buffer (Depth32 / RGBA16F / RGBA8)          |
|  - Tile-Based Compute Light Grid Culler (16x16 Tiles, 256 lights)       |
|  - Virtual Shadow Maps (VSM) & 4-Cascade PCF Atlases                    |
|  - 3D Texture Froxel Volumetric Fog & Light Shaft Raymarching           |
|  - Ground Truth Ambient Occlusion (GTAO) & Screen-Space Reflections     |
|  - Virtual Geometry Meshlet Cluster Culling & ExecuteIndirect MDI       |
|  - DXR 1.1 Hardware Ray Tracing (Raytraced Shadows, RTR, RTAO, ZGI GI)  |
+-------------------------------------------------------------------------+
```

---

## 2. Core Driver & Memory Subsystems

### Direct3D 12 Hardware RHI
* **Low-Level Native Device**: Owns Direct3D 12 device creation (`ID3D12Device5`), DXGI factory swapchains, direct command queues, and hardware adapter selection.
* **Win32 Integration**: Binds native window handles (`HWND`) directly to DXGI swapchain buffers without intermediate copying.
* **How to Verify**: Driver selection defaults to `d3d12` on Windows. Verify in **Project Settings -> Rendering -> Rendering Device -> Driver.Windows**.

### Triple-Buffered Upload Ring Allocator
* **Zero CPU-GPU Stalls**: Dynamically suballocates CPU-to-GPU uploads across `FRAME_COUNT = 3` in-flight frame slots.
* **Fence Tracking**: Advances slots only when `completedFenceValue >= slot.fenceValue`, ensuring zero buffer overwrites during dynamic vertex, index, font texture, and uniform buffer updates.
* **How to Use**: Operates 100% automatically under the hood.

### Render Graph & Automatic Barrier Engine
* **Asynchronous DAG Compilation**: Builds directed acyclic graphs (DAG) for all rendering and compute passes, aliasing transient memory targets to reduce VRAM footprint.
* **Automated Subresource Barriers**: Automatically tracks resource states (`RenderTarget`, `DepthWrite`, `PixelShaderResource`, `UnorderedAccess`, `CopySrc`/`CopyDest`) and injects minimal `D3D12_RESOURCE_BARRIER` transitions.

### Binary PSO Disk Caching
* **Hitch-Free Compilation**: Prewarms and serializes compiled D3D12 Pipeline State Objects (PSOs) to disk (`%LOCALAPPDATA%\ZeGFX\shader_cache\dx12_psos.bin`), eliminating shader compilation stutter.

---

## 3. Clustered Deferred Shading Engine

### Multi-Render-Target (MRT) G-Buffer Formats
ZeGFX decomposes opaque geometry rendering into dedicated high-precision G-Buffer channels:
* **`GBufferA` (`RGBA8_UNorm`)**: Base Albedo Color (RGB) + Alpha (A).
* **`GBufferB` (`RGBA16_Float`)**: High-precision World-Space Surface Normals.
* **`GBufferC` (`RGBA8_UNorm`)**: Roughness (R), Metallic (G), Ambient Occlusion (B).
* **`GBufferD` (`RGBA16_Float`)**: Emissive Color + Motion Vectors.
* **`DepthTarget` (`D32_Float`)**: Linearized 32-bit Depth Stencil Target.

### 16x16 Tile Compute Light Culling
* **High Multi-Light Scalability**: Divides screen space into `16x16` pixel tiles and dispatches a compute shader to cull point (`OmniLight3D`) and spot (`SpotLight3D`) lights against min/max tile depth bounds.
* **Performance**: Renders scenes with hundreds of dynamic point and spot lights at 60+ FPS in a single deferred shading pass.
* **How to Use**: Add `OmniLight3D` or `SpotLight3D` nodes anywhere in your scene; lighting is automatically batched into tile grids.

---

## 4. Atmosphere, Volumetrics & Shadow Systems

### 3D Texture Froxel Volumetric Fog & Light Shafts
* **3D Froxel Grid**: Generates a `160x90x128` 3D froxel volume in view space using a two-pass compute shader (`volumetric_inject.hlsl` + `volumetric_integrate.hlsl`).
* **Volumetric Light Shafts (God Rays)**: Computes in-scattering and transmittance along camera Z-slices, producing smooth volumetric light shafts behind light blockers.
* **How to Configure in Editor**:
  1. Add a **`WorldEnvironment`** node to your scene.
  2. In the Inspector, expand **Environment -> Volumetric Fog**:
     - Check **Enabled** = `ON`
     - Set **Density** = `0.04`
     - Set **Anisotropy** = `0.7` *(directs scattered light toward camera)*
  3. Select your **`DirectionalLight3D`**:
     - Check **Shadow -> Enabled** = `ON`
     - Set **Volumetric Fog Energy** = `2.0`
  4. Place a 3D mesh (e.g. `CSGBox3D`) between the sun and camera to see 3D god rays.

### Virtual Shadow Maps (VSM) & 4-Cascade PCF Atlases
* **Cascaded Directional Shadows**: Maps directional light shadows across 4 view-frustum splits with Percentage-Closer Filtering (PCF) blending.
* **Virtual Shadow Maps (VSM)**: Allocates shadow pages dynamically based on camera visibility.
* **How to Inspect in Editor**:
  1. Select a **`DirectionalLight3D`** node and set **Directional Shadow -> Mode** to `4 Cascades`.
  2. Click **Perspective -> Display Advanced... -> Directional Shadow Splits** in the 3D Viewport header menu to view the 4 colored frustum splits (Cascade 0: Red, Cascade 1: Green, Cascade 2: Blue, Cascade 3: Yellow).

---

## 5. Occlusion & Post-Processing Suite

### Ground Truth Ambient Occlusion (GTAO)
* **Horizon Search Occlusion**: Replaces legacy SSAO with horizon-based GTAO compute searches (`gtao.hlsl`) and spatial bilateral filtering.
* **Visual Quality**: Generates dark, physically accurate contact shadows in fine crevices without haloing around thin geometry.
* **How to Enable**: Check **Environment -> SSAO** in the `WorldEnvironment` Inspector.

### Screen-Space Raymarched Reflections (SSR)
* **View-Space Raytracing**: Traces reflection rays in view space using hierarchical depth buffers and binary search refinement, blending smoothly into environment probes at screen boundaries.
* **How to Enable**: Check **Environment -> SSR** in the `WorldEnvironment` Inspector.

### Dual-Filter Bloom & Auto-Exposure
* **Dual-Filter Bloom**: Generates a 5-mip downsample/upsample compute blur pyramid to create smooth glow highlights around bright light sources.
* **Histogram Auto-Exposure**: Computes a 256-bin luminance histogram with smooth temporal adaptation over ~1 second.
* **ACES Tonemapping**: Maps HDR scene color to LDR display space using ACES filmic curve highlight preservation.
* **How to Enable**: Enable **Glow**, **Auto Exposure**, and set **Tonemap -> Mode** to `ACES` in `WorldEnvironment`.

---

## 6. Virtual Geometry & GPU Direct Submission

### Meshlet Cluster Culling
* **Nanite-Style Cluster Culling**: Decomposes dense mesh geometry into small meshlets with bounding spheres.
* **GPU Frustum & Backface Culling**: Performs hierarchical bounding sphere frustum and backface culling on compute shaders, discarding occluded geometry before rasterization.

### Multi-Draw Indirect (`ExecuteIndirect`)
* **Zero CPU Thread Overhead**: Globally caches `ID3D12CommandSignature` objects (`D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED`) to issue multi-draw indirect commands directly on the GPU.
* **Performance**: Submits thousands of instanced static objects with <0.1ms dispatch time.

---

## 7. Hardware Ray Tracing (DXR 1.1) & Global Illumination

### DXR 1.1 Acceleration Structures & Shader Binding Tables
* **Hardware DXR Activation**: Queries `ID3D12Device5` for DXR 1.1 support.
* **Dynamic TLAS Updates**: Rebuilds Top-Level Acceleration Structures (TLAS) dynamically using `D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE` to eliminate GPU memory stalls.
* **Hardware Dispatch**: Dispatches rays via `ID3D12GraphicsCommandList4::DispatchRays`.

### Real-Time Global Illumination (ZGI)
* **Dynamic Probe GI (ZGI)**: Real-time probe volume global illumination ([zgi.h](file:///z:/ZeGFX-Engine/ZeGFX/include/zgi.h)). Updates 3D probe grids containing **octahedral-mapped irradiance** and **radial distance fields** via hardware DXR or compute gather passes.
* **Multi-Bounce Indirect Bounce**: Computes real-time indirect bounce lighting, soft indirect shadows, and ambient color bleeding dynamically.
* **How to Enable in Editor**:
  1. Add a **`WorldEnvironment`** node to your scene.
  2. In the Inspector, enable **SDFGI** or **SSIL**.
  3. Indirect bounce lighting automatically routes through ZeGFX's **ZGI probe gather pipeline**!

---

## 8. Feature & Controls Quick Reference

| Graphics Subsystem | Under-the-Hood Technology | Editor Control Path |
| :--- | :--- | :--- |
| **Direct3D 12 Driver** | Native `ID3D12Device5` & Win32 Swapchain | `Project Settings -> Driver.Windows` |
| **Dynamic Uploads** | `DX12UploadRingBuffer` (`FRAME_COUNT = 3`) | Automatic |
| **Shader PSO Cache** | Disk binary serialization (`dx12_psos.bin`) | Automatic |
| **Clustered Deferred Shading** | 5-Channel MRT G-Buffer + `16x16` Tile Compute | `OmniLight3D` / `SpotLight3D` |
| **Volumetric Fog & God Rays** | `160x90x128` 3D Texture Froxel Compute | `WorldEnvironment -> Volumetric Fog` |
| **Shadows & VSM** | 4-Cascade PCF Atlases & Virtual Shadow Maps | `DirectionalLight3D -> Cascades` |
| **Ambient Occlusion** | Ground Truth Ambient Occlusion (GTAO) | `WorldEnvironment -> SSAO` |
| **Reflections** | Screen-Space Raymarched Reflections (SSR) | `WorldEnvironment -> SSR` |
| **Bloom & Tonemapping** | Dual-Filter Mip Pyramid & ACES Filmic | `WorldEnvironment -> Glow / Tonemap` |
| **Virtual Geometry** | Meshlet Cluster Culling & `ExecuteIndirect` | Automatic |
| **Hardware Ray Tracing** | DXR 1.1 `DispatchRays` & Acceleration Structs | `WorldEnvironment -> SSR (Metallic)` |
| **Global Illumination** | ZGI Dynamic Irradiance Probe Volumes | `WorldEnvironment -> SDFGI / SSIL` |
