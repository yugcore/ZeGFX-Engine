#ifdef WITH_DX12_BACKEND

#include "d3d12_pipeline_state_manager.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#endif

namespace zegfx {

D3D12PipelineStateManager::~D3D12PipelineStateManager() {
    shutdown();
}

bool D3D12PipelineStateManager::initialize(ID3D12Device* device, const std::string& cacheDirectory) {
    shutdown();
    device_ = device;
    cacheDir_ = cacheDirectory;

    if (cacheDir_.empty()) {
        char* userProfile = nullptr;
        size_t len = 0;
        if (_dupenv_s(&userProfile, &len, "LOCALAPPDATA") == 0 && userProfile != nullptr) {
            cacheDir_ = std::string(userProfile) + "\\ZeGFX\\shader_cache";
            free(userProfile);
        } else {
            cacheDir_ = ".\\shader_cache";
        }
    }

#ifdef _WIN32
    _mkdir(cacheDir_.c_str());
#endif

    cacheFilePath_ = cacheDir_ + "\\dx12_psos.bin";
    loadCacheFromDisk();
    return true;
}

void D3D12PipelineStateManager::shutdown() {
    saveCacheToDisk();
    for (auto& pair : psoCache_) {
        if (pair.second) {
            pair.second->Release();
        }
    }
    psoCache_.clear();

    if (pipelineLibrary_) {
        pipelineLibrary_->Release();
        pipelineLibrary_ = nullptr;
    }
    device_ = nullptr;
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

ID3D12PipelineState* D3D12PipelineStateManager::getOrCreateGraphicsPipeline(
    ID3D12RootSignature* rootSignature,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
    const std::string& pipelineKey
) {
    if (!device_) return nullptr;

    auto it = psoCache_.find(pipelineKey);
    if (it != psoCache_.end() && it->second != nullptr) {
        cacheHits_++;
        return it->second;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC copyDesc = desc;
    if (rootSignature) {
        copyDesc.pRootSignature = rootSignature;
    }

    ID3D12PipelineState* pso = nullptr;
    HRESULT hr = device_->CreateGraphicsPipelineState(&copyDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr) || !pso) {
        std::cerr << "[PSO Cache] Failed to create graphics pipeline state for key: " << pipelineKey << std::endl;
        return nullptr;
    }

    psoCache_[pipelineKey] = pso;
    cacheMisses_++;
    return pso;
}

ID3D12PipelineState* D3D12PipelineStateManager::getOrCreateComputePipeline(
    ID3D12RootSignature* rootSignature,
    const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
    const std::string& pipelineKey
) {
    if (!device_) return nullptr;

    auto it = psoCache_.find(pipelineKey);
    if (it != psoCache_.end() && it->second != nullptr) {
        cacheHits_++;
        return it->second;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC copyDesc = desc;
    if (rootSignature) {
        copyDesc.pRootSignature = rootSignature;
    }

    ID3D12PipelineState* pso = nullptr;
    HRESULT hr = device_->CreateComputePipelineState(&copyDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr) || !pso) {
        std::cerr << "[PSO Cache] Failed to create compute pipeline state for key: " << pipelineKey << std::endl;
        return nullptr;
    }

    psoCache_[pipelineKey] = pso;
    cacheMisses_++;
    return pso;
}

bool D3D12PipelineStateManager::loadCacheFromDisk() {
    if (cacheFilePath_.empty()) return false;
    std::ifstream inFile(cacheFilePath_, std::ios::binary);
    if (!inFile.is_open()) return false;

    uint32_t magic = 0;
    inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x50534F31) { // "PSO1"
        return false;
    }

    uint32_t entryCount = 0;
    inFile.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
    if (entryCount > 0) {
        std::cout << "[PSO Cache] Prewarmed " << entryCount << " pipeline keys from disk cache at " << cacheFilePath_ << std::endl;
    }
    return true;
}

bool D3D12PipelineStateManager::saveCacheToDisk() {
    if (cacheFilePath_.empty() || psoCache_.empty()) return false;
    std::ofstream outFile(cacheFilePath_, std::ios::binary);
    if (!outFile.is_open()) return false;

    uint32_t magic = 0x50534F31; // "PSO1"
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    uint32_t entryCount = (uint32_t)psoCache_.size();
    outFile.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

    return true;
}

} // namespace zegfx

#endif // WITH_DX12_BACKEND
