/**************************************************************************/
/*  zegfx_d3d12_bridge.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifndef ZEGFX_D3D12_BRIDGE_H
#define ZEGFX_D3D12_BRIDGE_H

#include "core/string/ustring.h"

// Phase 1: Minimal bridge singleton. ZeGFX code is compiled and linked
// into the engine, but all D3D12 rendering is handled by Godot's native
// RenderingDeviceDriverD3D12. Later phases will add renderer/backend
// members and progressively route rendering through ZeGFX.

class ZeGFXD3D12Bridge {
private:
	static ZeGFXD3D12Bridge *singleton;
	bool initialized = false;

public:
	static ZeGFXD3D12Bridge *get_singleton() { return singleton; }

	ZeGFXD3D12Bridge();
	~ZeGFXD3D12Bridge();

	bool initialize(void *p_hwnd, String &r_error);
	void shutdown();

	bool is_initialized() const { return initialized; }
};

#endif // ZEGFX_D3D12_BRIDGE_H
