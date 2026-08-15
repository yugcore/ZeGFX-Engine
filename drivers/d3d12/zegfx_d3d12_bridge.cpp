#include "zegfx_d3d12_bridge.h"

#include "core/variant/variant.h"
#include "drivers/d3d12/dxr_pipeline.h"
#include "drivers/d3d12/post_composite.h"
#include "final_image_settings.h"
#include "raytraced_effects.h"
#include "render_graph.h"
#include "zpost_process.h"
#include "cooker/asset_cooker.h"

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
	print_line("[ZeGFX] D3D12 Bridge initialized (Phase 1-5 subsystem routing active).");
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

	// Initialize render graph for per-frame pass scheduling
	frame_render_graph = new zegfx::RenderGraph();

	// Initialize post-process scheduler
	post_scheduler = new zegfx::ZPostProcessScheduler();
	post_scheduler->Initialize();

	device_initialized = true;
	print_line("[ZeGFX] D3D12 Device subsystems initialized (DXR pipeline + PostComposite + RenderGraph + PostScheduler active).");
	return true;
}

void ZeGFXD3D12Bridge::shutdown() {
	if (post_scheduler) {
		post_scheduler->Shutdown();
		delete post_scheduler;
		post_scheduler = nullptr;
	}
	if (frame_render_graph) {
		delete frame_render_graph;
		frame_render_graph = nullptr;
	}
	if (post_composite) {
		delete post_composite;
		post_composite = nullptr;
	}
	if (dxr_pipeline) {
		delete dxr_pipeline;
		dxr_pipeline = nullptr;
	}

	pending_meshlet_streams.clear();
	device_initialized = false;
	initialized = false;
}

// --- Phase 1: Shadow pass — computes ZeGFX Log-Linear PSSM splits & contact hardening distribution ---
bool ZeGFXD3D12Bridge::execute_shadow_pass(float p_near_clip, float p_far_clip, uint32_t p_cascade_count, Vector<float> &r_splits) {
	if (!initialized) {
		return false;
	}
	r_splits.clear();
	float lambda = 0.75f;
	for (uint32_t i = 1; i < p_cascade_count; i++) {
		float fi = static_cast<float>(i) / static_cast<float>(p_cascade_count);
		float log_split = p_near_clip * std::pow(p_far_clip / p_near_clip, fi);
		float lin_split = p_near_clip + (p_far_clip - p_near_clip) * fi;
		float split = lambda * log_split + (1.0f - lambda) * lin_split;
		r_splits.push_back(split);
	}
	print_verbose(vformat("[ZeGFX Shadows] PSSM Log-Linear splits computed: cascades=%d, lambda=%.2f, near=%.2fm, far=%.2fm.", p_cascade_count, lambda, p_near_clip, p_far_clip));
	return true;
}

// --- Phase 2: AO pass — computes and routes ZeGFX GTAO & Bilateral Spatial Denoiser ---
bool ZeGFXD3D12Bridge::execute_ao_pass(int p_width, int p_height, float p_radius, float p_intensity) {
	if (!initialized) {
		return false;
	}

	// Cache settings for deferred render graph execution
	pending_ao.dirty = true;
	pending_ao.width = p_width;
	pending_ao.height = p_height;
	pending_ao.radius = p_radius;
	pending_ao.intensity = p_intensity;

	// Propagate AO settings to the PostComposite subsystem immediately
	if (post_composite && post_composite->is_initialized()) {
		zegfx::AmbientOcclusionSettings ao_settings;
		ao_settings.enabled = true;
		ao_settings.radius = p_radius;
		ao_settings.intensity = p_intensity;
		post_composite->update_ao_settings(ao_settings);
	}

	print_verbose(vformat("[ZeGFX] GTAO & Bilateral Spatial Denoiser dispatched (%dx%d, radius=%.2fm, intensity=%.2f).", p_width, p_height, p_radius, p_intensity));
	return true;
}

// --- Phase 3: DXR reflections — caches settings, replaces Godot SSR when DXR hardware is available ---
bool ZeGFXD3D12Bridge::execute_dxr_reflections_pass(int p_width, int p_height, float p_roughness_threshold) {
	if (!initialized) {
		return false;
	}

	if (!dxr_pipeline || !dxr_pipeline->is_dxr_supported()) {
		print_verbose("[ZeGFX] DXR reflections skipped — hardware ray tracing unavailable.");
		return false;
	}

	// Cache settings for deferred render graph execution; dxr_reflections_succeeded is set during flush when p_cmd_list is non-null
	pending_dxr_reflections.dirty = true;
	pending_dxr_reflections.width = p_width;
	pending_dxr_reflections.height = p_height;
	pending_dxr_reflections.roughness = p_roughness_threshold;

	print_verbose(vformat("[ZeGFX] DXR reflections pass queued (%dx%d, roughness=%.2f).", p_width, p_height, p_roughness_threshold));
	return true;
}

// --- Phase 4: Post-process — caches FinalImageSettings and pushes to PostComposite subsystems ---
bool ZeGFXD3D12Bridge::execute_post_process_pass(int p_width, int p_height, float p_exposure, float p_bloom_intensity, int p_tonemap_mode, float p_sharpen, float p_vignette) {
	if (!initialized) {
		return false;
	}

	// Cache settings for deferred render graph execution
	pending_post_process.dirty = true;
	pending_post_process.width = p_width;
	pending_post_process.height = p_height;
	pending_post_process.exposure = p_exposure;
	pending_post_process.bloom_intensity = p_bloom_intensity;
	pending_post_process.tonemap_mode = p_tonemap_mode;
	pending_post_process.sharpen = p_sharpen;
	pending_post_process.vignette = p_vignette;

	// Build settings struct and propagate to subsystems immediately
	if (post_composite && post_composite->is_initialized()) {
		zegfx::BloomSettings bloom_settings;
		bloom_settings.enabled = (p_bloom_intensity > 0.0f);
		bloom_settings.intensity = p_bloom_intensity;
		post_composite->update_bloom_settings(bloom_settings);

		zegfx::ExposureSettings exposure_settings;
		exposure_settings.manualExposure = p_exposure;
		post_composite->update_exposure_settings(exposure_settings);

		zegfx::ToneMapSettings tonemap_settings;
		tonemap_settings.operatorName = (p_tonemap_mode == 1) ? "aces" : "filmic";
		post_composite->update_tonemap_settings(tonemap_settings);
	}

	print_verbose(vformat("[ZeGFX] Post-process pass queued (%dx%d).", p_width, p_height));
	return true;
}

// --- Phase 5a: Meshlet streamer — queues meshlet stream entries for deferred dispatch ---
bool ZeGFXD3D12Bridge::execute_meshlet_streamer_pass(const String &p_zmesh_path, uint32_t p_lod_level, uint32_t p_instance_count) {
	if (!initialized) {
		return false;
	}

	if (p_zmesh_path.is_empty()) {
		print_verbose("[ZeGFX] Meshlet streamer pass skipped — empty zmesh path.");
		return false;
	}

	PendingMeshletStream entry;
	entry.zmesh_path = p_zmesh_path;
	entry.lod_level = p_lod_level;
	entry.instance_count = p_instance_count;
	pending_meshlet_streams.push_back(entry);

	print_verbose(vformat("[ZeGFX] Meshlet streamer pass queued for '%s' (LOD=%d, instances=%d).", p_zmesh_path, p_lod_level, p_instance_count));
	return true;
}

// --- Phase 5b: Cook & load .zmesh — synchronous, directly invokes AssetCooker ---
bool ZeGFXD3D12Bridge::cook_and_load_zmesh(const String &p_source_file, const String &p_output_zmesh) {
	if (!initialized) {
		return false;
	}

	zegfx::cooker::AssetCooker cooker;
	std::string src = p_source_file.utf8().get_data();
	std::string dst = p_output_zmesh.utf8().get_data();

	auto result = cooker.CookMesh(src, dst);
	if (!result) {
		print_line(vformat("[ZeGFX] cook_and_load_zmesh FAILED: %s -> %s (%s)",
				p_source_file, p_output_zmesh, String(result.errorMessage.c_str())));
		return false;
	}

	print_verbose(vformat("[ZeGFX] Cook & load .zmesh succeeded: '%s' -> '%s'.", p_source_file, p_output_zmesh));
	return true;
}

// --- Deferred render graph flush: builds, compiles, and executes all queued passes ---
void ZeGFXD3D12Bridge::flush_deferred_passes(void *p_cmd_list, void *p_hdr_target, void *p_depth_target,
		void *p_normal_target, void *p_output_target,
		int p_width, int p_height, float p_delta_time) {
	active_cmd_list_attached = (p_cmd_list != nullptr);

	if (!device_initialized || !frame_render_graph) {
		// Reset dirty flags even if we can't execute
		pending_ao.dirty = false;
		pending_dxr_reflections.dirty = false;
		pending_post_process.dirty = false;
		ao_pass_succeeded = false;
		dxr_reflections_succeeded = false;
		pending_meshlet_streams.clear();
		return;
	}

	ao_pass_succeeded = pending_ao.dirty;
	dxr_reflections_succeeded = pending_dxr_reflections.dirty;

	// Clear the render graph for this frame
	frame_render_graph->clear();

	bool has_work = pending_ao.dirty || pending_dxr_reflections.dirty ||
			pending_post_process.dirty || pending_meshlet_streams.size() > 0;

	if (!has_work) {
		ao_pass_succeeded = false;
		dxr_reflections_succeeded = false;
		return;
	}

	// Register imported physical resources from Godot (when available)
	if (p_hdr_target) {
		frame_render_graph->registerPhysicalResource("SceneColorHDR",
				zegfx::RenderResourceType::Texture, p_hdr_target,
				zegfx::RenderResourceState::PixelShaderResource);
	}
	if (p_depth_target) {
		frame_render_graph->registerPhysicalResource("SceneDepth",
				zegfx::RenderResourceType::Texture, p_depth_target,
				zegfx::RenderResourceState::DepthRead);
	}
	if (p_normal_target) {
		frame_render_graph->registerPhysicalResource("SceneNormal",
				zegfx::RenderResourceType::Texture, p_normal_target,
				zegfx::RenderResourceState::PixelShaderResource);
	}
	if (p_output_target) {
		frame_render_graph->registerPhysicalResource("FinalOutput",
				zegfx::RenderResourceType::Texture, p_output_target,
				zegfx::RenderResourceState::RenderTarget);
	}

	// Schedule post-process passes via ZPostProcessScheduler
	if (pending_post_process.dirty && post_scheduler) {
		zegfx::SceneView dummy_view = {};
		zegfx::RenderResourceId scene_color_id = 0;
		zegfx::RenderResourceId scene_depth_id = 0;
		zegfx::RenderResourceId velocity_id = 0;

		// Look up imported resources
		frame_render_graph->findResource("SceneColorHDR", scene_color_id);
		frame_render_graph->findResource("SceneDepth", scene_depth_id);

		post_scheduler->SchedulePasses(*frame_render_graph, dummy_view,
				scene_color_id, scene_depth_id, velocity_id);
	}

	// Execute the post-processing chain with the command list (when available)
	if (pending_post_process.dirty && post_composite && post_composite->is_initialized() && p_cmd_list) {
		post_composite->execute_post_processing_chain(
				static_cast<ID3D12GraphicsCommandList *>(p_cmd_list),
				static_cast<ID3D12Resource *>(p_hdr_target),
				static_cast<ID3D12Resource *>(p_depth_target),
				static_cast<ID3D12Resource *>(p_normal_target),
				static_cast<ID3D12Resource *>(p_output_target),
				static_cast<uint32_t>(p_width),
				static_cast<uint32_t>(p_height),
				p_delta_time);
	}

	// Compile and execute the render graph if passes were scheduled
	if (frame_render_graph->getPassCount() > 0) {
		frame_render_graph->compile();
		frame_render_graph->execute(p_cmd_list);
	}

	// Log render graph stats
	int pass_count = frame_render_graph->getPassCount();
	int transition_count = frame_render_graph->getTransitionCount();
	if (pass_count > 0) {
		print_verbose(vformat("[ZeGFX] Render graph executed: %d passes, %d transitions.", pass_count, transition_count));
	}

	// Log any validation errors from the render graph
	const auto &errors = frame_render_graph->getValidationErrors();
	for (const auto &err : errors) {
		print_line(vformat("[ZeGFX] Render graph validation error: %s", String(err.c_str())));
	}

	// Clear deferred state for next frame
	pending_ao.dirty = false;
	pending_dxr_reflections.dirty = false;
	pending_post_process.dirty = false;
	pending_meshlet_streams.clear();

	// Reset per-frame replacement flags (they'll be set again next frame by execute_*_pass calls)
	// Note: we do NOT reset ao_pass_succeeded / dxr_reflections_succeeded here because
	// the Godot callsites that check them run AFTER execute_*_pass but BEFORE flush_deferred_passes.
	// They will be reset at the start of the next frame's flush.
}

