/**************************************************************************/
/*  renderer_scene_d3d12.h                                                */
/**************************************************************************/

#pragma once

#include "servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.h"

#if defined(D3D12_ENABLED) && defined(WITH_DX12_BACKEND)
#include "ZeGFX/include/graphics_backend.h"
#include "ZeGFX/include/lighting_types.h"
#include "ZeGFX/include/render_graph.h"
#include "ZeGFX/include/render_world.h"
#include "drivers/d3d12/zegfx_d3d12_bridge.h"
#endif

namespace zegfx {
class LightingSystem;
class LightGridManager;
} // namespace zegfx

class RendererSceneD3D12 : public RendererSceneRenderImplementation::RenderForwardClustered {
private:
	bool initialized = false;
	uint32_t current_width = 0;
	uint32_t current_height = 0;
	zegfx::LightingSystem *lighting_system = nullptr;
	zegfx::LightGridManager *light_grid_manager = nullptr;

	void _setup_zegfx_frame(RenderDataRD *p_render_data, uint32_t p_width, uint32_t p_height);

protected:
	virtual void _render_scene(RenderDataRD *p_render_data, const Color &p_default_bg_color) override;

public:
	RendererSceneD3D12();
	virtual ~RendererSceneD3D12() override;

	bool initialize();
	void shutdown();

	void setup_gbuffer_pass(uint32_t p_width, uint32_t p_height);
	void setup_light_grid(const Transform3D &p_camera_transform, const Projection &p_projection, uint32_t p_width, uint32_t p_height);
	void render_scene_deferred(void *p_cmd_list);

	bool is_initialized() const { return initialized; }
};
