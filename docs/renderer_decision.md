# Architectural Decision: Direct3D 12 & ZeGFX Pipeline Activation (Path 3A)

**Status**: Accepted  
**Date**: 2026-08-25  
**Context**: Forensic audit of the rendering subsystem revealed that Direct3D 12 is initialized as the primary driver on Windows, but the custom ZeGFX subsystems (DXR reflections, GPU Hi-Z occlusion, post-processing composite graph) have operated in a decoupled/deferred pass state.

---

### Decision Statement

The project will pursue **Path 3A (Activate the Real DX12/ZeGFX Pipeline)**. Rather than stripping custom Direct3D 12 extensions in favor of a standard upstream Godot RD wrapper, the engine will fully wire the ZeGFX rendering pipeline into Godot's RenderingDeviceDriverD3D12 architecture.

This decision commits the project to a sequential, phased engineering effort:

1. **Phase 3A.1 (Completed): Live Command List Extraction & Camera Matrix Binding**:
   - Extracted and passed live `ID3D12GraphicsCommandList` instances from `RenderingDeviceDriverD3D12` into `ZeGFXD3D12Bridge::flush_deferred_passes()`.
   - Bound active camera view, projection, inverse matrices, and frustum planes to `zegfx::LightGridManager::SetCameraMatrices()`.

2. **Phase 5 (Next): Live G-Buffer & GPU Resource Barrier Synchronization (Geometry & Barriers Alone)**:
   - Transition scene mesh geometry draw call recording to the live D3D12 command list in `RendererSceneD3D12`.
   - Implement explicit `CD3DX12_RESOURCE_BARRIER` state transitions between Godot RD render passes and ZeGFX compute passes.
   - Ship and re-audit Phase 5 in complete isolation before touching ray tracing.

3. **Phase 6 (Subsequent): Hardware DXR 1.1 Reflection Dispatch & Descriptor Table Binding**:
   - Build live Top-Level Acceleration Structures (TLAS) from active scene instances.
   - Resolve descriptor table slot bindings 1 (TLAS SRV) & 2 (Output UAV) in `DXRPipelineD3D12::dispatch_reflection_rays()`.
   - Enable hardware ray dispatch strictly as its own isolated phase after Phase 5 geometry and barriers are verified crash-free.

4. **Multi-Backend Fallback Safety**:
   - All ZeGFX D3D12 bridge calls remain strictly guarded behind `#if defined(D3D12_ENABLED) && defined(WITH_DX12_BACKEND)`, ensuring non-Windows platforms (Linux, macOS, Mobile) and Vulkan/GLES3 builds continue to function without disruption.
