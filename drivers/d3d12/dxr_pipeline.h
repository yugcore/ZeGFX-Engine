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

class DXRPipelineD3D12 {
public:
    DXRPipelineD3D12();
    ~DXRPipelineD3D12();

    bool initialize(ID3D12Device* p_device);
    void shutdown();

    bool is_dxr_supported() const { return dxr_supported; }

    void build_tlas(ID3D12GraphicsCommandList4* p_cmd_list, ID3D12Resource* p_tlas_buffer, ID3D12Resource* p_instance_desc_buffer, uint32_t p_instance_count);

    void dispatch_rays(
        ID3D12GraphicsCommandList4* p_cmd_list,
        D3D12_DISPATCH_RAYS_DESC* p_dispatch_desc
    );

private:
    bool initialized = false;
    bool dxr_supported = false;
    ID3D12Device* device = nullptr;
    ID3D12Device5* device5 = nullptr;
    ID3D12StateObject* rtx_state_object = nullptr;
};

#endif // WITH_DX12_BACKEND
