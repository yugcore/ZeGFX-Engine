/**************************************************************************/
/*  knits_zelyn_expression_node.h                                         */
/**************************************************************************/

#pragma once

#include "knits_node.h"
#include "modules/zelyn/bridge/zelyn_variant_bridge.h"
#include "zelyn/compiler.h"
#include "zelyn/bytecode_vm.h"

class KnitNodeZelynExpression : public KnitNode {
	GDCLASS(KnitNodeZelynExpression, KnitNode);

private:
	String expression_code;
	Chunk compiled_chunk;
	bool is_compiled = false;
	BytecodeVM expr_vm;

protected:
	static void _bind_methods();

public:
	void set_expression(const String &p_code);
	String get_expression() const { return expression_code; }

	bool compile_expression(String &r_error);
	Variant evaluate(const Array &p_args);

	void configure_pins(const PackedStringArray &p_inputs, const String &p_output_name);

	KnitNodeZelynExpression();
	~KnitNodeZelynExpression();
};
