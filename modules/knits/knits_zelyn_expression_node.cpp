/**************************************************************************/
/*  knits_zelyn_expression_node.cpp                                       */
/**************************************************************************/

#include "knits_zelyn_expression_node.h"
#include "zelyn/tokenizer.h"

void KnitNodeZelynExpression::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_expression", "code"), &KnitNodeZelynExpression::set_expression);
	ClassDB::bind_method(D_METHOD("get_expression"), &KnitNodeZelynExpression::get_expression);
	ClassDB::bind_method(D_METHOD("evaluate", "args"), &KnitNodeZelynExpression::evaluate);
	ClassDB::bind_method(D_METHOD("configure_pins", "inputs", "output_name"), &KnitNodeZelynExpression::configure_pins);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "expression", PROPERTY_HINT_MULTILINE_TEXT), "set_expression", "get_expression");
}

void KnitNodeZelynExpression::set_expression(const String &p_code) {
	expression_code = p_code;
	String err;
	compile_expression(err);
}

bool KnitNodeZelynExpression::compile_expression(String &r_error) {
	is_compiled = false;
	if (expression_code.strip_edges().is_empty()) return false;

	CharString utf8 = expression_code.utf8();
	std::string code_str(utf8.get_data(), utf8.length());

	try {
		std::vector<Token> tokens = tokenize(code_str);
		Compiler comp;
		comp.isREPL = true;
		compiled_chunk = comp.compile(tokens);
		is_compiled = true;
		return true;
	} catch (const std::exception &e) {
		r_error = e.what();
		return false;
	} catch (...) {
		r_error = "Expression compilation failed";
		return false;
	}
}

Variant KnitNodeZelynExpression::evaluate(const Array &p_args) {
	if (!is_compiled) {
		String err;
		if (!compile_expression(err)) {
			return Variant();
		}
	}

	try {
		Value res = expr_vm.run(compiled_chunk);
		return ZelynVariantBridge::zelyn_to_variant(res);
	} catch (...) {
		return Variant();
	}
}

void KnitNodeZelynExpression::configure_pins(const PackedStringArray &p_inputs, const String &p_output_name) {
	input_pins.clear();
	output_pins.clear();

	for (int i = 0; i < p_inputs.size(); i++) {
		KnitTypeSignature sig;
		sig.kind = KnitDataType::Wildcard;
		add_input_pin(StringName(p_inputs[i]), KnitPinKind::Data, sig);
	}

	KnitTypeSignature sig_out;
	sig_out.kind = KnitDataType::Wildcard;
	add_output_pin(StringName(p_output_name.is_empty() ? "Result" : p_output_name), KnitPinKind::Data, sig_out);
}

KnitNodeZelynExpression::KnitNodeZelynExpression() {
	title = "Zelyn Expression";
	category = KnitNodeCategory::PureFunction;
}

KnitNodeZelynExpression::~KnitNodeZelynExpression() {}
