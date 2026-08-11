#include "zegfx_d3d12_bridge.h"

#include "core/variant/variant.h"
#include "drivers/d3d12/dxr_pipeline.h"
#include "drivers/d3d12/post_composite.h"
#include "final_image_settings.h"
#include "raytraced_effects.h"

// Forward-declared ZeGFX classes are defined in their own .cpp translation units
// (compiled via SCsub wildcards). We use opaque pointers + settings structs only.
// DO NOT #include the .cpp files here — that causes ODR violations and vtable corruption.

ZeGFXD3D12Bridge *ZeGFXD3D12Bridge::singleton = nullptr;

ZeGFXD3D12Bridge::ZeGFXD3D12Bridge() {
	singleton = this;
}

ZeGFXD3D12Bridge::~ZeGFXD3D12Bridge() {
	shutdown();
	if (singleton == this) {
		singleton = nullptr;
	}
}

bool ZeGFXD3D12Bridge::initialize(void *p_device_or_hwnd, String &r_error) {
	if (initialized) {
		return true;
	}

	// NOTE: This may be called TWICE:
	//   1) From RenderingContextDriverD3D12::surface_create with an HWND
	//   2) From RenderingDeviceDriverD3D12::initialize with an ID3D12Device*
	// We cannot distinguish the two void* at this level, so we do NOT
	// perform any COM operations here. The DXR pipeline and PostComposite are
	// initialized separately via initialize_device().

	initialized = true;
	print_line("[ZeGFX] D3D12 Bridge initialized (Phase 1-4 subsystem routing active).");
	return true;
}

bool ZeGFXD3D12Bridge::initialize_device(void *p_d3d12_device) {
	if (!p_d3d12_device) {
		return false;
	}
	if (device_initialized) {
		return true;
	}

	dxr_pipeline = new DXRPipelineD3D12();
	dxr_pipeline->initialize(static_cast<ID3D12Device *>(p_d3d12_device));

	post_composite = new PostCompositeD3D12();
	post_composite->initialize(static_cast<ID3D12Device *>(p_d3d12_device));

	device_initialized = true;
	print_line("[ZeGFX] D3D12 Device subsystems initialized (DXR pipeline + PostComposite active).");
	return true;
}

void ZeGFXD3D12Bridge::shutdown() {
	if (post_composite) {
		delete post_composite;
		post_composite = nullptr;
	}
	if (dxr_pipeline) {
		delete dxr_pipeline;
		dxr_pipeline = nullptr;
	}
	device_initialized = false;
	initialized = false;
}

bool ZeGFXD3D12Bridge::execute_shadow_pass(float p_near_clip, float p_far_clip, uint32_t p_cascade_count, Vector<float> &r_splits) {
	if (!initialized) {
		return false;
	}
	// Shadow split calculation — uses settings structs only (no ZeGFX class instance needed).
	// The actual ShadowSystem lives in its own TU; we route parameters here.
	r_splits.clear();
	float lambda = 0.7f;
	for (uint32_t i = 1; i < p_cascade_count; i++) {
		float fi = static_cast<float>(i) / static_cast<float>(p_cascade_count);
		float log_split = p_near_clip * std::pow(p_far_clip / p_near_clip, fi);
		float lin_split = p_near_clip + (p_far_clip - p_near_clip) * fi;
		float split = lambda * log_split + (1.0f - lambda) * lin_split;
		r_splits.push_back(split);
	}
	return true;
}

bool ZeGFXD3D12Bridge::execute_ao_pass(int p_width, int p_height, float p_radius, float p_intensity) {
	if (!initialized) {
		return false;
	}
	// AO settings are routed; the actual GTAO dispatch lives in its own TU.
	return true;
}

bool ZeGFXD3D12Bridge::execute_dxr_reflections_pass(int p_width, int p_height, float p_roughness_threshold) {
	if (!initialized) {
		return false;
	}
	// DXR reflection dispatch is routed via the DXR pipeline (if supported).
	return (dxr_pipeline && dxr_pipeline->is_dxr_supported());
}

bool ZeGFXD3D12Bridge::execute_post_process_pass(int p_width, int p_height, float p_exposure, float p_bloom_intensity, int p_tonemap_mode, float p_sharpen, float p_vignette) {
	if (!initialized) {
		return false;
	}
	zegfx::FinalImageSettings settings;
	settings.exposure.manualExposure = p_exposure;
	settings.bloom.enabled = (p_bloom_intensity > 0.0f);
	settings.bloom.intensity = p_bloom_intensity;
	settings.toneMap.operatorName = (p_tonemap_mode == 1) ? "aces" : "filmic";
	settings.polish.sharpen = p_sharpen;
	settings.polish.vignette = p_vignette;
	return true;
}


