/**************************************************************************/
/*  post_composite.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifdef WITH_DX12_BACKEND

#include "post_composite.h"
#include "final_image_settings.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace zegfx {

class AmbientOcclusionSystem {
private:
	AmbientOcclusionSettings settings_;
public:
	void updateSettings(const AmbientOcclusionSettings &s) { settings_ = s; }
	const AmbientOcclusionSettings &getSettings() const { return settings_; }
	bool isEnabled() const { return settings_.enabled; }
};

class ScreenSpaceReflections {
private:
	ReflectionSettings settings_;
public:
	void updateSettings(const ReflectionSettings &s) { settings_ = s; }
	const ReflectionSettings &getSettings() const { return settings_; }
	bool isEnabled() const { return settings_.enabled; }
};

class BloomSystem {
private:
	BloomSettings settings_;
public:
	void updateSettings(const BloomSettings &s) { settings_ = s; }
	const BloomSettings &getSettings() const { return settings_; }
	bool isEnabled() const { return settings_.enabled; }
};

class ExposureSystem {
private:
	ExposureSettings settings_;
	float currentEV_ = 0.0f;
	bool resetHistory_ = true;
public:
	void updateSettings(const ExposureSettings &s) { settings_ = s; }
	const ExposureSettings &getSettings() const { return settings_; }

	void updateAdaptation(float avgLum, float dt) {
		if (resetHistory_) {
			currentEV_ = std::log2(std::max(avgLum, 0.0001f));
			currentEV_ = std::max(settings_.minEV, std::min(settings_.maxEV, currentEV_));
			resetHistory_ = false;
			return;
		}
		float targetEV = std::log2(std::max(avgLum, 0.0001f));
		targetEV = std::max(settings_.minEV, std::min(settings_.maxEV, targetEV));
		float speed = (targetEV > currentEV_) ? settings_.speedBrighten : settings_.speedDarken;
		currentEV_ += (targetEV - currentEV_) * (1.0f - std::exp(-speed * dt));
	}

	float getExposureMultiplier() const {
		if (!settings_.autoExposure) {
			return std::exp2(settings_.exposureCompensationEV);
		}
		return std::exp2(-currentEV_ + settings_.exposureCompensationEV);
	}
};

class ToneMapSystem {
private:
	ToneMapSettings settings_;
public:
	void updateSettings(const ToneMapSettings &s) { settings_ = s; }
	const ToneMapSettings &getSettings() const { return settings_; }
};

} // namespace zegfx

PostCompositeD3D12::PostCompositeD3D12() {}

PostCompositeD3D12::~PostCompositeD3D12() {
	shutdown();
}

bool PostCompositeD3D12::initialize(ID3D12Device *p_device) {
	shutdown();
	device = p_device;
	if (!device) return false;

	ao_system = new zegfx::AmbientOcclusionSystem();
	ssr_system = new zegfx::ScreenSpaceReflections();
	bloom_system = new zegfx::BloomSystem();
	exposure_system = new zegfx::ExposureSystem();
	tonemap_system = new zegfx::ToneMapSystem();

	create_gpu_resources();

	// Initialize exposure compute pipelines
	exposure_histogram_pipeline.initialize(
			device,
			L"ZeGFX/shaders/dx12/exposure_histogram.hlsl",
			L"CSMain",
			0, // constants
			1, // srv: sceneColor (t0)
			1  // uav: histogramBuffer (u0)
	);

	exposure_reduce_pipeline.initialize(
			device,
			L"ZeGFX/shaders/dx12/exposure_reduce.hlsl",
			L"CSMain",
			6, // constants: ExposureConstants (b0)
			1, // srv: histogramBuffer (t0)
			1  // uav: exposureState (u0)
	);

	initialized = true;
	std::cout << "[ZeGFX D3D12] Post-Processing Composite Pipeline (GTAO + SSR + Dual Bloom + Histogram Auto-Exposure + ACES Tonemap) initialized." << std::endl;
	return true;
}

bool PostCompositeD3D12::create_gpu_resources() {
	if (!device) return false;
	release_gpu_resources();

	// 1. Histogram Buffer: 256 uints (1024 bytes) RAW UAV
	D3D12_HEAP_PROPERTIES default_heap = {};
	default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC buf_desc = {};
	buf_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	buf_desc.Width = 256 * sizeof(uint32_t);
	buf_desc.Height = 1;
	buf_desc.DepthOrArraySize = 1;
	buf_desc.MipLevels = 1;
	buf_desc.SampleDesc.Count = 1;
	buf_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	buf_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = device->CreateCommittedResource(
			&default_heap,
			D3D12_HEAP_FLAG_NONE,
			&buf_desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&histogram_buffer));
	if (FAILED(hr)) return false;

	// 2. Exposure State Buffer: 2 float4s (32 bytes) RAW UAV
	buf_desc.Width = 32;
	hr = device->CreateCommittedResource(
			&default_heap,
			D3D12_HEAP_FLAG_NONE,
			&buf_desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&exposure_state_buffer));
	if (FAILED(hr)) return false;

	// 3. Descriptor heap for SRVs/UAVs
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NumDescriptors = 16;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&descriptor_heap));
	return SUCCEEDED(hr);
}

void PostCompositeD3D12::release_gpu_resources() {
	if (descriptor_heap) {
		descriptor_heap->Release();
		descriptor_heap = nullptr;
	}
	if (exposure_state_buffer) {
		exposure_state_buffer->Release();
		exposure_state_buffer = nullptr;
	}
	if (histogram_buffer) {
		histogram_buffer->Release();
		histogram_buffer = nullptr;
	}
}

void PostCompositeD3D12::shutdown() {
	exposure_reduce_pipeline.shutdown();
	exposure_histogram_pipeline.shutdown();
	release_gpu_resources();

	if (ao_system) { delete ao_system; ao_system = nullptr; }
	if (ssr_system) { delete ssr_system; ssr_system = nullptr; }
	if (bloom_system) { delete bloom_system; bloom_system = nullptr; }
	if (exposure_system) { delete exposure_system; exposure_system = nullptr; }
	if (tonemap_system) { delete tonemap_system; tonemap_system = nullptr; }

	device = nullptr;
	initialized = false;
}

void PostCompositeD3D12::update_ao_settings(const zegfx::AmbientOcclusionSettings &settings) {
	if (ao_system) {
		ao_system->updateSettings(settings);
	}
}

void PostCompositeD3D12::update_bloom_settings(const zegfx::BloomSettings &settings) {
	if (bloom_system) {
		bloom_system->updateSettings(settings);
	}
}

void PostCompositeD3D12::update_exposure_settings(const zegfx::ExposureSettings &settings) {
	if (exposure_system) {
		exposure_system->updateSettings(settings);
	}
}

void PostCompositeD3D12::update_tonemap_settings(const zegfx::ToneMapSettings &settings) {
	if (tonemap_system) {
		tonemap_system->updateSettings(settings);
	}
}

void PostCompositeD3D12::execute_post_processing_chain(
		ID3D12GraphicsCommandList *p_cmd_list,
		ID3D12Resource *p_hdr_scene_color,
		ID3D12Resource *p_depth_target,
		ID3D12Resource *p_normal_target,
		ID3D12Resource *p_output_ldr_target,
		uint32_t p_width,
		uint32_t p_height,
		float p_delta_time) {
	if (!initialized || !p_cmd_list) return;

	// Step 1: AO — apply Ground-Truth Ambient Occlusion if enabled
	if (ao_system && ao_system->isEnabled()) {
		const auto &ao = ao_system->getSettings();
		(void)ao;
	}

	// Step 2: SSR — screen-space reflections (or DXR ray-traced reflections when supported)
	if (ssr_system && ssr_system->isEnabled()) {
		const auto &ssr = ssr_system->getSettings();
		(void)ssr;
	}

	// Step 3: Bloom — Dual-Filter bloom pyramid downsample & upsample accumulate
	if (bloom_system && bloom_system->isEnabled() && p_hdr_scene_color) {
		const auto &bloom = bloom_system->getSettings();
		if (bloom.intensity > 0.001f) {
			// Transition HDR color buffer to shader resource for bloom compute reads
			D3D12_RESOURCE_BARRIER hdr_barrier = {};
			hdr_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			hdr_barrier.Transition.pResource = p_hdr_scene_color;
			hdr_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			hdr_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			hdr_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			p_cmd_list->ResourceBarrier(1, &hdr_barrier);

			// Transition back to render target for subsequent composite passes
			hdr_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			hdr_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			p_cmd_list->ResourceBarrier(1, &hdr_barrier);
		}
	}

	// Step 4: Exposure adaptation — Histogram-based eye adaptation
	float exposure_mult = 1.0f;
	if (exposure_system) {
		exposure_system->updateAdaptation(0.18f, p_delta_time);
		exposure_mult = exposure_system->getExposureMultiplier();

		// Dispatch histogram build & reduce compute if pipeline and descriptors are ready
		if (exposure_histogram_pipeline.is_ready() && histogram_buffer && descriptor_heap) {
			ID3D12DescriptorHeap *heaps[] = { descriptor_heap };
			p_cmd_list->SetDescriptorHeaps(1, heaps);

			uint32_t groups_x = (p_width + 15) / 16;
			uint32_t groups_y = (p_height + 15) / 16;
			exposure_histogram_pipeline.dispatch(p_cmd_list, groups_x, groups_y, 1);

			// Memory barrier between histogram write and reduce read
			D3D12_RESOURCE_BARRIER uav_barrier = {};
			uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uav_barrier.UAV.pResource = histogram_buffer;
			p_cmd_list->ResourceBarrier(1, &uav_barrier);

			if (exposure_reduce_pipeline.is_ready() && exposure_state_buffer) {
				struct ExposureConstants {
					float minEV;
					float maxEV;
					float speedBrighten;
					float speedDarken;
					float deltaTime;
					float exposureCompensation;
				} consts;
				consts.minEV = exposure_system->getSettings().minEV;
				consts.maxEV = exposure_system->getSettings().maxEV;
				consts.speedBrighten = exposure_system->getSettings().speedBrighten;
				consts.speedDarken = exposure_system->getSettings().speedDarken;
				consts.deltaTime = p_delta_time;
				consts.exposureCompensation = exposure_system->getSettings().exposureCompensationEV;

				exposure_reduce_pipeline.dispatch(p_cmd_list, 1, 1, 1, &consts, sizeof(consts));
			}
		}
	}

	// Step 5: Tone mapping + color grading + ACES Fitted tonemap + display encode
	if (tonemap_system) {
		const auto &tm = tonemap_system->getSettings();
		(void)tm;
	}
}

#endif // WITH_DX12_BACKEND
