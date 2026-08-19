# ZeGFX Direct3D 12 Engine — Technical Manual & Architecture Guide

Welcome to the official engine documentation for the **ZeGFX Direct3D 12 Graphics Engine** powering Velvet Engine.

ZeGFX is a modern, high-performance C++20 rendering backend built on **Direct3D 12 (DX12)**, **Clustered Forward+ RD**, and **DirectX Raytracing (DXR 1.1)**. It features an asynchronous Render Graph DAG compiler, unified graphics quality presets, 128-tap Vogel Disk PCSS soft shadows, ACES Fitted HDR tonemapping, 3D raymarched volumetric clouds, a 24-hour astronomical celestial time-of-day system, dynamic weather state management with PBR surface wetness, subpixel Halton TAA + AMD FSR 2.2, and 16x anisotropic PBR shading.

---

## 1. High-Level Engine Architecture

ZeGFX interfaces directly with the native Win32 window subsystem and provides a unified, AAA-grade Direct3D 12 and Clustered Forward+ hardware pipeline:

```
+---------------------------------------------------------------------------------------------------+
|                                     VELVET ENGINE APPLICATION & EDITOR                            |
|        (3D Viewport / WorldEnvironment / Terrain3D / TimeOfDay3D / VolumetricClouds3D / UI)       |
+--------------------------------------------------+------------------------------------------------+
                                                   |
                                                   v (Scene Hierarchy, Scenario & Settings)
+--------------------------------------------------+------------------------------------------------+
|                                        RENDERING SERVER CORE                                      |
|  - Unified Graphics Quality Presets Engine (Low / Medium / High / Ultra)                          |
|  - Scene Culling, Camera Frustum & PSSM Logarithmic Split Manager                                 |
|  - Global Shader Parameter System (Weather Wetness, Puddles, Celestial Ephemeris)                 |
+--------------------------------------------------+------------------------------------------------+
                                                   |
                                                   v (Asynchronous Frame Commands)
+--------------------------------------------------+------------------------------------------------+
|                                    ZEGFX D3D12 & CLUSTERED RD PIPELINE                            |
|  - D3D12 Native Device RHI (ID3D12Device5) & Win32 Direct Swapchain                               |
|  - Asynchronous Render Graph DAG Compiler & Automatic Resource Barrier Tracker                    |
|  - 128-Tap Vogel Disk PCSS Soft Contact-Hardening Shadows + Screen-Space Contact Shadows (SSCS)   |
|  - High-Density 3D Froxel Volumetric Fog Grid (256x128 Depth Slices on Ultra)                     |
|  - Horizon-Based GTAO / SSAO with Multi-Pass Edge-Aware Bilateral Filter                          |
|  - GGX Microfacet Screen-Space Reflections (SSR) with Full-Resolution Raymarch                    |
|  - 3D Raymarched Volumetric Clouds (Perlin-Worley Erosion, Powder Effect, Mie Silver Lining)     |
|  - 24-Hour Astronomical Celestial Ephemeris (Sun/Moon Arcs, Kelvin Temperature Curve, Stars Dome) |
|  - Dynamic Weather Controller with PBR Surface Wetness Darkening & Puddle Accumulation            |
|  - ACES Fitted HDR Tonemapper, 3D LUT Color Grading & Dual-Filter Bloom (Karis Firefly Filter)    |
|  - Optical Bokeh Depth of Field with Golden-Angle Jittered Sampling (Zero Ring Banding)          |
|  - Subpixel Halton Jitter TAA (9-Tap Catmull-Rom History + Variance Clipping) + Native AMD FSR 2.2|
|  - Tokuyoshi & Kaplanyan Specular Roughness Limiter (Zero Normal-Map Shimmer)                    |
|  - 16x Anisotropic Texture Filtering on PBR Materials, Decals, and Projectors                    |
|  - Filament / Kulla-Conty Microfacet Multi-Scatter Energy Compensation                            |
|  - Jimenez Separable 25-Tap Multi-Dipole Subsurface Scattering (SSS)                              |
|  - DXR 1.1 Hardware Ray Tracing Acceleration Structures (BLAS/TLAS) & ZGI Probe Grid              |
+---------------------------------------------------------------------------------------------------+
```

---

## 2. Unified Graphics Quality Preset System

ZeGFX features an engine-wide **Unified Graphics Quality Preset** architecture supporting **`Low`**, **`Medium`**, **`High`**, and **`Ultra`** profiles.

### Preset Fidelity Matrix

| Subsystem | Low | Medium | High *(Default)* | Ultra |
| :--- | :--- | :--- | :--- | :--- |
| **Directional Shadow Atlas** | 2048 | 4096 | 4096 | **8192** |
| **Shadow Filter Quality** | 16-Tap PCF | 32-Tap PCF | 64-Tap Vogel Disk | **128-Tap Vogel PCSS** |
| **Screen-Space Contact Shadows** | Off | Fast (0.15) | Active (0.35) | **High-Res (0.50)** |
| **Volumetric Fog Froxel Grid** | $32 \times 32$ | $64 \times 64$ | $128 \times 64$ | **$256 \times 128$** |
| **SSAO / GTAO Quality** | Very Low (Half-Res, 1 Blur) | Medium (Half-Res, 2 Blur) | High (Full-Res, 3 Blur) | **Ultra (Full-Res, 4 Blur)** |
| **SSIL Quality** | Very Low | Medium | High | **Ultra** |
| **Screen-Space Reflections (SSR)** | Half-Res (Fast) | Half-Res (Filtered) | Full-Res GGX | **Full-Res GGX (Max Steps)** |
| **SDFGI Global Illumination** | 8 Rays / 5 Frames | 16 Rays / 10 Frames | 64 Rays / 20 Frames | **128 Rays / 30 Frames** |
| **Subsurface Scattering (SSS)** | Low (11 Samples) | Medium (17 Samples) | High (25 Samples) | **Ultra (25 Samples Full-Res)**|
| **Anisotropic Filtering** | 4x | 8x | 16x | **16x** |
| **Decal / Projector Filter** | Linear Mipmaps | Linear Mipmaps | 16x Anisotropic | **16x Anisotropic** |
| **Optical Bokeh DoF** | Circle (Fast) | Circle + Jitter | Circle + Jitter (HQ) | **Circle + Jitter (HQ Max)** |

### How to Control Presets in Editor & Code

1. **In the 3D Viewport Header**:
   * Click **`[View]`** (top-left of 3D Viewport) -> **`Quality Preset...`** -> Select **`Low`**, **`Medium`**, **`High`**, or **`Ultra`**.
   * An on-screen notification toast will confirm the switch with instant real-time visual feedback.
2. **In Project Settings**:
   * Open **Project Settings -> Rendering -> Quality -> Preset** -> Choose default starting preset.
3. **In GDScript / Runtime**:
   ```gdscript
   extends Node

   func _ready():
       # Switch to Ultra Preset
       RenderingServer.graphics_preset_apply(RenderingServer.GRAPHICS_PRESET_ULTRA)
       print("Current Graphics Preset: ", RenderingServer.graphics_preset_get())
   ```

---

## 3. Sky, Atmosphere, Volumetric Clouds & Dynamic Weather

### 3.1 3D Raymarched Volumetric Clouds (`VolumetricClouds3D`)
The `VolumetricClouds3D` node renders realistic 3D volumetric cloud layers using dual-layer raymarching and physical optics:
* **Perlin-Worley 3D Detail Erosion**: Dense cumulus cloud formations with natural billowing and edge wisps.
* **Optical Physics**: Beer-Lambert light absorption ($e^{-\sigma_t d}$), Henyey-Greenstein forward Mie phase function (solar silver lining highlights), and powder-sugar multi-scattering approximation.
* **Dynamic Wind Drift**: Real-time cloud evolution and drift along 3D wind velocity vectors.
* **Moving Ground Shadows**: Projects top-down cloud transmittance directly onto the terrain and directional shadow cascades.

#### Node Properties:
* `base_altitude`: Cloud layer base height (e.g. `1500m`).
* `cloud_thickness`: Vertical cloud thickness (e.g. `2500m`).
* `coverage` ($0.0 - 1.0$): Cloud volume coverage factor.
* `density` ($0.0 - 5.0$): Optical density and light absorption.
* `silver_lining_intensity`: Forward-scattering highlight boost around the sun.
* `wind_direction` & `wind_speed`: Real-time cloud drift velocity.
* `cast_shadows_on_ground`: Enables real-time terrain cloud shadow projection.

---

### 3.2 24-Hour Celestial Controller (`TimeOfDay3D`)
The `TimeOfDay3D` manager node provides a complete 24-hour astronomical day/night cycle:
* **Astronomical Ephemeris**: Calculates real-time solar declination, hour angle, elevation, and azimuth based on `time_of_day` ($0.0 - 24.0\text{h}$), latitude, and calendar day of the year.
* **Sun Lighting Synchronization**: Automatically rotates the linked `DirectionalLight3D` along the solar arc and shifts color temperature along a realistic Kelvin blackbody curve (warm 2000K sunrise/sunset $\rightarrow$ crisp 6500K noon).
* **Moon & Lunar Phases**: Tracks the opposing lunar orbit and drives night ambient illumination.
* **Celestial Star Dome**: Rotates the starry night sky dome based on planetary orientation.
* **Signals**: Emits `time_changed(time)`, `hour_passed(hour)`, `day_started`, and `night_started`.

```gdscript
extends Node3D

@onready var tod = $TimeOfDay3D

func _ready():
    tod.time_of_day = 6.5 # Sunrise
    tod.time_scale = 60.0 # 1 real second = 1 game minute
    tod.day_started.connect(_on_day_started)

func _on_day_started():
    print("Good morning! Sunrise has begun.")
```

---

### 3.3 Dynamic Weather & PBR Surface Wetness (`WeatherController3D`)
The `WeatherController3D` node manages dynamic weather state machines and environmental surface effects:
* **Supported Weather States**:
  * `WEATHER_CLEAR`
  * `WEATHER_PARTLY_CLOUDY`
  * `WEATHER_OVERCAST`
  * `WEATHER_RAIN`
  * `WEATHER_STORM`
  * `WEATHER_FOGGY`
  * `WEATHER_SNOW`
* **Smooth State Transitions**: Linearly interpolates cloud coverage, density, precipitation intensity, wind gusts, and fog density across configurable durations.
* **Dynamic PBR Surface Wetness & Puddles**: Drives global shader parameters (`weather_wetness`, `weather_puddle_amount`) in `scene_forward_clustered.glsl`:
  * Surfaces darken realistically as they absorb water.
  * Roughness drops, boosting specular reflections.
  * Puddle accumulation forms reflective water pools in ground crevices.

```gdscript
extends Node3D

@onready var weather = $WeatherController3D

func trigger_rain_storm():
    # Transition to Storm over 10 seconds
    weather.change_weather(WeatherController3D.WEATHER_STORM, 10.0)
```

---

## 4. Lighting, Shadows & Indirect Global Illumination

### 4.1 128-Tap Vogel Disk PCSS Soft Shadows
* **Contact-Hardening Penumbra**: Directional shadows calculate blocker distance and filter penumbras using a golden-angle Vogel spiral disk with up to 128 randomized sample taps on Ultra preset.
* **Logarithmic Split Cascading**: 4-cascade PSSM splits are calculated with logarithmic curvature to distribute shadow map resolution smoothly from close-up character details to distant horizons.
* **Shadow Atlases**: Up to **8192×8192** shadow maps on Ultra preset.

### 4.2 Screen-Space Contact Shadows (SSCS)
* Raymarches screen-space depth buffers along light direction vectors to resolve fine geometric contact shadows (under footsteps, pebbles, grass blades, and facial features) where shadow map resolution limits occur.

### 4.3 Horizon-Based GTAO & SSIL
* **Ground Truth Ambient Occlusion**: Computes screen-space horizon integrals to produce deep, physically grounded contact ambient occlusion without white halo artifacts.
* **Screen-Space Indirect Lighting (SSIL)**: Full-resolution multi-bounce diffuse light bounce between adjacent geometries.

### 4.4 High-Density 3D Froxel Volumetric Fog
* Generates a view-space 3D froxel volume ($256\times128$ depth slices on Ultra) with Mie forward-scattering phase functions, direct sun ray injection, and clustered point/spot light scattering for volumetric god rays.

---

## 5. Color, Tonemapping & Post-Processing Suite

### 5.1 ACES Fitted HDR Tonemapping & 3D LUT Color Grading
* **Stephen Hill / MJP ACES Fitted Curve**: Preserves HDR highlight rolloff, contrast, and gamut mapping without oversaturation or highlight blowout.
* **3D LUT Grading**: Decoupled 3D Color Correction LUT volume mapping for filmic color grading and stylized aesthetics.

### 5.2 Dual-Filter Bloom with Karis Firefly Suppression
* Employs a 13-tap Karis luminance-weighted downsampling and 9-tap 3x3 tent upsampling blur pyramid with bicubic upscaling, completely suppressing specular firefly sparkle noise.

### 5.3 Physical Optical Bokeh Depth of Field
* Circular optical bokeh disc generation with golden-angle randomized jittered sampling (`use_jitter`), eliminating concentric ring banding artifacts in out-of-focus bokeh highlights.

### 5.4 8-Bit & 10-Bit Screen-Space Debanding
* Applies high-frequency triangular noise dithering with `/ 255.0` (8-bit SDR) and `/ 1023.0` (10-bit HDR) quantization, eliminating banding gradients on skies and shadows.

---

## 6. Anti-Aliasing & Temporal Stability

### 6.1 Subpixel Halton Jitter TAA + Velocity Reprojection
* **Halton Sequence Subpixel Jitter**: Jitters camera projection matrices across subpixel phases.
* **9-Tap Catmull-Rom History Resolve**: Filters history buffers using 9-tap bicubic Catmull-Rom interpolation to maintain edge sharpness under camera and object motion.
* **Dynamic 3x3 Variance Clipping**: Closest-depth velocity dilation and dynamic variance AABB clipping completely eliminate ghosting artifacts behind moving objects.

### 6.2 Native AMD FSR 2.2 Temporal Upscaling
* Full AMD FidelityFX Super Resolution 2.2 integration supporting Native AA (1.0x display resolution reconstruction) and scaling profiles (1.3x Ultra Quality, 1.5x Quality) with RCAS contrast-adaptive sharpening.

### 6.3 Specular Roughness Limiter (Zero Normal-Map Shimmer)
* Implements Tokuyoshi & Kaplanyan Geometric Specular AA: calculates screen-space normal derivatives ($dFdx / dFdy$) to adaptively bias GGX microfacet specular roughness on high-frequency normal maps, eliminating specular aliasing fireflies.

---

## 7. Materials & Shading Fidelity

### 7.1 16x Anisotropic Texture Filtering
* Automatically constructs 16x anisotropic sampler states with negative mipmap LOD bias compensation for all PBR materials, clustered Decals, and light projectors.

### 7.2 Microfacet Multi-Scatter Energy Compensation
* Implements Filament / Kulla-Conty energy compensation (`1.0 + f0 * (1.0 / max(1e-4, env) - 1.0)`), restoring lost inter-microfacet bounce energy on high-roughness metals and dielectrics.

### 7.3 Jimenez Separable Subsurface Scattering (SSS)
* Multi-dipole subsurface scattering with 25-sample skin kernels across High and Ultra presets, providing photorealistic skin, wax, and marble translucency with backscatter transmission.

### 7.4 Dual-Lobe Clearcoat & Anisotropic Highlights
* Secondary clearcoat reflection lobe for car paint and lacquer, paired with flow-map driven anisotropic specular highlights for brushed metals, hair, and carbon fiber.

---

## 8. Landscape, Foliage & World Systems

* **`Terrain3D`**: Native multi-chunk heightmap landscape system with 16-bit linear heightmap loading, continuous CDLOD perimeter skirts (zero-crack seams), triplanar cliff texturing, real-time in-viewport brush sculpting, and synchronized `StaticBody3D` / `HeightMapShape3D` physics collision.
* **`Grass3D` & `Foliage3D`**: High-density GPU-instanced grass clumps and vegetation with automatic terrain height snapping, distance-based culling, wind flutter, and interactive player trample collision push-back.
* **`WorldPartition3D`**: Multi-threaded background grid streaming pool for massive open worlds.
* **`FloatingOrigin3D`**: Automated origin rebasing with 64-bit double precision transform math for jitter-free large-world coordinates.

---

## 9. Feature & Class Quick Reference

| Class / Subsystem | Category | Description |
| :--- | :--- | :--- |
| **`VolumetricClouds3D`** | Sky & Atmosphere | 3D raymarched volumetric clouds with noise erosion, silver lining & ground shadows. |
| **`TimeOfDay3D`** | Sky & Atmosphere | 24-hour astronomical celestial cycle controller driving Sun, Moon, and Stars. |
| **`WeatherController3D`**| Sky & Atmosphere | Dynamic weather state machine with smooth transitions & PBR surface wetness. |
| **`Terrain3D`** | World & Terrain | Multi-chunk terrain with CDLOD skirts, triplanar texturing, and live sculpting. |
| **`Grass3D`** | Foliage | Procedural GPU-instanced grass with terrain snapping & player trample physics. |
| **`Foliage3D`** | Foliage | Scalable chunk-based vegetation and custom tree/rock instancing. |
| **`WorldPartition3D`** | World Streaming | Asynchronous 2D spatial grid streaming worker pool for open worlds. |
| **`FloatingOrigin3D`** | World Space | Double-precision large-world coordinate origin rebasing. |
| **`RenderingServer`** | Rendering API | Controls graphics presets (`GRAPHICS_PRESET_LOW` to `GRAPHICS_PRESET_ULTRA`). |
