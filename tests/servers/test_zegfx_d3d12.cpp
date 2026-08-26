/**************************************************************************/
/*  test_zegfx_d3d12.cpp                                                 */
/**************************************************************************/

#include "tests/servers/test_zegfx_d3d12.h"
#include "tests/test_macros.h"

TEST_FORCE_LINK(test_zegfx_d3d12)

#ifdef WITH_DX12_BACKEND
#include "drivers/d3d12/zegfx_d3d12_bridge.h"
#include "drivers/d3d12/d3d12_pipeline_state_manager.h"
#include "drivers/d3d12/gpu_command_queue.h"
#include "drivers/d3d12/dxr_pipeline.h"
#include "ZeGFX/include/render_graph.h"
#include "ZeGFX/include/graphics_backend.h"
#endif

namespace TestZeGFXD3D12 {

#ifdef WITH_DX12_BACKEND
TEST_CASE("[ZeGFX][D3D12] Render Graph DAG Builder and Resource Barriers") {
    zegfx::RenderGraph graph;
    SUBCASE("Check physical resource registration") {
        zegfx::RenderResourceId id = graph.registerPhysicalResource(
            "TestBuffer", zegfx::RenderResourceType::Buffer, nullptr, zegfx::RenderResourceState::Unknown);
        CHECK(id != 0);
        CHECK(graph.getResource(id).name == "TestBuffer");
    }
}
#endif

TEST_CASE("[ZeGFX][D3D12] Hardware Capabilities and Pipeline State Manager") {
#ifdef WITH_DX12_BACKEND
    SUBCASE("Pipeline State Manager Initialization") {
        zegfx::D3D12PipelineStateManager psoMgr;
        CHECK(psoMgr.getCachedPipelineCount() == 0);
    }

    SUBCASE("GPU Direct Command Signature Manager") {
        GPUCommandQueueD3D12 cmdQueue;
        CHECK_FALSE(cmdQueue.is_initialized());
    }

    SUBCASE("DXR Hardware Raytracing State Object Manager") {
        DXRPipelineD3D12 dxrPipeline;
        CHECK_FALSE(dxrPipeline.is_initialized());
    }

    SUBCASE("Bridge Subsystem Dispatch and Deferred Pass Tracking") {
        ZeGFXD3D12Bridge bridge;
        String err;
        CHECK(bridge.initialize(nullptr, err));
        CHECK(bridge.is_initialized());

        Vector<float> splits;
        CHECK(bridge.execute_shadow_pass(0.1f, 100.0f, 4, splits));
        CHECK(splits.size() == 3);

        CHECK(bridge.execute_ao_pass(1920, 1080, 1.5f, 1.0f));
        CHECK_FALSE(bridge.ao_pass_active()); // returns false because no command list is attached

        // DXR reflections will return false without ID3D12Device initialization
        CHECK_FALSE(bridge.execute_dxr_reflections_pass(1920, 1080, 0.5f));
        CHECK_FALSE(bridge.dxr_reflections_active());

        CHECK(bridge.execute_post_process_pass(1920, 1080, 1.0f, 0.5f, 1, 0.04f, 0.1f));

        CHECK(bridge.execute_meshlet_streamer_pass("res://test.zmesh", 0, 1));
        CHECK_FALSE(bridge.execute_meshlet_streamer_pass("", 0, 1));

        // Flush deferred passes safely with null command list
        bridge.flush_deferred_passes(nullptr, nullptr, nullptr, nullptr, nullptr, 1920, 1080, 0.016f);
    }
#endif
}

} // namespace TestZeGFXD3D12
