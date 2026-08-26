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

TEST_CASE("[Knits] Extended math and bitwise operations") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "BitwiseMathTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// Modulo node: 14.0 % 4.0 = 2.0
	Ref<KnitNode> mod_node = graph->create_node(KnitNodeCategory::PureFunction, "mod", Vector2(100, 200));
	KnitPinID mod_a = mod_node->add_input_pin("a", KnitPinKind::Data, sig_float, 14.0);
	KnitPinID mod_b = mod_node->add_input_pin("b", KnitPinKind::Data, sig_float, 4.0);
	KnitPinID mod_out = mod_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Pow node: 2.0 ** 3.0 = 8.0
	Ref<KnitNode> pow_node = graph->create_node(KnitNodeCategory::PureFunction, "pow", Vector2(300, 200));
	KnitPinID pow_a = pow_node->add_input_pin("base", KnitPinKind::Data, sig_float);
	KnitPinID pow_b = pow_node->add_input_pin("exp", KnitPinKind::Data, sig_float, 3.0);
	KnitPinID pow_out = pow_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	graph->connect_pins(mod_node->id, mod_out, pow_node->id, pow_a);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(500, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float);

	graph->connect_pins(entry->id, entry_out, ret_node->id, ret_in_exec);
	graph->connect_pins(pow_node->id, pow_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(double(return_val) == doctest::Approx(8.0));
}

TEST_CASE("[Knits] Utility function invocation via CALL_UTILITY") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "UtilityFuncTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// Sqrt utility call: sqrt(64.0) = 8.0
	Ref<KnitNode> sqrt_node = graph->create_node(KnitNodeCategory::PureFunction, "sqrt", Vector2(150, 200));
	sqrt_node->add_input_pin("x", KnitPinKind::Data, sig_float, 64.0);
	KnitPinID sqrt_out = sqrt_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Clamp utility call: clamp(8.0, 0.0, 5.0) = 5.0
	Ref<KnitNode> clamp_node = graph->create_node(KnitNodeCategory::PureFunction, "clamp", Vector2(350, 200));
	KnitPinID clamp_val = clamp_node->add_input_pin("val", KnitPinKind::Data, sig_float);
	clamp_node->add_input_pin("min", KnitPinKind::Data, sig_float, 0.0);
	clamp_node->add_input_pin("max", KnitPinKind::Data, sig_float, 5.0);
	KnitPinID clamp_out = clamp_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	graph->connect_pins(sqrt_node->id, sqrt_out, clamp_node->id, clamp_val);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(550, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float);

	graph->connect_pins(entry->id, entry_out, ret_node->id, ret_in_exec);
	graph->connect_pins(clamp_node->id, clamp_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(double(return_val) == doctest::Approx(5.0));
}

TEST_CASE("[Knits] Container operations parity (Array and Dictionary)") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "ContainerParityTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_array;
	sig_array.kind = KnitDataType::Array;
	KnitTypeSignature sig_dict;
	sig_dict.kind = KnitDataType::Dictionary;
	KnitTypeSignature sig_int;
	sig_int.kind = KnitDataType::Int64;
	KnitTypeSignature sig_string;
	sig_string.kind = KnitDataType::String;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// 1. Array construct: [100, 200]
	Ref<KnitNode> arr_create = graph->create_node(KnitNodeCategory::PureFunction, "array_construct", Vector2(100, 200));
	arr_create->add_input_pin("elem0", KnitPinKind::Data, sig_int, 100);
	arr_create->add_input_pin("elem1", KnitPinKind::Data, sig_int, 200);
	KnitPinID arr_out = arr_create->add_output_pin("array", KnitPinKind::Data, sig_array);

	// 2. Array size (len): len([100, 200]) == 2
	Ref<KnitNode> arr_size = graph->create_node(KnitNodeCategory::PureFunction, "array_size", Vector2(300, 200));
	KnitPinID arr_size_in = arr_size->add_input_pin("array", KnitPinKind::Data, sig_array);
	KnitPinID arr_size_out = arr_size->add_output_pin("size", KnitPinKind::Data, sig_int);

	graph->connect_pins(arr_create->id, arr_out, arr_size->id, arr_size_in);

	// 3. Dict construct: {"score": 99}
	Ref<KnitNode> dict_create = graph->create_node(KnitNodeCategory::PureFunction, "dict_construct", Vector2(100, 350));
	dict_create->add_input_pin("key0", KnitPinKind::Data, sig_string, "score");
	dict_create->add_input_pin("val0", KnitPinKind::Data, sig_int, 99);
	KnitPinID dict_out = dict_create->add_output_pin("dict", KnitPinKind::Data, sig_dict);

	// 4. Dict get: dict.get("score") == 99
	Ref<KnitNode> dict_get = graph->create_node(KnitNodeCategory::PureFunction, "dict_get", Vector2(300, 350));
	KnitPinID dict_get_in = dict_get->add_input_pin("dict", KnitPinKind::Data, sig_dict);
	dict_get->add_input_pin("key", KnitPinKind::Data, sig_string, "score");
	dict_get->add_input_pin("default", KnitPinKind::Data, sig_int, 0);
	KnitPinID dict_get_out = dict_get->add_output_pin("val", KnitPinKind::Data, sig_int);

	graph->connect_pins(dict_create->id, dict_out, dict_get->id, dict_get_in);

	// 5. Add array_size + dict_get: 2 + 99 = 101
	Ref<KnitNode> add_node = graph->create_node(KnitNodeCategory::PureFunction, "add", Vector2(500, 250));
	KnitPinID add_a = add_node->add_input_pin("a", KnitPinKind::Data, sig_int);
	KnitPinID add_b = add_node->add_input_pin("b", KnitPinKind::Data, sig_int);
	KnitPinID add_out = add_node->add_output_pin("res", KnitPinKind::Data, sig_int);

	graph->connect_pins(arr_size->id, arr_size_out, add_node->id, add_a);
	graph->connect_pins(dict_get->id, dict_get_out, add_node->id, add_b);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(700, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_int);

	graph->connect_pins(entry->id, entry_out, ret_node->id, ret_in_exec);
	graph->connect_pins(add_node->id, add_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(int64_t(return_val) == 101);
}

TEST_CASE("[Knits] Trigonometry and interpolation parity (sin, cos, lerp, remap)") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "TrigLerpTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_float;
	sig_float.kind = KnitDataType::Float;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// Lerp node: lerp(10.0, 30.0, 0.5) = 20.0
	Ref<KnitNode> lerp_node = graph->create_node(KnitNodeCategory::PureFunction, "lerp", Vector2(150, 200));
	lerp_node->add_input_pin("from", KnitPinKind::Data, sig_float, 10.0);
	lerp_node->add_input_pin("to", KnitPinKind::Data, sig_float, 30.0);
	lerp_node->add_input_pin("weight", KnitPinKind::Data, sig_float, 0.5);
	KnitPinID lerp_out = lerp_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	// Remap node: remap(20.0, 10.0, 30.0, 0.0, 100.0) = 50.0
	Ref<KnitNode> remap_node = graph->create_node(KnitNodeCategory::PureFunction, "remap", Vector2(350, 200));
	KnitPinID remap_val = remap_node->add_input_pin("val", KnitPinKind::Data, sig_float);
	remap_node->add_input_pin("istart", KnitPinKind::Data, sig_float, 10.0);
	remap_node->add_input_pin("istop", KnitPinKind::Data, sig_float, 30.0);
	remap_node->add_input_pin("ostart", KnitPinKind::Data, sig_float, 0.0);
	remap_node->add_input_pin("ostop", KnitPinKind::Data, sig_float, 100.0);
	KnitPinID remap_out = remap_node->add_output_pin("res", KnitPinKind::Data, sig_float);

	graph->connect_pins(lerp_node->id, lerp_out, remap_node->id, remap_val);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(550, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_float);

	graph->connect_pins(entry->id, entry_out, ret_node->id, ret_in_exec);
	graph->connect_pins(remap_node->id, remap_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(double(return_val) == doctest::Approx(50.0));
}

TEST_CASE("[Knits] Bitwise logical operators parity (&, |, ^, ~, <<, >>)") {
	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = "BitwiseOpsTest";

	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	KnitTypeSignature sig_int;
	sig_int.kind = KnitDataType::Int64;

	Ref<KnitNode> entry = graph->create_node(KnitNodeCategory::Event, "Entry", Vector2(50, 100));
	KnitPinID entry_out = entry->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	// (1 << 3) = 8
	Ref<KnitNode> shl_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_shl", Vector2(100, 200));
	shl_node->add_input_pin("val", KnitPinKind::Data, sig_int, 1);
	shl_node->add_input_pin("shift", KnitPinKind::Data, sig_int, 3);
	KnitPinID shl_out = shl_node->add_output_pin("res", KnitPinKind::Data, sig_int);

	// (8 | 2) = 10
	Ref<KnitNode> or_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_or", Vector2(300, 200));
	KnitPinID or_a = or_node->add_input_pin("a", KnitPinKind::Data, sig_int);
	or_node->add_input_pin("b", KnitPinKind::Data, sig_int, 2);
	KnitPinID or_out = or_node->add_output_pin("res", KnitPinKind::Data, sig_int);

	graph->connect_pins(shl_node->id, shl_out, or_node->id, or_a);

	// (10 ^ 1) = 11
	Ref<KnitNode> xor_node = graph->create_node(KnitNodeCategory::PureFunction, "bit_xor", Vector2(500, 200));
	KnitPinID xor_a = xor_node->add_input_pin("a", KnitPinKind::Data, sig_int);
	xor_node->add_input_pin("b", KnitPinKind::Data, sig_int, 1);
	KnitPinID xor_out = xor_node->add_output_pin("res", KnitPinKind::Data, sig_int);

	graph->connect_pins(or_node->id, or_out, xor_node->id, xor_a);

	Ref<KnitNode> ret_node = graph->create_node(KnitNodeCategory::ImpureAction, "return", Vector2(700, 100));
	KnitPinID ret_in_exec = ret_node->add_input_pin("FlowIn", KnitPinKind::Execution, sig_exec);
	KnitPinID ret_in_val = ret_node->add_input_pin("val", KnitPinKind::Data, sig_int);

	graph->connect_pins(entry->id, entry_out, ret_node->id, ret_in_exec);
	graph->connect_pins(xor_node->id, xor_out, ret_node->id, ret_in_val);

	KnitsCompiler compiler;
	KnitCompiledGraph compiled;
	String error;
	bool ok = compiler.compile(graph, compiled, error);
	CHECK(ok);

	KnitsBytecodeVM vm;
	Variant return_val;
	KnitVMStatus status = vm.execute(compiled, nullptr, nullptr, 0, return_val);

	CHECK(status == KnitVMStatus::Returned);
	CHECK(int64_t(return_val) == 11);
}

} // namespace TestKnits
