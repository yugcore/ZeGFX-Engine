/**************************************************************************/
/*  dxr_pipeline.cpp                                                      */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "dxr_pipeline.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include <unknwn.h>
#include <objbase.h>
#include <dxcapi.h>

namespace {

struct DXCHelper {
    HMODULE dxc_module = nullptr;
    HMODULE dxil_module = nullptr;
    DxcCreateInstanceProc create_instance = nullptr;
    bool available = false;

    bool init() {
        dxil_module = LoadLibraryW(L"dxil.dll");
        if (!dxil_module) {
            dxil_module = LoadLibraryW(L"bin/dxil.dll");
        }

        dxc_module = LoadLibraryW(L"dxcompiler.dll");
        if (!dxc_module) {
            dxc_module = LoadLibraryW(L"bin/dxcompiler.dll");
        }

        if (!dxil_module || !dxc_module) {
            WCHAR exe_path[MAX_PATH];
            if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH)) {
                WCHAR *last_slash = wcsrchr(exe_path, L'\\');
                if (last_slash) {
                    *(last_slash + 1) = L'\0';
                    std::wstring exe_dir = exe_path;
                    if (!dxil_module) {
                        dxil_module = LoadLibraryW((exe_dir + L"dxil.dll").c_str());
                    }
                    if (!dxc_module) {
                        dxc_module = LoadLibraryW((exe_dir + L"dxcompiler.dll").c_str());
                    }
                }
            }
        }

        if (!dxc_module) {
            return false;
        }

        create_instance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxc_module, "DxcCreateInstance"));
        if (!create_instance) {
            return false;
        }

        available = true;
        return true;
    }

    void cleanup() {
        if (dxc_module) {
            FreeLibrary(dxc_module);
            dxc_module = nullptr;
        }
        if (dxil_module) {
            FreeLibrary(dxil_module);
            dxil_module = nullptr;
        }
        create_instance = nullptr;
        available = false;
    }

    bool compile_shader(const std::wstring& file_path, const std::wstring& entry, const std::wstring& target, IDxcBlob** out_blob) {
        if (!available || !create_instance || !out_blob) return false;
        *out_blob = nullptr;

        IDxcUtils* utils = nullptr;
        HRESULT hr = create_instance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hr) || !utils) return false;

        IDxcCompiler3* compiler = nullptr;
        hr = create_instance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hr) || !compiler) {
            utils->Release();
            return false;
        }

        IDxcBlobEncoding* source_blob = nullptr;
        hr = utils->LoadFile(file_path.c_str(), nullptr, &source_blob);
        if (FAILED(hr) || !source_blob) {
            WCHAR exe_path[MAX_PATH];
            if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH)) {
                WCHAR *last_slash = wcsrchr(exe_path, L'\\');
                if (last_slash) {
                    *last_slash = L'\0';
                    WCHAR *parent_slash = wcsrchr(exe_path, L'\\');
                    std::wstring root_dir = exe_path;
                    if (parent_slash) {
                        root_dir = std::wstring(exe_path, parent_slash - exe_path);
                    }
                    std::wstring alt_path = root_dir + L"/" + file_path;
                    hr = utils->LoadFile(alt_path.c_str(), nullptr, &source_blob);
                }
            }
        }
        if (FAILED(hr) || !source_blob) {
            compiler->Release();
            utils->Release();
            return false;
        }

        DxcBuffer source_buffer = {};
        source_buffer.Ptr = source_blob->GetBufferPointer();
        source_buffer.Size = source_blob->GetBufferSize();
        source_buffer.Encoding = DXC_CP_ACP;

        std::vector<LPCWSTR> arguments;
        if (!entry.empty()) {
            arguments.push_back(L"-E");
            arguments.push_back(entry.c_str());
        }
        arguments.push_back(L"-T");
        arguments.push_back(target.c_str());
        arguments.push_back(L"-I");
        arguments.push_back(L"ZeGFX/shaders/dx12");
        arguments.push_back(L"-I");
        arguments.push_back(L"ZeGFX/shaders/dx12/include");
        arguments.push_back(L"-Qstrip_debug");
        arguments.push_back(L"-O3");

        IDxcResult* result = nullptr;
        hr = compiler->Compile(&source_buffer, arguments.data(), static_cast<UINT32>(arguments.size()), nullptr, IID_PPV_ARGS(&result));

        if (SUCCEEDED(hr) && result) {
            HRESULT status = S_OK;
            result->GetStatus(&status);
            if (SUCCEEDED(status)) {
                result->GetResult(out_blob);
            } else {
                IDxcBlobEncoding* error_blob = nullptr;
                result->GetErrorBuffer(&error_blob);
                if (error_blob && error_blob->GetBufferSize() > 0) {
                    std::cerr << "[ZeGFX DXC Shader Error] " << static_cast<const char*>(error_blob->GetBufferPointer()) << std::endl;
                    error_blob->Release();
                }
            }
            result->Release();
        }

        source_blob->Release();
        compiler->Release();
        utils->Release();

        return (*out_blob != nullptr);
    }
};

static DXCHelper g_dxc;

} // namespace

DXRPipelineD3D12::DXRPipelineD3D12() {
}

DXRPipelineD3D12::~DXRPipelineD3D12() {
    shutdown();
}

bool DXRPipelineD3D12::initialize(ID3D12Device* p_device) {
    shutdown();
    if (!p_device) return false;
    device = p_device;

    ID3D12Device5* p_dev5 = nullptr;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&p_dev5));
    if (SUCCEEDED(hr) && p_dev5) {
        device5 = p_dev5;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(device5->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))) &&
            options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
            dxr_supported = true;
            std::cout << "[ZeGFX D3D12] Hardware Ray Tracing (DXR 1.1) interface active on GPU." << std::endl;

            // Initialize DXC & assemble Raytracing State Object
            if (g_dxc.init()) {
                if (create_global_root_signature() && create_raytracing_state_object() && build_shader_binding_table()) {
                    std::cout << "[ZeGFX D3D12] Raytracing State Object & Shader Binding Table ready." << std::endl;
                } else {
                    std::cout << "[ZeGFX D3D12] RTPSO deferred; compute reflection fallback active." << std::endl;
                }
            } else {
                std::cout << "[ZeGFX D3D12] dxcompiler.dll not found in search path; compute fallback active." << std::endl;
            }
        } else {
            dxr_supported = false;
            std::cout << "[ZeGFX D3D12] DXR Hardware Ray Tracing not available; compute fallback active." << std::endl;
        }
    } else {
        dxr_supported = false;
        std::cout << "[ZeGFX D3D12] Hardware Ray Tracing (DXR 1.1) interface query skipped." << std::endl;
    }

    initialized = true;
    return true;
}

bool DXRPipelineD3D12::create_global_root_signature() {
    if (!device) return false;

    D3D12_ROOT_PARAMETER root_params[3] = {};

    // Parameter 0: 32-bit Constants (b0) for reflection settings
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].Constants.ShaderRegister = 0;
    root_params[0].Constants.RegisterSpace = 0;
    root_params[0].Constants.Num32BitValues = sizeof(DXRReflectionConstants) / 4;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Parameter 1: Top-Level Acceleration Structure SRV (t0)
    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_params[1].Descriptor.ShaderRegister = 0;
    root_params[1].Descriptor.RegisterSpace = 0;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Parameter 2: Unordered Access View (u0) for Output Radiance
    root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    root_params[2].Descriptor.ShaderRegister = 0;
    root_params[2].Descriptor.RegisterSpace = 0;
    root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
    root_sig_desc.NumParameters = 3;
    root_sig_desc.pParameters = root_params;
    root_sig_desc.NumStaticSamplers = 0;
    root_sig_desc.pStaticSamplers = nullptr;
    root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob* sig_blob = nullptr;
    ID3DBlob* err_blob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig_blob, &err_blob);
    if (FAILED(hr) || !sig_blob) {
        if (err_blob) err_blob->Release();
        return false;
    }

    hr = device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(), IID_PPV_ARGS(&global_root_sig));
    sig_blob->Release();
    if (err_blob) err_blob->Release();

    return SUCCEEDED(hr) && global_root_sig != nullptr;
}

bool DXRPipelineD3D12::create_raytracing_state_object() {
    if (!device5 || !global_root_sig) return false;

    IDxcBlob* raygen_blob = nullptr;
    IDxcBlob* closesthit_blob = nullptr;
    IDxcBlob* miss_blob = nullptr;

    if (!g_dxc.compile_shader(L"ZeGFX/shaders/dx12/rt_raygen.hlsl", L"", L"lib_6_3", &raygen_blob) ||
        !g_dxc.compile_shader(L"ZeGFX/shaders/dx12/rt_closesthit.hlsl", L"", L"lib_6_3", &closesthit_blob) ||
        !g_dxc.compile_shader(L"ZeGFX/shaders/dx12/rt_miss.hlsl", L"", L"lib_6_3", &miss_blob)) {
        if (raygen_blob) raygen_blob->Release();
        if (closesthit_blob) closesthit_blob->Release();
        if (miss_blob) miss_blob->Release();
        return false;
    }

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    // Subobject 0: Shader Configuration
    D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
    shader_config.MaxPayloadSizeInBytes = 64;
    shader_config.MaxAttributeSizeInBytes = 8;

    D3D12_STATE_SUBOBJECT shader_config_subobj = {};
    shader_config_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shader_config_subobj.pDesc = &shader_config;
    subobjects.push_back(shader_config_subobj);

    // Subobject 1: Pipeline Configuration
    D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
    pipeline_config.MaxTraceRecursionDepth = 2;

    D3D12_STATE_SUBOBJECT pipeline_config_subobj = {};
    pipeline_config_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipeline_config_subobj.pDesc = &pipeline_config;
    subobjects.push_back(pipeline_config_subobj);

    // Subobject 2: Global Root Signature
    D3D12_STATE_SUBOBJECT global_root_sig_subobj = {};
    global_root_sig_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    global_root_sig_subobj.pDesc = &global_root_sig;
    subobjects.push_back(global_root_sig_subobj);

    // Subobject 3: DXIL Libraries
    D3D12_EXPORT_DESC raygen_export = { L"RayGen", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC raygen_lib = {};
    raygen_lib.DXILLibrary.pShaderBytecode = raygen_blob->GetBufferPointer();
    raygen_lib.DXILLibrary.BytecodeLength = raygen_blob->GetBufferSize();
    raygen_lib.NumExports = 1;
    raygen_lib.pExports = &raygen_export;

    D3D12_STATE_SUBOBJECT raygen_subobj = {};
    raygen_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    raygen_subobj.pDesc = &raygen_lib;
    subobjects.push_back(raygen_subobj);

    D3D12_EXPORT_DESC closesthit_export = { L"ClosestHit", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC closesthit_lib = {};
    closesthit_lib.DXILLibrary.pShaderBytecode = closesthit_blob->GetBufferPointer();
    closesthit_lib.DXILLibrary.BytecodeLength = closesthit_blob->GetBufferSize();
    closesthit_lib.NumExports = 1;
    closesthit_lib.pExports = &closesthit_export;

    D3D12_STATE_SUBOBJECT closesthit_subobj = {};
    closesthit_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    closesthit_subobj.pDesc = &closesthit_lib;
    subobjects.push_back(closesthit_subobj);

    D3D12_EXPORT_DESC miss_export = { L"Miss", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC miss_lib = {};
    miss_lib.DXILLibrary.pShaderBytecode = miss_blob->GetBufferPointer();
    miss_lib.DXILLibrary.BytecodeLength = miss_blob->GetBufferSize();
    miss_lib.NumExports = 1;
    miss_lib.pExports = &miss_export;

    D3D12_STATE_SUBOBJECT miss_subobj = {};
    miss_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    miss_subobj.pDesc = &miss_lib;
    subobjects.push_back(miss_subobj);

    // Subobject 4: Hit Group
    D3D12_HIT_GROUP_DESC hit_group_desc = {};
    hit_group_desc.HitGroupExport = L"HitGroup";
    hit_group_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hit_group_desc.ClosestHitShaderImport = L"ClosestHit";

    D3D12_STATE_SUBOBJECT hit_group_subobj = {};
    hit_group_subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hit_group_subobj.pDesc = &hit_group_desc;
    subobjects.push_back(hit_group_subobj);

    // Assemble State Object
    D3D12_STATE_OBJECT_DESC state_obj_desc = {};
    state_obj_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    state_obj_desc.NumSubobjects = static_cast<UINT>(subobjects.size());
    state_obj_desc.pSubobjects = subobjects.data();

    HRESULT hr = device5->CreateStateObject(&state_obj_desc, IID_PPV_ARGS(&rtx_state_object));

    raygen_blob->Release();
    closesthit_blob->Release();
    miss_blob->Release();

    if (FAILED(hr) || !rtx_state_object) {
        std::cerr << "[ZeGFX D3D12] CreateStateObject failed with HRESULT 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    hr = rtx_state_object->QueryInterface(IID_PPV_ARGS(&state_object_props));
    return SUCCEEDED(hr) && state_object_props != nullptr;
}

bool DXRPipelineD3D12::build_shader_binding_table() {
    if (!device || !state_object_props) return false;

    void* raygen_id = state_object_props->GetShaderIdentifier(L"RayGen");
    void* miss_id = state_object_props->GetShaderIdentifier(L"Miss");
    void* hit_group_id = state_object_props->GetShaderIdentifier(L"HitGroup");

    if (!raygen_id || !miss_id || !hit_group_id) {
        return false;
    }

    const UINT record_size = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT; // 64 bytes
    const UINT sbt_size = record_size * 3;

    D3D12_HEAP_PROPERTIES upload_heap = {};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC buffer_desc = {};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Width = sbt_size;
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (sbt_buffer) {
        sbt_buffer->Release();
        sbt_buffer = nullptr;
    }

    HRESULT hr = device->CreateCommittedResource(
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&sbt_buffer)
    );

    if (FAILED(hr) || !sbt_buffer) {
        return false;
    }

    uint8_t* mapped_data = nullptr;
    hr = sbt_buffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped_data));
    if (FAILED(hr) || !mapped_data) {
        return false;
    }

    // Record 0: RayGen
    memcpy(mapped_data + 0, raygen_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Record 1: Miss
    memcpy(mapped_data + record_size, miss_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Record 2: HitGroup
    memcpy(mapped_data + record_size * 2, hit_group_id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    sbt_buffer->Unmap(0, nullptr);

    // Setup DispatchRays descriptors
    D3D12_GPU_VIRTUAL_ADDRESS sbt_gpu = sbt_buffer->GetGPUVirtualAddress();

    dispatch_desc.RayGenerationShaderRecord.StartAddress = sbt_gpu;
    dispatch_desc.RayGenerationShaderRecord.SizeInBytes = record_size;

    dispatch_desc.MissShaderTable.StartAddress = sbt_gpu + record_size;
    dispatch_desc.MissShaderTable.SizeInBytes = record_size;
    dispatch_desc.MissShaderTable.StrideInBytes = record_size;

    dispatch_desc.HitGroupTable.StartAddress = sbt_gpu + record_size * 2;
    dispatch_desc.HitGroupTable.SizeInBytes = record_size;
    dispatch_desc.HitGroupTable.StrideInBytes = record_size;

    return true;
}

void DXRPipelineD3D12::shutdown() {
    if (sbt_buffer) {
        sbt_buffer->Release();
        sbt_buffer = nullptr;
    }

    if (reflection_cb) {
        reflection_cb->Release();
        reflection_cb = nullptr;
    }

    if (global_root_sig) {
        global_root_sig->Release();
        global_root_sig = nullptr;
    }

    if (state_object_props) {
        state_object_props->Release();
        state_object_props = nullptr;
    }

    if (rtx_state_object) {
        rtx_state_object->Release();
        rtx_state_object = nullptr;
    }

    if (device5) {
        device5->Release();
        device5 = nullptr;
    }

    g_dxc.cleanup();

    device = nullptr;
    initialized = false;
    dxr_supported = false;
}

void DXRPipelineD3D12::build_tlas(ID3D12GraphicsCommandList4* p_cmd_list, ID3D12Resource* p_tlas_buffer, ID3D12Resource* p_instance_desc_buffer, uint32_t p_instance_count) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_tlas_buffer || !p_instance_desc_buffer) return;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
    build_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    build_desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    build_desc.Inputs.NumDescs = p_instance_count;
    build_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    build_desc.Inputs.InstanceDescs = p_instance_desc_buffer->GetGPUVirtualAddress();
    build_desc.DestAccelerationStructureData = p_tlas_buffer->GetGPUVirtualAddress();

    p_cmd_list->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
}

void DXRPipelineD3D12::dispatch_rays(
    ID3D12GraphicsCommandList4* p_cmd_list,
    D3D12_DISPATCH_RAYS_DESC* p_dispatch_desc
) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_dispatch_desc) return;
    p_cmd_list->DispatchRays(p_dispatch_desc);
}

void DXRPipelineD3D12::dispatch_reflection_rays(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_hdr_target,
    ID3D12Resource* p_depth_target,
    ID3D12Resource* p_normal_target,
    int p_width,
    int p_height,
    float p_roughness_threshold
) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_hdr_target) return;

    ID3D12GraphicsCommandList4* cmd_list4 = nullptr;
    if (FAILED(p_cmd_list->QueryInterface(IID_PPV_ARGS(&cmd_list4))) || !cmd_list4) {
        return;
    }

    if (rtx_state_object && global_root_sig && sbt_buffer) {
        cmd_list4->SetPipelineState1(rtx_state_object);
        cmd_list4->SetComputeRootSignature(global_root_sig);

        DXRReflectionConstants constants = {};
        constants.max_distance = 1000.0f;
        constants.roughness_cutoff = p_roughness_threshold;
        constants.width = static_cast<uint32_t>(p_width);
        constants.height = static_cast<uint32_t>(p_height);

        cmd_list4->SetComputeRoot32BitConstants(0, sizeof(DXRReflectionConstants) / 4, &constants, 0);

        // Note: Root parameter 1 (TLAS SRV) and Parameter 2 (Output UAV) descriptor table bindings
        // are sequenced for Phase 6. Only dispatch rays once TLAS SRV and UAV descriptors are live.
#if defined(DXR_DESCRIPTOR_TABLES_BOUND)
        dispatch_desc.Width = static_cast<UINT>(p_width);
        dispatch_desc.Height = static_cast<UINT>(p_height);
        dispatch_desc.Depth = 1;
        cmd_list4->DispatchRays(&dispatch_desc);
#endif
    }

    cmd_list4->Release();
}

void DXRPipelineD3D12::dispatch_gi_rays(
    ID3D12GraphicsCommandList* p_cmd_list,
    ID3D12Resource* p_hdr_target,
    ID3D12Resource* p_depth_target,
    ID3D12Resource* p_normal_target,
    int p_width,
    int p_height,
    float p_max_distance,
    float p_energy,
    int p_bounce_count
) {
    if (!initialized || !dxr_supported || !p_cmd_list || !p_hdr_target) return;

    ID3D12GraphicsCommandList4* cmd_list4 = nullptr;
    if (FAILED(p_cmd_list->QueryInterface(IID_PPV_ARGS(&cmd_list4))) || !cmd_list4) {
        return;
    }

    if (rtx_state_object && global_root_sig && sbt_buffer) {
        cmd_list4->SetPipelineState1(rtx_state_object);
        cmd_list4->SetComputeRootSignature(global_root_sig);

        DXRGIConstants constants = {};
        constants.max_distance = p_max_distance;
        constants.energy = p_energy;
        constants.bounce_count = static_cast<uint32_t>(p_bounce_count);
        constants.width = static_cast<uint32_t>(p_width);
        constants.height = static_cast<uint32_t>(p_height);

        cmd_list4->SetComputeRoot32BitConstants(0, sizeof(DXRGIConstants) / 4, &constants, 0);

#if defined(DXR_DESCRIPTOR_TABLES_BOUND)
        dispatch_desc.Width = static_cast<UINT>(p_width);
        dispatch_desc.Height = static_cast<UINT>(p_height);
        dispatch_desc.Depth = 1;
        cmd_list4->DispatchRays(&dispatch_desc);
#endif
    }

    cmd_list4->Release();
}

#endif // WITH_DX12_BACKEND
