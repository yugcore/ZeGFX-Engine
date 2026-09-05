/**************************************************************************/
/*  gpu_command_queue.h                                                   */
/**************************************************************************/

#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>

#include <cstdint>

class GPUCommandQueueD3D12 {
public:
    GPUCommandQueueD3D12();
    ~GPUCommandQueueD3D12();

    bool initialize(ID3D12Device* p_device);
    void shutdown();

    ID3D12CommandSignature* get_indirect_draw_signature() const { return draw_indexed_signature; }

    void execute_multi_draw_indirect(
        ID3D12GraphicsCommandList* p_cmd_list,
        ID3D12Resource* p_argument_buffer,
        uint64_t p_argument_buffer_offset,
        uint32_t p_max_command_count,
        ID3D12Resource* p_count_buffer = nullptr,
        uint64_t p_count_buffer_offset = 0
    );

    // Asynchronous compute queue management
    bool has_async_compute() const { return async_compute_supported; }
    ID3D12CommandQueue* get_compute_queue() const { return compute_queue; }
    ID3D12GraphicsCommandList* begin_async_compute();
    void end_and_execute_async_compute(ID3D12CommandQueue* p_direct_queue = nullptr);
    void sync_direct_queue_with_compute(ID3D12CommandQueue* p_direct_queue);

    bool is_initialized() const { return initialized; }

private:
    bool initialized = false;
    ID3D12Device* device = nullptr;
    ID3D12CommandSignature* draw_indexed_signature = nullptr;

    // Async compute queue objects
    ID3D12CommandQueue* compute_queue = nullptr;
    ID3D12CommandAllocator* compute_allocator = nullptr;
    ID3D12GraphicsCommandList* compute_cmd_list = nullptr;
    ID3D12Fence* compute_fence = nullptr;
    uint64_t compute_fence_value = 0;
    bool async_compute_supported = false;
    bool compute_recording = false;
};

#endif // WITH_DX12_BACKEND
