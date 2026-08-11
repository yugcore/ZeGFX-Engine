/**************************************************************************/
/*  renderer_scene_d3d12.h                                                */
/**************************************************************************/

#pragma once

#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"

#ifdef WITH_DX12_BACKEND
#include "ZeGFX/include/graphics_backend.h"
#include "ZeGFX/include/lighting_types.h"
#include "ZeGFX/include/render_graph.h"
#include "ZeGFX/include/render_world.h"
#endif

namespace zegfx {
class LightingSystem;
class LightGridManager;
}

class RendererSceneD3D12 {
public:
    RendererSceneD3D12();
    ~RendererSceneD3D12();

    bool initialize();
    void shutdown();

    void setup_gbuffer_pass(uint32_t p_width, uint32_t p_height);
    void setup_light_grid(const Transform3D& p_camera_transform, const Projection& p_projection, uint32_t p_width, uint32_t p_height);
    void render_scene_deferred(void* p_cmd_list);

    bool is_initialized() const { return initialized; }

private:
    bool initialized = false;
    uint32_t current_width = 0;
    uint32_t current_height = 0;
    zegfx::LightingSystem* lighting_system = nullptr;
    zegfx::LightGridManager* light_grid_manager = nullptr;
};
