/**************************************************************************/
/*  renderer_scene_d3d12.cpp                                              */
/**************************************************************************/

#include "renderer_scene_d3d12.h"
#include "core/os/os.h"
#include <iostream>

#ifdef WITH_DX12_BACKEND
#include "ZeGFX/include/light_grid_manager.h"
#include "ZeGFX/include/lighting_types.h"
#endif

RendererSceneD3D12::RendererSceneD3D12() {
}

RendererSceneD3D12::~RendererSceneD3D12() {
    shutdown();
}

bool RendererSceneD3D12::initialize() {
    if (initialized) return true;

#ifdef WITH_DX12_BACKEND
    light_grid_manager = memnew(zegfx::LightGridManager);
    if (light_grid_manager) {
        light_grid_manager->Initialize();
    }
#endif

    initialized = true;
    std::cout << "[ZeGFX D3D12] Scene Renderer & Deferred Light Grid initialized." << std::endl;
    return true;
}

void RendererSceneD3D12::shutdown() {
    if (!initialized) return;

#ifdef WITH_DX12_BACKEND
    if (light_grid_manager) {
        memdelete(light_grid_manager);
        light_grid_manager = nullptr;
    }
#endif

    initialized = false;
}

void RendererSceneD3D12::setup_gbuffer_pass(uint32_t p_width, uint32_t p_height) {
    current_width = p_width;
    current_height = p_height;
}

void RendererSceneD3D12::setup_light_grid(const Transform3D& p_camera_transform, const Projection& p_projection, uint32_t p_width, uint32_t p_height) {
    if (!initialized) return;
    current_width = p_width;
    current_height = p_height;

#ifdef WITH_DX12_BACKEND
    if (light_grid_manager) {
        zegfx::CameraRenderData cameraData;
        
        // Map Godot camera transform to view matrix (inverse camera transform)
        Transform3D inv_transform = p_camera_transform.affine_inverse();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                cameraData.view.m[r][c] = inv_transform.basis.rows[r][c];
            }
            cameraData.view.m[r][3] = 0.0f;
            cameraData.view.m[3][r] = inv_transform.origin[r];
        }
        cameraData.view.m[3][3] = 1.0f;

        // Map Godot projection matrix
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                cameraData.projection.m[r][c] = p_projection.columns[c][r];
            }
        }

        cameraData.viewProjection = cameraData.view * cameraData.projection;

        // Camera position and clip planes
        cameraData.position.x = p_camera_transform.origin.x;
        cameraData.position.y = p_camera_transform.origin.y;
        cameraData.position.z = p_camera_transform.origin.z;
        cameraData.nearPlane = p_projection.get_z_near();
        cameraData.farPlane = p_projection.get_z_far();

        zegfx::ZSceneData sceneData;
        light_grid_manager->BuildLightGrid(cameraData, sceneData, p_width, p_height);
    }
#endif
}

void RendererSceneD3D12::render_scene_deferred(void* p_cmd_list) {
    if (!initialized || !p_cmd_list) return;

#ifdef WITH_DX12_BACKEND
    // Deferred light grid dispatch on live D3D12 command list
#endif
}
