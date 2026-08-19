/**************************************************************************/
/*  test_knits.h                                                          */
/**************************************************************************/

#pragma once

#include "../knits_bytecode.h"
#include "../knits_compiler.h"
#include "../knits_node.h"
#include "../knits_script.h"
#include "../knits_types.h"
#include "../knits_vm.h"

#include "tests/test_macros.h"

namespace TestKnits {

TEST_CASE("[Knits] Random 64-bit UUID generation has zero collisions") {
	HashSet<uint64_t> ids;
	const int count = 2000;
	for (int i = 0; i < count; i++) {
		uint64_t id = KnitIDGenerator::generate();
		CHECK(id != 0);
		CHECK_FALSE(ids.has(id));
		ids.insert(id);
	}
	CHECK(ids.size() == count);
}

TEST_CASE("[Knits] Type signature compatibility and promotion") {
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	KnitTypeSignature sig_int32;
	sig_int32.kind = KnitDataType::Int32;

	KnitTypeSignature sig_string;
	sig_string.kind = KnitDataType::String;

	KnitTypeSignature sig_wildcard;
	sig_wildcard.kind = KnitDataType::Wildcard;

	// Exact match
	CHECK(sig_float.is_compatible_with(sig_float));
	CHECK(sig_int32.is_compatible_with(sig_int32));

	// Int32 promotes to Float
	CHECK(sig_int32.is_compatible_with(sig_float));

	// Incompatible types
	CHECK_FALSE(sig_string.is_compatible_with(sig_float));
	CHECK_FALSE(sig_float.is_compatible_with(sig_string));

	// Wildcard matches any type
	CHECK(sig_wildcard.is_compatible_with(sig_float));
	CHECK(sig_float.is_compatible_with(sig_wildcard));
}

TEST_CASE("[Knits] Graph construction and pin connection validation") {
	Ref<KnitsGraph> graph;
	graph.instantiate();

	Ref<KnitNode> node_a = graph->create_node(KnitNodeCategory::Event, "_ready", Vector2(100, 100));
	Ref<KnitNode> node_b = graph->create_node(KnitNodeCategory::ImpureAction, "print_value", Vector2(300, 100));

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;

	KnitPinID out_exec = node_a->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);
	KnitPinID in_exec = node_b->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);

	// Connect Output -> Input (Success)
	KnitConnectionID conn = graph->connect_pins(node_a->id, out_exec, node_b->id, in_exec);
	CHECK(conn != 0);
	CHECK(graph->connections.size() == 1);

	// Invalid connection: Input -> Output should fail
	KnitConnectionID invalid_conn = graph->connect_pins(node_b->id, in_exec, node_a->id, out_exec);
	CHECK(invalid_conn == 0);
}

TEST_CASE("[Knits] Compiler and Register VM math execution") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "CalculateMath";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;

	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	// 1. Entry node
	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out_exec = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// 2. Pure Add node: (5.0 + 3.0)
	Ref<KnitNode> add_node = graph->create_node(KnitNodeCategory::PureFunction, "add", Vector2(100, 250));
	KnitPinID add_in_a = add_node->add_input_pin("a", KnitPinKind::Data, sig_float, 5.0);
	KnitPinID add_in_b = add_node->add_input_pin("b", KnitPinKind::Data, sig_float, 3.0);
	KnitPinID add_out = add_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// 3. Pure Mul node: (add_res * 2.0)
	Ref<KnitNode> mul_node = graph->create_node(KnitNodeCategory::PureFunction, "mul", Vector2(300, 250));
	KnitPinID mul_in_a = mul_node->add_input_pin("a", KnitPinKind::Data, sig_float);
	KnitPinID mul_in_b = mul_node->add_input_pin("b", KnitPinKind::Data, sig_float, 2.0);
	KnitPinID mul_out = mul_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Wire add_out -> mul_in_a
	graph->connect_pins(add_node->id, add_out, mul_node->id, mul_in_a);

	// 4. Return node
	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(500, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float);

	// Wire entry -> return exec
	graph->connect_pins(entry->id, entry_out_exec, ret_node->id, ret_in_exec);
	// Wire mul_out -> return value
	graph->connect_pins(mul_node->id, mul_out, ret_node->id, ret_in_val);

	// Compile graph
	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);
	CHECK(error.is_empty());

	// Execute in VM
	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(double(return_val) == doctest::Approx(16.0));
}

TEST_CASE("[Knits] Resumable coroutine state-machine frame suspension and resumption") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "YieldTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out_exec = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	Ref<KnitNode> yield_node = graph->create_node(KnitNodeCategory::ImpureAction, "yield_seconds", Vector2(250, 100));
	KnitPinID yield_in_exec = yield_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID yield_in_sec = yield_node->add_input_pin("seconds", KnitPinKind::Data, sig_float, 0.5);
	KnitPinID yield_out_exec = yield_node->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	graph->connect_pins(entry->id, entry_out_exec, yield_node->id, yield_in_exec);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(450, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float, 42.0);

	graph->connect_pins(yield_node->id, yield_out_exec, ret_node->id, ret_in_exec);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	// Execute with dummy object instance
	Ref<RefCounted> test_obj;
	test_obj.instantiate();

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, test_obj.ptr(), nullptr, 0, return_val);

	// VM should yield on YIELD_SECONDS
	CHECK(status == KnitVMStatus::Yielded);
	CHECK(vm.get_suspended_frame_count() == 1);

	// Advance timer by 0.3s -> should still be suspended
	vm.tick_coroutines(0.3);
	CHECK(vm.get_suspended_frame_count() == 1);

	// Advance timer by another 0.3s (total 0.6s >= 0.5s) -> ready to resume
	vm.tick_coroutines(0.3);
	CHECK(vm.get_suspended_frame_count() == 0);
}

TEST_CASE("[Knits] Unconstrained wildcard generic compile-time rejection") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "UnconstrainedGenericTest";

	KnitTypeSignature sig_wildcard;
	sig_wildcard.kind = KnitDataType::Wildcard;
	sig_wildcard.generic_symbol = "T";

	Ref<KnitNode> generic_node = graph->create_node(KnitNodeCategory::PureFunction, "array_get", Vector2(100, 100));
	generic_node->add_input_pin("element", KnitPinKind::Data, sig_wildcard);

	// Compile without resolving T in generic_bindings
	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);

	// Must fail at compile time with unresolved generic error
	CHECK_FALSE(ok);
	CHECK(error.find("Unresolved generic type 'T'") != -1);
}

TEST_CASE("[Knits] Fault trapping on division by zero") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "DivByZeroTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out_exec = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	Ref<KnitNode> div_node = graph->create_node(KnitNodeCategory::PureFunction, "div", Vector2(100, 250));
	div_node->add_input_pin("a", KnitPinKind::Data, sig_float, 10.0);
	div_node->add_input_pin("b", KnitPinKind::Data, sig_float, 0.0); // Div by zero!
	KnitPinID div_out = div_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(300, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float);

	graph->connect_pins(entry->id, entry_out_exec, ret_node->id, ret_in_exec);
	graph->connect_pins(div_node->id, div_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	// VM must trap fault
	CHECK(status == KnitVMStatus::Fault);
	CHECK(vm.has_fault());
	CHECK(vm.get_last_fault().message.find("Division by zero") != -1);
}

} // namespace TestKnits
