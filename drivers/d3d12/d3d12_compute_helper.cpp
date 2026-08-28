/**************************************************************************/
/*  d3d12_compute_helper.cpp                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "d3d12_compute_helper.h"
#include <iostream>
#include <vector>
#include <dxcapi.h>

namespace zegfx {

namespace {

struct DXCHelperGlobal {
	HMODULE dxc_module = nullptr;
	HMODULE dxil_module = nullptr;
	DxcCreateInstanceProc create_instance = nullptr;
	bool available = false;

	bool init() {
		if (available) return true;

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

	bool compile(const std::wstring &file_path, const std::wstring &entry, const std::wstring &target, IDxcBlob **out_blob) {
		if (!init() || !create_instance || !out_blob) return false;
		*out_blob = nullptr;

		IDxcUtils *utils = nullptr;
		HRESULT hr = create_instance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr) || !utils) return false;

		IDxcCompiler3 *compiler = nullptr;
		hr = create_instance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr) || !compiler) {
			utils->Release();
			return false;
		}

		IDxcBlobEncoding *source_blob = nullptr;
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

		IDxcResult *result = nullptr;
		hr = compiler->Compile(&source_buffer, arguments.data(), static_cast<UINT32>(arguments.size()), nullptr, IID_PPV_ARGS(&result));

		if (SUCCEEDED(hr) && result) {
			HRESULT status = S_OK;
			result->GetStatus(&status);
			if (SUCCEEDED(status)) {
				result->GetResult(out_blob);
			} else {
				IDxcBlobEncoding *error_blob = nullptr;
				result->GetErrorBuffer(&error_blob);
				if (error_blob && error_blob->GetBufferSize() > 0) {
					std::cerr << "[ZeGFX DXC Compute Error] " << static_cast<const char *>(error_blob->GetBufferPointer()) << std::endl;
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

static DXCHelperGlobal g_dxc_compute;

} // namespace

bool compile_hlsl_compute(
		const std::wstring &p_shader_file,
		const std::wstring &p_entry_point,
		const std::wstring &p_target,
		IDxcBlob **r_blob) {
	return g_dxc_compute.compile(p_shader_file, p_entry_point, p_target, r_blob);
}

D3D12ComputePipeline::D3D12ComputePipeline() {}

D3D12ComputePipeline::~D3D12ComputePipeline() {
	shutdown();
}

bool D3D12ComputePipeline::initialize(
		ID3D12Device *p_device,
		const std::wstring &p_shader_file,
		const std::wstring &p_entry_point,
		uint32_t p_constant_dword_count,
		uint32_t p_srv_count,
		uint32_t p_uav_count) {
	shutdown();
	if (!p_device) return false;
	device = p_device;
	constant_dwords = p_constant_dword_count;
	has_srvs = (p_srv_count > 0);
	has_uavs = (p_uav_count > 0);

	// 1. Compile compute shader
	IDxcBlob *cs_blob = nullptr;
	if (!compile_hlsl_compute(p_shader_file, p_entry_point, L"cs_6_0", &cs_blob)) {
		std::cerr << "[ZeGFX] Failed to compile compute shader: " << std::string(p_shader_file.begin(), p_shader_file.end()) << std::endl;
		return false;
	}

	// 2. Build Root Signature
	std::vector<D3D12_ROOT_PARAMETER> root_params;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;

	// Parameter 0: 32-bit Root Constants (b0)
	if (p_constant_dword_count > 0) {
		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param.Constants.ShaderRegister = 0;
		param.Constants.RegisterSpace = 0;
		param.Constants.Num32BitValues = p_constant_dword_count;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		root_params.push_back(param);
	}

	// Parameter 1: SRV Table (t0..tN)
	if (has_srvs) {
		D3D12_DESCRIPTOR_RANGE srv_range = {};
		srv_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srv_range.NumDescriptors = p_srv_count;
		srv_range.BaseShaderRegister = 0;
		srv_range.RegisterSpace = 0;
		srv_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges.push_back(srv_range);

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &ranges.back();
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		root_params.push_back(param);
	}

	// Parameter 2: UAV Table (u0..uN)
	if (has_uavs) {
		D3D12_DESCRIPTOR_RANGE uav_range = {};
		uav_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uav_range.NumDescriptors = p_uav_count;
		uav_range.BaseShaderRegister = 0;
		uav_range.RegisterSpace = 0;
		uav_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges.push_back(uav_range);

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &ranges.back();
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		root_params.push_back(param);
	}

	// Static Sampler (s0: Linear Clamp)
	D3D12_STATIC_SAMPLER_DESC linear_clamp_sampler = {};
	linear_clamp_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linear_clamp_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linear_clamp_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linear_clamp_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linear_clamp_sampler.MipLODBias = 0;
	linear_clamp_sampler.MaxAnisotropy = 1;
	linear_clamp_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	linear_clamp_sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	linear_clamp_sampler.MinLOD = 0.0f;
	linear_clamp_sampler.MaxLOD = D3D12_FLOAT32_MAX;
	linear_clamp_sampler.ShaderRegister = 0;
	linear_clamp_sampler.RegisterSpace = 0;
	linear_clamp_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC root_desc = {};
	root_desc.NumParameters = static_cast<UINT>(root_params.size());
	root_desc.pParameters = root_params.empty() ? nullptr : root_params.data();
	root_desc.NumStaticSamplers = 1;
	root_desc.pStaticSamplers = &linear_clamp_sampler;
	root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob *sig_blob = nullptr;
	ID3DBlob *err_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig_blob, &err_blob);
	if (FAILED(hr) || !sig_blob) {
		if (err_blob) err_blob->Release();
		cs_blob->Release();
		return false;
	}

	hr = device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature));
	sig_blob->Release();
	if (err_blob) err_blob->Release();
	if (FAILED(hr) || !root_signature) {
		cs_blob->Release();
		return false;
	}

	// 3. Create Compute Pipeline State
	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.pRootSignature = root_signature;
	pso_desc.CS.pShaderBytecode = cs_blob->GetBufferPointer();
	pso_desc.CS.BytecodeLength = cs_blob->GetBufferSize();

	hr = device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pso));
	cs_blob->Release();

	return SUCCEEDED(hr) && pso != nullptr;
}

void D3D12ComputePipeline::shutdown() {
	if (pso) {
		pso->Release();
		pso = nullptr;
	}
	if (root_signature) {
		root_signature->Release();
		root_signature = nullptr;
	}
	device = nullptr;
}

void D3D12ComputePipeline::dispatch(
		ID3D12GraphicsCommandList *p_cmd_list,
		uint32_t p_groups_x,
		uint32_t p_groups_y,
		uint32_t p_groups_z,
		const void *p_constants,
		size_t p_constants_size_bytes,
		D3D12_GPU_DESCRIPTOR_HANDLE p_srv_table,
		D3D12_GPU_DESCRIPTOR_HANDLE p_uav_table) {
	if (!is_ready() || !p_cmd_list) return;

	p_cmd_list->SetPipelineState(pso);
	p_cmd_list->SetComputeRootSignature(root_signature);

	UINT param_idx = 0;
	if (constant_dwords > 0) {
		if (p_constants && p_constants_size_bytes > 0) {
			UINT dwords_to_set = std::min<UINT>(constant_dwords, static_cast<UINT>(p_constants_size_bytes / 4));
			p_cmd_list->SetComputeRoot32BitConstants(param_idx, dwords_to_set, p_constants, 0);
		}
		param_idx++;
	}

	if (has_srvs) {
		if (p_srv_table.ptr != 0) {
			p_cmd_list->SetComputeRootDescriptorTable(param_idx, p_srv_table);
		}
		param_idx++;
	}

	if (has_uavs) {
		if (p_uav_table.ptr != 0) {
			p_cmd_list->SetComputeRootDescriptorTable(param_idx, p_uav_table);
		}
		param_idx++;
	}

	p_cmd_list->Dispatch(p_groups_x, p_groups_y, p_groups_z);
}

} // namespace zegfx

#endif // WITH_DX12_BACKEND
