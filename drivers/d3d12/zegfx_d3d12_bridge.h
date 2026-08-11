/**************************************************************************/
/*  zegfx_d3d12_bridge.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifndef ZEGFX_D3D12_BRIDGE_H
#define ZEGFX_D3D12_BRIDGE_H

#include "core/string/ustring.h"
#include "core/templates/vector.h"

class DXRPipelineD3D12;
class PostCompositeD3D12;

// Bridge singleton wiring ZeGFX subsystems into Godot's
// RenderingDeviceDriverD3D12 while preserving 100% of editor UI & gizmos.

class ZeGFXD3D12Bridge {
private:
	static ZeGFXD3D12Bridge *singleton;
	bool initialized = false;
	bool device_initialized = false;
	DXRPipelineD3D12 *dxr_pipeline = nullptr;
	PostCompositeD3D12 *post_composite = nullptr;

public:
	static ZeGFXD3D12Bridge *get_singleton() { return singleton; }

	ZeGFXD3D12Bridge();
	~ZeGFXD3D12Bridge();

	bool initialize(void *p_device_or_hwnd, String &r_error);
	bool initialize_device(void *p_d3d12_device);
	void shutdown();

	bool is_initialized() const { return initialized; }

	// Phase 1 Subsystem Swap: godotShadow -> zegfxShadow
	bool execute_shadow_pass(float p_near_clip, float p_far_clip, uint32_t p_cascade_count, Vector<float> &r_splits);

	// Phase 2 Subsystem Swap: godotAO -> zegfxAO (GTAO)
	bool execute_ao_pass(int p_width, int p_height, float p_radius, float p_intensity);

	// Phase 3 Subsystem Swap: godotSSR -> zegfxDXR
	bool execute_dxr_reflections_pass(int p_width, int p_height, float p_roughness_threshold);

	// Phase 4 Subsystem Swap: godotPost -> zegfxPost (Bloom + Auto-Exposure + ACES Tonemap + Sharpening)
	bool execute_post_process_pass(int p_width, int p_height, float p_exposure, float p_bloom_intensity, int p_tonemap_mode, float p_sharpen, float p_vignette);

	// Phase 5 Subsystem Swap: Cooked .zmesh V2 Meshlets & Dynamic LOD Streamer
	bool execute_meshlet_streamer_pass(const String &p_zmesh_path, uint32_t p_lod_level, uint32_t p_instance_count);
	bool cook_and_load_zmesh(const String &p_source_file, const String &p_output_zmesh);
};

#endif // ZEGFX_D3D12_BRIDGE_H

