/**************************************************************************/
/*  d3d12_compute_helper.h                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#ifdef WITH_DX12_BACKEND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include <cstdint>
struct IDxcBlob;

namespace zegfx {

class D3D12ComputePipeline {
public:
	D3D12ComputePipeline();
	~D3D12ComputePipeline();

	// Initialize and compile compute shader from HLSL source file
	bool initialize(
			ID3D12Device *p_device,
			const std::wstring &p_shader_file,
			const std::wstring &p_entry_point = L"main",
			uint32_t p_constant_dword_count = 16,
			uint32_t p_srv_count = 8,
			uint32_t p_uav_count = 4);

	void shutdown();

	bool is_ready() const { return pso != nullptr && root_signature != nullptr; }

	ID3D12RootSignature *get_root_signature() const { return root_signature; }
	ID3D12PipelineState *get_pipeline_state() const { return pso; }

	// Bind and dispatch compute shader with push constants
	void dispatch(
			ID3D12GraphicsCommandList *p_cmd_list,
			uint32_t p_groups_x,
			uint32_t p_groups_y,
			uint32_t p_groups_z,
			const void *p_constants = nullptr,
			size_t p_constants_size_bytes = 0,
			D3D12_GPU_DESCRIPTOR_HANDLE p_srv_table = {},
			D3D12_GPU_DESCRIPTOR_HANDLE p_uav_table = {});

private:
	ID3D12Device *device = nullptr;
	ID3D12RootSignature *root_signature = nullptr;
	ID3D12PipelineState *pso = nullptr;
	uint32_t constant_dwords = 0;
	bool has_srvs = false;
	bool has_uavs = false;
};

// Global DXC compiler loader helper
bool compile_hlsl_compute(
		const std::wstring &p_shader_file,
		const std::wstring &p_entry_point,
		const std::wstring &p_target,
		IDxcBlob **r_blob);

} // namespace zegfx

#endif // WITH_DX12_BACKEND
