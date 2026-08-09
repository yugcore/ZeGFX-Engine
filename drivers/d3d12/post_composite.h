/**************************************************************************/
/*  post_composite.h                                                      */
/**************************************************************************/

#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>

#include <cstdint>

namespace zegfx {
class AmbientOcclusionSystem;
class ScreenSpaceReflections;
class BloomSystem;
class ExposureSystem;
class ToneMapSystem;
}

class PostCompositeD3D12 {
public:
    PostCompositeD3D12();
    ~PostCompositeD3D12();

    bool initialize(ID3D12Device* p_device);
    void shutdown();

    void execute_post_processing_chain(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_hdr_scene_color,
        ID3D12Resource* p_depth_target,
        ID3D12Resource* p_normal_target,
        ID3D12Resource* p_output_ldr_target,
        uint32_t p_width,
        uint32_t p_height,
        float p_delta_time
    );

    bool is_initialized() const { return initialized; }

private:
    bool initialized = false;
    ID3D12Device* device = nullptr;

    zegfx::AmbientOcclusionSystem* ao_system = nullptr;
    zegfx::ScreenSpaceReflections* ssr_system = nullptr;
    zegfx::BloomSystem* bloom_system = nullptr;
    zegfx::ExposureSystem* exposure_system = nullptr;
    zegfx::ToneMapSystem* tonemap_system = nullptr;
};

#endif // WITH_DX12_BACKEND
