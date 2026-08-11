#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace zegfx {

class D3D12PipelineStateManager {
public:
    D3D12PipelineStateManager() = default;
    ~D3D12PipelineStateManager();

    bool initialize(ID3D12Device* device, const std::string& cacheDirectory = "");
    void shutdown();

    ID3D12PipelineState* getOrCreateGraphicsPipeline(
        ID3D12RootSignature* rootSignature,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
        const std::string& pipelineKey
    );

    ID3D12PipelineState* getOrCreateComputePipeline(
        ID3D12RootSignature* rootSignature,
        const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
        const std::string& pipelineKey
    );

    bool loadCacheFromDisk();
    bool saveCacheToDisk();

    size_t getCachedPipelineCount() const { return psoCache_.size(); }
    uint32_t getCacheHitCount() const { return cacheHits_; }
    uint32_t getCacheMissCount() const { return cacheMisses_; }

private:
    ID3D12Device* device_ = nullptr;
    ID3D12PipelineLibrary* pipelineLibrary_ = nullptr;
    std::string cacheDir_;
    std::string cacheFilePath_;
    std::unordered_map<std::string, ID3D12PipelineState*> psoCache_;
    uint32_t cacheHits_ = 0;
    uint32_t cacheMisses_ = 0;
};

} // namespace zegfx

#endif // WITH_DX12_BACKEND
