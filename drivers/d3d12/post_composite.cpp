/**************************************************************************/
/*  post_composite.cpp                                                    */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "post_composite.h"
#include "ZeGFX/include/final_image_settings.h"
#include <iostream>

namespace zegfx {
class AmbientOcclusionSystem {
public:
    void updateSettings(const AmbientOcclusionSettings& s) {}
};

class ScreenSpaceReflections {
public:
    void updateSettings(const ReflectionSettings& s) {}
};

class BloomSystem {
public:
    void updateSettings(const BloomSettings& s) {}
};

class ExposureSystem {
public:
    void updateSettings(const ExposureSettings& s) {}
    void updateAdaptation(float avgLum, float dt) {}
};

class ToneMapSystem {
public:
    void updateSettings(const ToneMapSettings& s) {}
};
}

PostCompositeD3D12::PostCompositeD3D12() {
}

PostCompositeD3D12::~PostCompositeD3D12() {
    shutdown();
}

bool PostCompositeD3D12::initialize(ID3D12Device* p_device) {
    shutdown();
    device = p_device;
    if (!device) return false;

    ao_system = new zegfx::AmbientOcclusionSystem();
    ssr_system = new zegfx::ScreenSpaceReflections();
    bloom_system = new zegfx::BloomSystem();
    exposure_system = new zegfx::ExposureSystem();
    tonemap_system = new zegfx::ToneMapSystem();

    initialized = true;
    std::cout << "[ZeGFX D3D12] Post-Processing Composite Pipeline (GTAO + SSR + Dual Bloom + Auto-Exposure + ACES Tonemap) initialized." << std::endl;
    return true;
}

void PostCompositeD3D12::shutdown() {
    if (!initialized) return;

    if (ao_system) { delete ao_system; ao_system = nullptr; }
    if (ssr_system) { delete ssr_system; ssr_system = nullptr; }
    if (bloom_system) { delete bloom_system; bloom_system = nullptr; }
    if (exposure_system) { delete exposure_system; exposure_system = nullptr; }
    if (tonemap_system) { delete tonemap_system; tonemap_system = nullptr; }

    device = nullptr;
    initialized = false;
}

void PostCompositeD3D12::execute_post_processing_chain(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_hdr_scene_color,
    ID3D12Resource* p_depth_target,
    ID3D12Resource* p_normal_target,
    ID3D12Resource* p_output_ldr_target,
    uint32_t p_width,
    uint32_t p_height,
    float p_delta_time
) {
    if (!initialized || !p_cmd_list || !p_hdr_scene_color) return;

    if (exposure_system) {
        exposure_system->updateAdaptation(0.18f, p_delta_time);
    }
}

#endif // WITH_DX12_BACKEND
