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
#include "scene/resources/environment.h"
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

    SUBCASE("GPU Direct Command Signature and Asynchronous Compute Queue") {
        GPUCommandQueueD3D12 cmdQueue;
        CHECK_FALSE(cmdQueue.is_initialized());
        CHECK_FALSE(cmdQueue.has_async_compute());
        CHECK(cmdQueue.get_indirect_draw_signature() == nullptr);
        CHECK(cmdQueue.get_compute_queue() == nullptr);
        CHECK(cmdQueue.begin_async_compute() == nullptr);

        // Safe no-op execution when uninitialized
        cmdQueue.end_and_execute_async_compute(nullptr);
        cmdQueue.sync_direct_queue_with_compute(nullptr);
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

        // Active command list and main queue tracking
        CHECK_FALSE(bridge.has_active_command_list());
        CHECK(bridge.get_active_command_list() == nullptr);
        bridge.set_active_command_list((void*)0x1234);
        CHECK(bridge.has_active_command_list());
        CHECK(bridge.get_active_command_list() == (void*)0x1234);
        bridge.set_active_command_list(nullptr);
        CHECK_FALSE(bridge.has_active_command_list());

        CHECK(bridge.get_main_command_queue() == nullptr);
        bridge.set_main_command_queue((void*)0x5678);
        CHECK(bridge.get_main_command_queue() == (void*)0x5678);
        bridge.set_main_command_queue(nullptr);
        CHECK(bridge.get_main_command_queue() == nullptr);

        // GPU scene instance registration
        Transform3D t;
        AABB aabb(Vector3(0, 0, 0), Vector3(2, 2, 2));
        bridge.register_scene_instance(t, aabb, 0);

        // Texture cooker safe error handling
        CHECK_FALSE(bridge.cook_and_load_ztex("", ""));

        // Flush deferred passes safely with null command list
        bridge.flush_deferred_passes(nullptr, nullptr, nullptr, nullptr, nullptr, 1920, 1080, 0.016f);
    }

    SUBCASE("Environment DXR Ray Tracing Controls and Clamping") {
        Ref<Environment> env;
        env.instantiate();
        CHECK(env.is_valid());
        CHECK(env->is_dxr_reflections_enabled());
        CHECK(env->get_dxr_reflection_roughness() == doctest::Approx(0.5f));

        env->set_dxr_reflections_enabled(false);
        CHECK_FALSE(env->is_dxr_reflections_enabled());

        env->set_dxr_reflections_enabled(true);
        CHECK(env->is_dxr_reflections_enabled());

        env->set_dxr_reflection_roughness(0.85f);
        CHECK(env->get_dxr_reflection_roughness() == doctest::Approx(0.85f));

        // Test clamp bounds [0.0, 1.0]
        env->set_dxr_reflection_roughness(-0.5f);
        CHECK(env->get_dxr_reflection_roughness() == doctest::Approx(0.0f));

        env->set_dxr_reflection_roughness(1.5f);
        CHECK(env->get_dxr_reflection_roughness() == doctest::Approx(1.0f));

        // DXR AO validation
        CHECK(env->is_dxr_ao_enabled());
        CHECK(env->get_dxr_ao_radius() == doctest::Approx(1.5f));
        CHECK(env->get_dxr_ao_intensity() == doctest::Approx(1.0f));

        env->set_dxr_ao_enabled(false);
        CHECK_FALSE(env->is_dxr_ao_enabled());

        env->set_dxr_ao_enabled(true);
        CHECK(env->is_dxr_ao_enabled());

        env->set_dxr_ao_radius(2.5f);
        CHECK(env->get_dxr_ao_radius() == doctest::Approx(2.5f));

        // Test clamp bounds (radius >= 0.1, intensity >= 0.0)
        env->set_dxr_ao_radius(0.01f);
        CHECK(env->get_dxr_ao_radius() == doctest::Approx(0.1f));

        env->set_dxr_ao_intensity(3.0f);
        CHECK(env->get_dxr_ao_intensity() == doctest::Approx(3.0f));

        env->set_dxr_ao_intensity(-1.0f);
        CHECK(env->get_dxr_ao_intensity() == doctest::Approx(0.0f));

        // DXR GI validation
        CHECK(env->is_dxr_gi_enabled());
        CHECK(env->get_dxr_gi_max_distance() == doctest::Approx(64.0f));
        CHECK(env->get_dxr_gi_energy() == doctest::Approx(1.0f));
        CHECK(env->get_dxr_gi_bounce_count() == 1);

        env->set_dxr_gi_enabled(false);
        CHECK_FALSE(env->is_dxr_gi_enabled());

        env->set_dxr_gi_enabled(true);
        CHECK(env->is_dxr_gi_enabled());

        env->set_dxr_gi_max_distance(128.0f);
        CHECK(env->get_dxr_gi_max_distance() == doctest::Approx(128.0f));

        env->set_dxr_gi_energy(2.5f);
        CHECK(env->get_dxr_gi_energy() == doctest::Approx(2.5f));

        env->set_dxr_gi_bounce_count(3);
        CHECK(env->get_dxr_gi_bounce_count() == 3);

        // Test clamp bounds (max_distance >= 1.0, energy >= 0.0, 1 <= bounce_count <= 4)
        env->set_dxr_gi_max_distance(0.1f);
        CHECK(env->get_dxr_gi_max_distance() == doctest::Approx(1.0f));

        env->set_dxr_gi_energy(-2.0f);
        CHECK(env->get_dxr_gi_energy() == doctest::Approx(0.0f));

        env->set_dxr_gi_bounce_count(0);
        CHECK(env->get_dxr_gi_bounce_count() == 1);

        env->set_dxr_gi_bounce_count(10);
        CHECK(env->get_dxr_gi_bounce_count() == 4);
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
