/**************************************************************************/
/*  gpu_command_queue.cpp                                                 */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "gpu_command_queue.h"
#include <iostream>

GPUCommandQueueD3D12::GPUCommandQueueD3D12() {
}

GPUCommandQueueD3D12::~GPUCommandQueueD3D12() {
    shutdown();
}

bool GPUCommandQueueD3D12::initialize(ID3D12Device* p_device) {
    shutdown();
    device = p_device;
    if (!device) return false;

    // Create globally cached ID3D12CommandSignature for D3D12_DRAW_INDEXED_ARGUMENTS
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
    sigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &argDesc;
    sigDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&draw_indexed_signature));
    if (FAILED(hr)) {
        std::cerr << "[ZeGFX D3D12] Failed to create ExecuteIndirect command signature! HR: " << std::hex << hr << std::endl;
        return false;
    }

    initialized = true;
    std::cout << "[ZeGFX D3D12] GPU Direct Multi-Draw (ExecuteIndirect) Command Signature cached." << std::endl;
    return true;
}

void GPUCommandQueueD3D12::shutdown() {
    if (!initialized) return;

    if (draw_indexed_signature) {
        draw_indexed_signature->Release();
        draw_indexed_signature = nullptr;
    }

    device = nullptr;
    initialized = false;
}

void GPUCommandQueueD3D12::execute_multi_draw_indirect(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_argument_buffer,
    uint64_t p_argument_buffer_offset,
    uint32_t p_max_command_count,
    ID3D12Resource* p_count_buffer,
    uint64_t p_count_buffer_offset
) {
    if (!initialized || !p_cmd_list || !p_argument_buffer || !draw_indexed_signature) return;

    p_cmd_list->ExecuteIndirect(
        draw_indexed_signature,
        p_max_command_count,
        p_argument_buffer,
        p_argument_buffer_offset,
        p_count_buffer,
        p_count_buffer_offset
    );
}

#endif // WITH_DX12_BACKEND
