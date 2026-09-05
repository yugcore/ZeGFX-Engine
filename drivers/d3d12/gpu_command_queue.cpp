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

    // Initialize dedicated asynchronous compute queue & hardware synchronization fence
    D3D12_COMMAND_QUEUE_DESC compute_queue_desc = {};
    compute_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    compute_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    compute_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    compute_queue_desc.NodeMask = 0;

    HRESULT hr_q = device->CreateCommandQueue(&compute_queue_desc, IID_PPV_ARGS(&compute_queue));
    if (SUCCEEDED(hr_q)) {
        HRESULT hr_alloc = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&compute_allocator));
        if (SUCCEEDED(hr_alloc)) {
            HRESULT hr_list = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, compute_allocator, nullptr, IID_PPV_ARGS(&compute_cmd_list));
            if (SUCCEEDED(hr_list)) {
                compute_cmd_list->Close();
                HRESULT hr_fence = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&compute_fence));
                if (SUCCEEDED(hr_fence)) {
                    compute_fence_value = 0;
                    async_compute_supported = true;
                    std::cout << "[ZeGFX D3D12] Asynchronous Compute Queue & hardware fence initialized." << std::endl;
                }
            }
        }
    }

    initialized = true;
    std::cout << "[ZeGFX D3D12] GPU Direct Multi-Draw (ExecuteIndirect) Command Signature cached." << std::endl;
    return true;
}

void GPUCommandQueueD3D12::shutdown() {
    if (!initialized) return;

    if (compute_recording && compute_cmd_list) {
        compute_cmd_list->Close();
        compute_recording = false;
    }

    if (compute_fence) {
        compute_fence->Release();
        compute_fence = nullptr;
    }

    if (compute_cmd_list) {
        compute_cmd_list->Release();
        compute_cmd_list = nullptr;
    }

    if (compute_allocator) {
        compute_allocator->Release();
        compute_allocator = nullptr;
    }

    if (compute_queue) {
        compute_queue->Release();
        compute_queue = nullptr;
    }

    if (draw_indexed_signature) {
        draw_indexed_signature->Release();
        draw_indexed_signature = nullptr;
    }

    async_compute_supported = false;
    compute_fence_value = 0;
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

ID3D12GraphicsCommandList* GPUCommandQueueD3D12::begin_async_compute() {
    if (!initialized || !async_compute_supported || compute_recording) {
        return nullptr;
    }

    HRESULT hr_alloc = compute_allocator->Reset();
    if (FAILED(hr_alloc)) {
        return nullptr;
    }

    HRESULT hr_list = compute_cmd_list->Reset(compute_allocator, nullptr);
    if (FAILED(hr_list)) {
        return nullptr;
    }

    compute_recording = true;
    return compute_cmd_list;
}

void GPUCommandQueueD3D12::end_and_execute_async_compute(ID3D12CommandQueue* p_direct_queue) {
    if (!initialized || !async_compute_supported || !compute_recording) {
        return;
    }

    compute_recording = false;
    HRESULT hr_close = compute_cmd_list->Close();
    if (FAILED(hr_close)) {
        return;
    }

    ID3D12CommandList* cmd_lists[] = { compute_cmd_list };
    compute_queue->ExecuteCommandLists(1, cmd_lists);

    compute_fence_value++;
    compute_queue->Signal(compute_fence, compute_fence_value);

    if (p_direct_queue) {
        p_direct_queue->Wait(compute_fence, compute_fence_value);
    }
}

void GPUCommandQueueD3D12::sync_direct_queue_with_compute(ID3D12CommandQueue* p_direct_queue) {
    if (!initialized || !async_compute_supported || !p_direct_queue || !compute_fence) {
        return;
    }
    p_direct_queue->Wait(compute_fence, compute_fence_value);
}

#endif // WITH_DX12_BACKEND
