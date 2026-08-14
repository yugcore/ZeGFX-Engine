# Godot Fork (DXR 1.1 / ZeGFX) — Phased Implementation Plan

Based on confirmed findings from the codebase audit. Ordered by (impact / implementation cost), not by dependency alone — Phase 1 is the single highest-leverage fix and is almost fully isolated from everything else.

---

## Phase 0 — Instrumentation baseline (do this first, before any fix)

You already have the data sources; they're just not wired into a visible baseline yet.

- Surface `ZeGFX::RendererDiagnostics` (`zegfx.cpp`) — `pipelineCompileHits`, `pipelineCompileMisses`, `pipelineCompileFailures`, `blasBytes`, `tlasBytes`, `drawCallCount`, `dispatchCount` — into an on-screen overlay or a logged CSV per session.
- Add explicit timers around the two `waitForGpu()` call sites (`dx12_dxr.cpp:235` in `buildBLAS`, `dx12_dxr.cpp:396` in `buildTLAS`) so you can quote a hard "ms lost per stall" number before you touch them.
- Combine with Godot's `Performance` singleton (`TIME_PROCESS`, `TIME_PHYSICS_PROCESS`) for a full-frame picture.

**Exit criteria:** you can reproduce a sector-load hitch and print "N ms in buildBLAS wait, M ms in buildTLAS wait, K pipeline compile misses" for that single event. Every later phase is graded against this number.

---

## Phase 1 — Kill the synchronous `waitForGpu()` stalls (highest ROI, most isolated)

This is the actual cause of the hitch in the "new sector enters range" path — not a design gap, a specific blocking call.

**Target:** `buildBLAS` (`dx12_dxr.cpp:235`) and `buildTLAS` (`dx12_dxr.cpp:396`).

- Replace the blocking `waitForGpu()` with a fence-based async pattern: submit the build command list, record a fence value, return immediately. Track "structure ready" via fence completion rather than blocking the CPU thread on it.
- Gate renderability on the fence signal, not on function return — i.e., the instance shouldn't be marked visible/traced-against until its BLAS fence (and the subsequent TLAS fence that includes it) has signaled. This likely means adding a small pending-state flag to `DX12ModelData`/`DX12TlasInstance` rather than restructuring the caller.
- If your D3D12 device exposes a copy/compute queue separate from the direct queue, move BLAS builds there entirely — BLAS build is a background operation, it doesn't need to share the queue actively submitting draw calls.
- Do **not** combine this with Phase 2 (rebuild→refit) in the same change — you want to isolate and measure the async-submission win by itself first, since it's the bigger of the two effects and you want a clean before/after number.

**Exit criteria:** re-run the Phase 0 reproduction case. The CPU-side stall should drop to near-zero even though the *work* (BLAS/TLAS build) still happens — you've moved it off the critical path, not made it cheaper yet. That cost-reduction is Phase 2.

---

## Phase 2 — TLAS refit instead of full rebuild

**Target:** `buildTLAS` (`dx12_dxr.cpp:343`), currently hardcoded to `D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE` with no `ALLOW_UPDATE`/`PERFORM_UPDATE`.

- Add the `ALLOW_UPDATE` flag to the initial TLAS build.
- Split the update path into two cases:
  - **Transform-only changes** (existing instances moving) → `PERFORM_UPDATE` refit. Cheap, should be near-free even done per-frame.
  - **Instance count changes** (new sector streamed in, instance added/removed) → still needs a rebuild, but batch these: accumulate instance adds/removes over a frame (or a few frames) and do one rebuild covering all of them, instead of one rebuild per `zfx_renderer_rtTlasAddInstance` call.
- This directly addresses the "TLAS insert triggers → rebuild call" pattern the audit found — right now every single instance spawn is a full rebuild; batching + refit-for-transforms should cut both the frequency and the average cost of full rebuilds substantially.

**Exit criteria:** streaming in a sector with N new instances should trigger one rebuild covering all N, not N rebuilds. Per-frame instance transform updates (moving objects) should use refit, measurably cheaper than rebuild in your Phase 0 timers.

---

## Phase 3 — Raster PSO warm-up pass

**Target:** `PipelineCacheRD::_generate_version` (`pipeline_cache_rd.cpp:58`) — currently first-draw lazy compilation, confirmed no warm-up exists.

- Add a scene/level-load step: walk all materials referenced by the scene about to become active, and for each, force-generate the PSO combinations you know will be needed (the pass-mode × MSAA × vertex-format combinations already enumerated in the audit — `MODE_RENDER` variants, fog/SDFGI/VoxelGI/lightmap flags, etc.).
- Run this during a loading screen or a "streaming ahead" window (if Phase 2's batched sector loading gives you a lead time before the sector becomes visible, warm its PSOs during that lead time).
- You already have persistent bytecode caching (`ShaderRD::_load_from_cache`, `dx12_psos.bin`) — this phase is about *forcing instantiation* of `ID3D12PipelineState` objects from that cache proactively, not building a new caching layer.
- Use the Phase 0 `pipelineCompileMisses` counter as your success metric: it should approach zero during actual gameplay once warm-up is working, with all the misses front-loaded into the warm-up window instead.

---

## Phase 4 — RT hit-group PSO warm-up

**Target:** `DX12Backend::buildStateObject` (`dx12_dxr.cpp`) — confirmed no warm-up for material-variant RT hit groups.

- Same approach as Phase 3, applied to hit-group state objects: precompile hit groups for every material that can appear under the RT toggle, during the same loading/streaming-ahead window.
- Separate decision to evaluate (not necessarily do yet): whether to migrate `rtShadowTrace`/`rtReflectionTrace` from the classic RT pipeline object model to DXR 1.1 inline ray tracing (`RayQuery`) for effects that don't need the full pipeline object's flexibility. This would let those effects reuse the *raster* PSO cache instead of maintaining a fully separate RT PSO system — a real simplification, but a bigger structural change. Worth a spike/prototype after Phase 4 ships, not before.

---

## Phase 5 — Predictive streaming system

Only start this once Phases 1–4 are shipped and measured — it's the biggest single piece of new engineering, and Phases 1–2 will remove most of the *severity* of hitches even under the current purely-reactive loading, buying you room to build this properly rather than urgently.

- Build the spatial partitioning layer the audit confirms doesn't exist (grid or octree of sectors).
- Add velocity/heading-based prediction on top of `ResourceLoader.load_threaded_request` so sectors begin loading before the player reaches their trigger radius — this lead time is what Phase 3/4's warm-up passes need to actually run ahead of visibility rather than on-demand.
- Feed sector entry/exit directly into Phase 2's batched TLAS update path (one batched update per sector transition, not per-instance).

---

## Phase 6 — Hygiene / free wins (parallelizable, do anytime)

Independent of the above, no dependencies:

- Strip unused compiled-in modules confirmed by the audit: `MODULE_OPENXR_ENABLED`, `MODULE_WEBXR_ENABLED`, `MODULE_MOBILE_VR_ENABLED` (if no XR support shipped), 2D physics/navigation/tilemap modules (if 3D-only), unused networking modules (`ENET`/`WEBRTC`/`WEBSOCKET`/`UPNP` if unused), unused decoders (`THEORA`, `SVG`, `BMP` if unused).
- Add BLAS/TLAS VRAM as an explicit budgeted line item using the existing `blasBytes`/`tlasBytes` diagnostics — you already have the formulas (BLAS: `tris*64+1024` result / `tris*32+512` scratch; TLAS: `instances*64+4096` result / `instances*32+1024` scratch), just needs a budget ceiling + warning threshold wired to it.

---

## Summary ordering

| Phase | Fix | Why this order |
|---|---|---|
| 0 | Instrumentation | Need a baseline before claiming any win |
| 1 | Remove `waitForGpu()` blocking stalls | Single largest, most isolated fix — pure stall removal |
| 2 | TLAS refit + batched rebuilds | Reduces the *cost* of the work Phase 1 made async |
| 3 | Raster PSO warm-up | Isolated addition, no dependency on 1/2 |
| 4 | RT hit-group PSO warm-up | Same pattern as 3, applied to RT |
| 5 | Predictive streaming | Biggest new system — do once 1–4 lower the stakes |
| 6 | Module cleanup + VRAM budget | No dependencies, fits in anywhere |

Phase 1 alone should produce the most noticeable, measurable improvement for the least code changed — it's a synchronous-to-async conversion around two specific, already-located call sites, not a redesign.
