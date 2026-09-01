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
#include "ZeGFX/include/cooked_asset_serialization.h"
#endif

namespace TestZeGFXD3D12 {

#ifdef WITH_DX12_BACKEND
TEST_CASE("[ZeGFX][D3D12] Render Graph DAG Builder and Resource Barriers") {
    zegfx::RenderGraph graph;
    SUBCASE("Check physical resource registration") {
        zegfx::RenderResourceId id = graph.registerPhysicalResource(
            "TestBuffer", zegfx::RenderResourceType::Buffer, nullptr, zegfx::RenderResourceState::Unknown);
        CHECK(graph.getResource(id).name == "TestBuffer");
        zegfx::RenderResourceId found_id = 0;
        CHECK(graph.findResource("TestBuffer", found_id));
        CHECK(found_id == id);
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

#ifdef WITH_DX12_BACKEND
TEST_CASE("[ZeGFX][D3D12] Cooked Asset Serialization and Octahedral Vertex Packing") {
    SUBCASE("Packed Vertex Structure Layout and Stride Validation") {
        CHECK(sizeof(zegfx::DX12Vertex3DTextured) == 48);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, x) == 0);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, octNormal) == 12);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, octTangent) == 16);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, u) == 20);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, u1) == 24);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, colorRgba) == 28);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, jointIndices) == 32);
        CHECK(offsetof(zegfx::DX12Vertex3DTextured, jointWeights) == 40);
    }

    SUBCASE("Octahedral Normal Encoding & Decoding Parity") {
        float testVectors[5][3] = {
            { 0.0f, 1.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, -1.0f },
            { 0.57735f, 0.57735f, 0.57735f }
        };

        for (int i = 0; i < 5; ++i) {
            int16_t oct[2] = {};
            zegfx::encodeOctahedralSNORM16(testVectors[i][0], testVectors[i][1], testVectors[i][2], oct);

            float rx = 0.0f, ry = 0.0f, rz = 0.0f;
            zegfx::decodeOctahedralSNORM16(oct, rx, ry, rz);

            CHECK(doctest::Approx(rx).epsilon(0.01f) == testVectors[i][0]);
            CHECK(doctest::Approx(ry).epsilon(0.01f) == testVectors[i][1]);
            CHECK(doctest::Approx(rz).epsilon(0.01f) == testVectors[i][2]);
        }
    }

    SUBCASE("Octahedral Tangent Handedness Preservation") {
        float tx = 1.0f, ty = 0.0f, tz = 0.0f;

        int16_t octPositive[2] = {};
        zegfx::encodeOctahedralTangentSNORM16(tx, ty, tz, 1.0f, octPositive);
        float rx = 0.0f, ry = 0.0f, rz = 0.0f, rw = 0.0f;
        zegfx::decodeOctahedralTangentSNORM16(octPositive, rx, ry, rz, rw);
        CHECK(rw == 1.0f);
        CHECK(doctest::Approx(rx).epsilon(0.01f) == 1.0f);

        int16_t octNegative[2] = {};
        zegfx::encodeOctahedralTangentSNORM16(tx, ty, tz, -1.0f, octNegative);
        zegfx::decodeOctahedralTangentSNORM16(octNegative, rx, ry, rz, rw);
        CHECK(rw == -1.0f);
        CHECK(doctest::Approx(rx).epsilon(0.01f) == 1.0f);
    }

    SUBCASE("Float16 Half Precision Round Trip") {
        float testVals[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, -0.5f, 12.34f };
        for (float v : testVals) {
            uint16_t h = zegfx::floatToHalf(v);
            float r = zegfx::halfToFloat(h);
            CHECK(doctest::Approx(r).epsilon(0.01f) == v);
        }
    }
}
#endif

} // namespace TestZeGFXD3D12
