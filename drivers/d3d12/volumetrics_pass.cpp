/**************************************************************************/
/*  volumetrics_pass.cpp                                                  */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "volumetrics_pass.h"
#include "ZeGFX/include/volumetrics.h"
#include <iostream>

VolumetricsPassD3D12::VolumetricsPassD3D12() {
}

VolumetricsPassD3D12::~VolumetricsPassD3D12() {
    shutdown();
}

bool VolumetricsPassD3D12::initialize(ID3D12Device* p_device, uint32_t p_grid_x, uint32_t p_grid_y, uint32_t p_grid_z) {
    shutdown();
    device = p_device;
    if (!device) return false;

    fog_system = new zegfx::VolumetricFogSystem();
    if (fog_system) {
        zegfx::VolumetricFogDesc desc;
        desc.GridSize = { p_grid_x, p_grid_y, p_grid_z };
        desc.ScatteringAnisotropy = 0.6f;
        desc.NearGridDepth = 1.0f;
        desc.FarGridDepth = 100.0f;
        fog_system->Initialize(desc);
    }

    // Allocate default structured light buffer so froxel fog injection always has valid GPU inputs
    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = 64;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &uploadProps,
        D3D12_HEAP_FLAG_NONE,
        &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&default_light_buffer));
    if (SUCCEEDED(hr) && default_light_buffer) {
        struct DefaultLightData {
            float position[3];
            float radius;
            float color[3];
            float intensity;
            float direction[3];
            float spotAngle;
        } lightData = {
            { 0.0f, 10.0f, 0.0f }, 100.0f,
            { 1.0f, 0.95f, 0.8f }, 1.5f,
            { 0.25f, -0.86f, 0.42f }, 0.0f
        };
        void *mapped = nullptr;
        if (SUCCEEDED(default_light_buffer->Map(0, nullptr, &mapped))) {
            memcpy(mapped, &lightData, sizeof(lightData));
            default_light_buffer->Unmap(0, nullptr);
        }
    }

    initialized = true;
    std::cout << "[ZeGFX D3D12] 3D Texture Volumetric Froxel Fog initialized (" 
              << p_grid_x << "x" << p_grid_y << "x" << p_grid_z << ")." << std::endl;
    return true;
}

void VolumetricsPassD3D12::shutdown() {
    if (!initialized) return;

    if (default_light_buffer) {
        default_light_buffer->Release();
        default_light_buffer = nullptr;
    }

    if (fog_system) {
        delete fog_system;
        fog_system = nullptr;
    }

    device = nullptr;
    initialized = false;
}

void VolumetricsPassD3D12::dispatch_volumetric_fog(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_light_buffer,
    ID3D12Resource* p_shadow_atlas,
    uint32_t p_width,
    uint32_t p_height
) {
    if (!initialized || !p_cmd_list || !fog_system) return;

    ID3D12Resource *light_buf = p_light_buffer ? p_light_buffer : default_light_buffer;
    if (!light_buf) return;

    fog_system->InjectVolumetricLighting(p_cmd_list, light_buf, 0);
    fog_system->IntegrateVolume(p_cmd_list);
}

#endif // WITH_DX12_BACKEND
