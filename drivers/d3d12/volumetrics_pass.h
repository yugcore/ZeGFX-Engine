/**************************************************************************/
/*  volumetrics_pass.h                                                    */
/**************************************************************************/

#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>

#include <cstdint>
#include <memory>

namespace zegfx {
class VolumetricFogSystem;
}

class VolumetricsPassD3D12 {
public:
    VolumetricsPassD3D12();
    ~VolumetricsPassD3D12();

    bool initialize(ID3D12Device* p_device, uint32_t p_grid_x = 160, uint32_t p_grid_y = 90, uint32_t p_grid_z = 128);
    void shutdown();

    void dispatch_volumetric_fog(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_light_buffer,
        ID3D12Resource* p_shadow_atlas,
        uint32_t p_width,
        uint32_t p_height
    );

    bool is_initialized() const { return initialized; }

private:
    bool initialized = false;
    ID3D12Device* device = nullptr;
    zegfx::VolumetricFogSystem* fog_system = nullptr;
};

#endif // WITH_DX12_BACKEND
