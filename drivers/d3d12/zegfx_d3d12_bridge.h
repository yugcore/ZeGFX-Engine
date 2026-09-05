/**************************************************************************/
/*  zegfx_d3d12_bridge.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifndef ZEGFX_D3D12_BRIDGE_H
#define ZEGFX_D3D12_BRIDGE_H

#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering/rendering_device_driver.h"

#include <cstdint>

class DXRPipelineD3D12;
class PostCompositeD3D12;
class VolumetricsPassD3D12;
class GPUCommandQueueD3D12;

namespace zegfx {
class RenderGraph;
class ZPostProcessScheduler;
class VirtualGeometryManager;
class ZCullingContext;
class ZSubmissionContext;
class ZGPUScene;
} // namespace zegfx

// Bridge singleton wiring ZeGFX subsystems into Godot's
// RenderingDeviceDriverD3D12 while preserving 100% of editor UI & gizmos.

class ZeGFXD3D12Bridge {
private:
	static ZeGFXD3D12Bridge *singleton;
	bool initialized = false;
	bool device_initialized = false;
	DXRPipelineD3D12 *dxr_pipeline = nullptr;
	PostCompositeD3D12 *post_composite = nullptr;
	VolumetricsPassD3D12 *volumetrics_pass = nullptr;
	GPUCommandQueueD3D12 *gpu_command_queue = nullptr;
	void *main_direct_queue = nullptr;

	// --- Render graph & GPU culling subsystems (owned) ---
	zegfx::RenderGraph *frame_render_graph = nullptr;
	zegfx::ZPostProcessScheduler *post_scheduler = nullptr;
	zegfx::VirtualGeometryManager *virtual_geom_manager = nullptr;
	zegfx::ZCullingContext *culling_context = nullptr;
	zegfx::ZSubmissionContext *submission_context = nullptr;
	zegfx::ZGPUScene *gpu_scene = nullptr;

	struct CullingViewData {
		Transform3D cam_transform;
		Projection cam_projection;
		Vector3 cam_position;
		bool dirty = false;
	} culling_view;

	// --- Deferred AO pass state ---
	struct PendingAO {
		bool dirty = false;
		int width = 0;
		int height = 0;
		float radius = 0.0f;
		float intensity = 0.0f;
		float power = 1.0f;
		int samples = 4;
	} pending_ao;

	// --- Deferred DXR reflections state ---
	struct PendingDXRReflections {
		bool dirty = false;
		int width = 0;
		int height = 0;
		float roughness = 0.0f;
	} pending_dxr_reflections;

	// --- Deferred DXR GI state ---
	struct PendingDXRGI {
		bool dirty = false;
		int width = 0;
		int height = 0;
		float max_distance = 64.0f;
		float energy = 1.0f;
		int bounce_count = 1;
	} pending_dxr_gi;

	// --- Deferred DXR shadow state ---
	struct PendingDXRShadow {
		bool dirty = false;
		int width = 0;
		int height = 0;
		float light_dir[3] = { 0.0f, -1.0f, 0.0f };
		float light_pos[3] = { 0.0f, 0.0f, 0.0f };
		float max_distance = 150.0f;
		float softness = 1.0f;
		int samples = 1;
		int light_type = 0;
	} pending_dxr_shadow;

	// --- Deferred post-process state ---
	struct PendingPostProcess {
		bool dirty = false;
		int width = 0;
		int height = 0;
		float exposure = 1.0f;
		float bloom_intensity = 0.0f;
		int tonemap_mode = 0;
		float sharpen = 0.0f;
		float vignette = 0.0f;
	} pending_post_process;

	// --- Deferred meshlet streamer queue ---
	struct PendingMeshletStream {
		String zmesh_path;
		uint32_t lod_level = 0;
		uint32_t instance_count = 0;
	};
	Vector<PendingMeshletStream> pending_meshlet_streams;

	// --- Registered .zmesh metadata cache ---
	struct ZMeshMetadata {
		String zmesh_path;
		uint32_t meshlet_count = 0;
		uint32_t lod_count = 0;
		uint32_t primitive_count = 0;
		uint32_t vertex_stride = 0;
	};
	HashMap<String, ZMeshMetadata> registered_zmeshes;

	// --- Track whether ZeGFX passes replaced Godot's this frame ---
	bool ao_pass_succeeded = false;
	bool dxr_reflections_succeeded = false;
	bool dxr_gi_succeeded = false;
	bool dxr_shadows_succeeded = false;
	bool active_cmd_list_attached = false;
	void *active_cmd_list = nullptr;

public:
	static ZeGFXD3D12Bridge *get_singleton() { return singleton; }

	ZeGFXD3D12Bridge();
	~ZeGFXD3D12Bridge();

	bool initialize(void *p_device_or_hwnd, String &r_error);
	bool initialize_device(void *p_d3d12_device);
	void shutdown();

	bool is_initialized() const { return initialized; }

	// Active native command list & queue tracking
	void set_active_command_list(void *p_cmd_list);
	void *get_active_command_list() const { return active_cmd_list; }
	bool has_active_command_list() const { return active_cmd_list != nullptr; }
	void set_main_command_queue(void *p_queue) { main_direct_queue = p_queue; }
	void *get_main_command_queue() const { return main_direct_queue; }
	GPUCommandQueueD3D12 *get_gpu_command_queue() const { return gpu_command_queue; }

	// Register and query cooked .zmesh assets
	void register_zmesh_metadata(const String &p_path, uint32_t p_meshlet_count, uint32_t p_lod_count, uint32_t p_primitive_count, uint32_t p_vertex_stride);
	bool is_zmesh_registered(const String &p_path) const;
	const ZMeshMetadata *get_zmesh_metadata(const String &p_path) const;

	// Update camera and view constants for GPU Hi-Z culling
	void update_culling_view(const Transform3D &p_cam_transform, const Projection &p_cam_projection, const Vector3 &p_cam_pos);
	zegfx::VirtualGeometryManager *get_virtual_geometry_manager() const { return virtual_geom_manager; }
	zegfx::ZCullingContext *get_culling_context() const { return culling_context; }
	zegfx::ZGPUScene *get_gpu_scene() const { return gpu_scene; }

	// Register scene instance transforms and bounds for GPU-driven culling
	void register_scene_instance(const Transform3D &p_transform, const AABB &p_aabb, uint32_t p_material_id = 0);
	void clear_scene_instances();

	// Query whether ZeGFX passes should replace Godot's equivalents this frame
	bool ao_pass_active() const { return initialized && ao_pass_succeeded && active_cmd_list_attached; }
	bool dxr_reflections_active() const { return initialized && dxr_reflections_succeeded && active_cmd_list_attached; }
	bool has_dxr_reflections_succeeded() const { return dxr_reflections_succeeded; }
	bool dxr_gi_active() const { return initialized && dxr_gi_succeeded && active_cmd_list_attached; }
	bool has_dxr_gi_succeeded() const { return dxr_gi_succeeded; }
	bool dxr_shadows_active() const { return initialized && dxr_shadows_succeeded && active_cmd_list_attached; }
	bool has_dxr_shadows_succeeded() const { return dxr_shadows_succeeded; }
	bool post_composite_active() const { return initialized && post_composite != nullptr; }

	// Phase 1 Subsystem Swap: godotShadow -> zegfxShadow
	bool execute_shadow_pass(float p_near_clip, float p_far_clip, uint32_t p_cascade_count, Vector<float> &r_splits);

	// Phase 2 Subsystem Swap: godotAO -> zegfxAO (DXR RTAO / GTAO)
	bool execute_ao_pass(int p_width, int p_height, float p_radius, float p_intensity, float p_power = 1.0f, int p_samples = 4);

	// Phase 3 Subsystem Swap: godotSSR -> zegfxDXR
	bool execute_dxr_reflections_pass(int p_width, int p_height, float p_roughness_threshold);
	bool execute_dxr_gi_pass(int p_width, int p_height, float p_max_distance, float p_energy, int p_bounce_count);
	bool execute_dxr_shadow_pass(int p_width, int p_height, const float p_light_dir[3], const float p_light_pos[3], float p_max_distance, float p_softness, int p_samples, int p_light_type);

	// Phase 4 Subsystem Swap: godotPost -> zegfxPost (Bloom + Auto-Exposure + ACES Tonemap + Sharpening)
	bool execute_post_process_pass(int p_width, int p_height, float p_exposure, float p_bloom_intensity, int p_tonemap_mode, float p_sharpen, float p_vignette);

	// Phase 5 Subsystem Swap: Cooked .zmesh V2 Meshlets & Dynamic LOD Streamer
	bool execute_meshlet_streamer_pass(const String &p_zmesh_path, uint32_t p_lod_level, uint32_t p_instance_count);
	bool cook_and_load_zmesh(const String &p_source_file, const String &p_output_zmesh);
	bool cook_and_load_ztex(const String &p_source_file, const String &p_output_ztex);

	// Called when a D3D12 command list is available to flush all deferred render graph passes.
	// p_cmd_list: ID3D12GraphicsCommandList* (passed as void* to avoid D3D12 header dependency here).
	// p_hdr_target, p_depth_target, p_normal_target, p_output_target: ID3D12Resource* for render graph I/O.
	void flush_deferred_passes(void *p_cmd_list, void *p_hdr_target, void *p_depth_target,
			void *p_normal_target, void *p_output_target,
			int p_width, int p_height, float p_delta_time);

	struct DeferredPassArgs {
		void *hdr_target = nullptr;
		void *depth_target = nullptr;
		void *normal_target = nullptr;
		void *output_target = nullptr;
		int width = 0;
		int height = 0;
		float delta_time = 0.016f;
	};

	// Synchronized driver callback invoked by RenderingDeviceGraph during GPU command recording
	static void driver_callback_flush_passes(RenderingDeviceDriver *p_driver, RDD::CommandBufferID p_cmd_buffer, void *p_userdata);
};

#endif // ZEGFX_D3D12_BRIDGE_H

