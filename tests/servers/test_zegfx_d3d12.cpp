/**************************************************************************/
/*  test_zegfx_d3d12.cpp                                                 */
/**************************************************************************/

#include "tests/servers/test_zegfx_d3d12.h"
#include "tests/test_macros.h"

#ifdef WITH_DX12_BACKEND
#include "drivers/d3d12/zegfx_d3d12_bridge.h"
#include "drivers/d3d12/d3d12_pipeline_state_manager.h"
#include "drivers/d3d12/gpu_command_queue.h"
#include "drivers/d3d12/dxr_pipeline.h"
#include "ZeGFX/include/render_graph.h"
#include "ZeGFX/include/graphics_backend.h"
#endif

namespace TestZeGFXD3D12 {

TEST_CASE("[ZeGFX][D3D12] Render Graph DAG Builder and Resource Barriers") {
    zegfx::RenderGraph graph;
    SUBCASE("Check physical resource registration") {
        zegfx::RenderResourceId id = graph.registerPhysicalResource(
            "TestBuffer", zegfx::RenderResourceType::Buffer, nullptr, zegfx::RenderResourceState::Unknown);
        CHECK(id != 0);
        CHECK(graph.getResource(id).name == "TestBuffer");
    }
}

TEST_CASE("[ZeGFX][D3D12] Hardware Capabilities and Pipeline State Manager") {
#ifdef WITH_DX12_BACKEND
    SUBCASE("Pipeline State Manager Initialization") {
        D3D12PipelineStateManager psoMgr;
        CHECK_FALSE(psoMgr.is_initialized());
    }

    SUBCASE("GPU Direct Command Signature Manager") {
        GPUCommandQueueD3D12 cmdQueue;
        CHECK_FALSE(cmdQueue.is_initialized());
    }

    SUBCASE("DXR Hardware Raytracing State Object Manager") {
        DXRPipelineD3D12 dxrPipeline;
        CHECK_FALSE(dxrPipeline.is_initialized());
    }
#endif
}

} // namespace TestZeGFXD3D12
