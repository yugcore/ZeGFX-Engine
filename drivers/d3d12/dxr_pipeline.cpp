/**************************************************************************/
/*  dxr_pipeline.cpp                                                      */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "dxr_pipeline.h"
#include <iostream>

DXRPipelineD3D12::DXRPipelineD3D12() {
}

DXRPipelineD3D12::~DXRPipelineD3D12() {
    shutdown();
}

bool DXRPipelineD3D12::initialize(ID3D12Device* p_device) {
    shutdown();
    device = p_device;
    if (!device) return false;

    // Query ID3D12Device5 for DXR tier support
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device5)))) {
        std::cout << "[ZeGFX D3D12] Hardware Ray Tracing (DXR 1.1) interface query skipped." << std::endl;
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))) &&
        options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
        dxr_supported = true;
        std::cout << "[ZeGFX D3D12] Hardware Ray Tracing (DXR 1.1) active on GPU." << std::endl;
    } else {
        dxr_supported = false;
        std::cout << "[ZeGFX D3D12] DXR Hardware Ray Tracing not available; compute fallback active." << std::endl;
    }

    initialized = true;
    return true;
}

void DXRPipelineD3D12::shutdown() {
    if (!initialized) return;

    if (rtx_state_object) {
        rtx_state_object->Release();
        rtx_state_object = nullptr;
    }

    if (device5) {
        device5->Release();
        device5 = nullptr;
    }

    device = nullptr;
    initialized = false;
    dxr_supported = false;
}

void DXRPipelineD3D12::build_tlas(ID3D12GraphicsCommandList4* p_cmd_list, ID3D12Resource* p_tlas_buffer, ID3D12Resource* p_instance_desc_buffer, uint32_t p_instance_count) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_tlas_buffer || !p_instance_desc_buffer) return;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    buildDesc.Inputs.NumDescs = p_instance_count;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.InstanceDescs = p_instance_desc_buffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = p_tlas_buffer->GetGPUVirtualAddress();

    p_cmd_list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

void DXRPipelineD3D12::dispatch_rays(
    ID3D12GraphicsCommandList4* p_cmd_list,
    D3D12_DISPATCH_RAYS_DESC* p_dispatch_desc
) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_dispatch_desc) return;
    p_cmd_list->DispatchRays(p_dispatch_desc);
}

#endif // WITH_DX12_BACKEND
