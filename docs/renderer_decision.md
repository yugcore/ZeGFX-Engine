# Architectural Decision: Direct3D 12 & ZeGFX Pipeline Activation (Path 3A)

**Status**: Accepted  
**Date**: 2026-08-25  
**Context**: Forensic audit of the rendering subsystem revealed that Direct3D 12 is initialized as the primary driver on Windows, but the custom ZeGFX subsystems (DXR reflections, GPU Hi-Z occlusion, post-processing composite graph) have operated in a decoupled/deferred pass state.

---

### Decision Statement

The project will pursue **Path 3A (Activate the Real DX12/ZeGFX Pipeline)**. Rather than stripping custom Direct3D 12 extensions in favor of a standard upstream Godot RD wrapper, the engine will fully wire the ZeGFX rendering pipeline into Godot's RenderingDeviceDriverD3D12 architecture.

This decision commits the project to a sequential, phased engineering effort:

1. **Live Command List Extraction & Resource Barrier Synchronization**:
   - Extract and pass live `ID3D12GraphicsCommandList` instances from `RenderingDeviceDriverD3D12` / `RenderingDevice` into `ZeGFXD3D12Bridge::flush_deferred_passes()` instead of `nullptr`.
   - Ensure proper D3D12 resource state transitions between Godot RD passes and ZeGFX compute/graphics passes.

2. **Deferred Pass Execution & Camera Matrix Binding**:
   - Implement `RendererSceneD3D12::render_scene_deferred()` to bind live camera view, projection, and inverse matrices from active render views rather than hardcoded identity values.

3. **DXR 1.1 Acceleration Structure & Root Signature Binding**:
   - Resolve descriptor table slot bindings 1 & 2 in `DXRPipelineD3D12::dispatch_reflection_rays()` only after command list execution and GPU barrier transitions are live and stable.

4. **Multi-Backend Fallback Safety**:
   - All ZeGFX D3D12 bridge calls remain strictly guarded behind `#if defined(D3D12_ENABLED) && defined(WITH_DX12_BACKEND)`, ensuring non-Windows platforms (Linux, macOS, Mobile) and Vulkan/GLES3 builds continue to function without disruption.
