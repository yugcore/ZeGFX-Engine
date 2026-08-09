/**************************************************************************/
/*  zegfx_d3d12_bridge.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "zegfx_d3d12_bridge.h"

#include "core/variant/variant.h"

ZeGFXD3D12Bridge *ZeGFXD3D12Bridge::singleton = nullptr;

ZeGFXD3D12Bridge::ZeGFXD3D12Bridge() {
	singleton = this;
}

ZeGFXD3D12Bridge::~ZeGFXD3D12Bridge() {
	shutdown();
	if (singleton == this) {
		singleton = nullptr;
	}
}

bool ZeGFXD3D12Bridge::initialize(void *p_hwnd, String &r_error) {
	if (initialized) {
		return true;
	}

	// Phase 1: ZeGFX is compiled and linked into the engine but operates
	// passively. All actual D3D12 rendering is handled by Godot's native
	// RenderingDeviceDriverD3D12. No ZeGFX renderer, backend, or window
	// is created — this avoids competing D3D12 devices and swap chains.
	// Later phases will progressively route rendering through ZeGFX.

	initialized = true;
	print_line("[ZeGFX] D3D12 bridge linked successfully (Phase 1).");
	return true;
}

void ZeGFXD3D12Bridge::shutdown() {
	if (!initialized) {
		return;
	}
	initialized = false;
}
