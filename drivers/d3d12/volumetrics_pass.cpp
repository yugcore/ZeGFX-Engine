/**************************************************************************/
/*  volumetrics_pass.cpp                                                  */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "volumetrics_pass.h"
#include "ZeGFX/include/volumetrics.h"
#include <iostream>

VolumetricsPassD3D12::VolumetricsPassD3D12() {
}

VolumetricsPassD3D12::~VolumetricsPassD3D12() {
    shutdown();
}

bool VolumetricsPassD3D12::initialize(ID3D12Device* p_device, uint32_t p_grid_x, uint32_t p_grid_y, uint32_t p_grid_z) {
    shutdown();
    device = p_device;
    if (!device) return false;

    fog_system = new zegfx::VolumetricFogSystem();
    if (fog_system) {
        zegfx::VolumetricFogDesc desc;
        desc.GridSize = { p_grid_x, p_grid_y, p_grid_z };
        desc.ScatteringAnisotropy = 0.6f;
        desc.NearGridDepth = 1.0f;
        desc.FarGridDepth = 100.0f;
        fog_system->Initialize(desc);
    }

    initialized = true;
    std::cout << "[ZeGFX D3D12] 3D Texture Volumetric Froxel Fog initialized (" 
              << p_grid_x << "x" << p_grid_y << "x" << p_grid_z << ")." << std::endl;
    return true;
}

void VolumetricsPassD3D12::shutdown() {
    if (!initialized) return;

    if (fog_system) {
        delete fog_system;
        fog_system = nullptr;
    }

    device = nullptr;
    initialized = false;
}

void VolumetricsPassD3D12::dispatch_volumetric_fog(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_light_buffer,
    ID3D12Resource* p_shadow_atlas,
    uint32_t p_width,
    uint32_t p_height
) {
    if (!initialized || !p_cmd_list || !fog_system) return;

    fog_system->InjectVolumetricLighting(p_cmd_list, p_light_buffer, 0);
    fog_system->IntegrateVolume(p_cmd_list);
}

#endif // WITH_DX12_BACKEND
