/**************************************************************************/
/*  dxr_pipeline.h                                                        */
/**************************************************************************/

#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>

#include <cstdint>

struct DXRReflectionConstants {
    float max_distance = 1000.0f;
    float roughness_cutoff = 0.5f;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DXRGIConstants {
    float max_distance = 64.0f;
    float energy = 1.0f;
    uint32_t bounce_count = 1;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DXRShadowConstants {
    float light_direction[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
    float light_position[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float max_distance = 150.0f;
    float softness = 1.0f;
    uint32_t samples = 1;
    uint32_t light_type = 0; // 0 = directional, 1 = spot/omni
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DXRAmbientOcclusionConstants {
    float radius = 1.5f;
    float intensity = 1.0f;
    float power = 1.0f;
    uint32_t samples = 4;
    uint32_t width = 0;
    uint32_t height = 0;
    float padding[2] = { 0.0f, 0.0f };
};

class DXRPipelineD3D12 {
public:
    DXRPipelineD3D12();
    ~DXRPipelineD3D12();

    bool initialize(ID3D12Device* p_device);
    void shutdown();

    bool is_initialized() const { return initialized; }
    bool is_dxr_supported() const { return dxr_supported; }
    bool is_pipeline_ready() const { return rtx_state_object != nullptr && sbt_buffer != nullptr; }

    void build_tlas(ID3D12GraphicsCommandList4* p_cmd_list, ID3D12Resource* p_tlas_buffer, ID3D12Resource* p_instance_desc_buffer, uint32_t p_instance_count);

    void dispatch_rays(
        ID3D12GraphicsCommandList4* p_cmd_list,
        D3D12_DISPATCH_RAYS_DESC* p_dispatch_desc
    );

    void dispatch_reflection_rays(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_hdr_target,
        ID3D12Resource* p_depth_target,
        ID3D12Resource* p_normal_target,
        int p_width,
        int p_height,
        float p_roughness_threshold
    );

    void dispatch_gi_rays(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_hdr_target,
        ID3D12Resource* p_depth_target,
        ID3D12Resource* p_normal_target,
        int p_width,
        int p_height,
        float p_max_distance,
        float p_energy,
        int p_bounce_count
    );

    void dispatch_shadow_rays(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_hdr_target,
        ID3D12Resource* p_depth_target,
        ID3D12Resource* p_normal_target,
        int p_width,
        int p_height,
        const float p_light_dir[3],
        const float p_light_pos[3],
        float p_max_distance,
        float p_softness,
        int p_samples,
        int p_light_type
    );

    void dispatch_ao_rays(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_hdr_target,
        ID3D12Resource* p_depth_target,
        ID3D12Resource* p_normal_target,
        int p_width,
        int p_height,
        float p_radius,
        float p_intensity,
        float p_power,
        int p_samples
    );

private:
    bool create_global_root_signature();
    bool create_raytracing_state_object();
    bool build_shader_binding_table();

    bool initialized = false;
    bool dxr_supported = false;
    ID3D12Device* device = nullptr;
    ID3D12Device5* device5 = nullptr;
    ID3D12StateObject* rtx_state_object = nullptr;
    ID3D12StateObjectProperties* state_object_props = nullptr;
    ID3D12RootSignature* global_root_sig = nullptr;
    ID3D12Resource* sbt_buffer = nullptr;
    ID3D12Resource* reflection_cb = nullptr;

    D3D12_DISPATCH_RAYS_DESC dispatch_desc = {};
};

#endif // WITH_DX12_BACKEND
