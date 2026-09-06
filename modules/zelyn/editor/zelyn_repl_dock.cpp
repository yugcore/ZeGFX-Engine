/**************************************************************************/
/*  zelyn_repl_dock.cpp                                                   */
/**************************************************************************/

#include "zelyn_repl_dock.h"
#include "bridge/zelyn_variant_bridge.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "zelyn/tokenizer.h"

void ZelynREPLDock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_execute_entered", "text"), &ZelynREPLDock::_on_execute_entered);
	ClassDB::bind_method(D_METHOD("_on_eval_pressed"), &ZelynREPLDock::_on_eval_pressed);
	ClassDB::bind_method(D_METHOD("_on_clear_pressed"), &ZelynREPLDock::_on_clear_pressed);
}

void ZelynREPLDock::_on_execute_entered(const String &p_text) {
	if (p_text.strip_edges().is_empty()) return;
	history.push_back(p_text);
	history_index = history.size();
	command_input->clear();
	evaluate_expression(p_text);
}

void ZelynREPLDock::_on_eval_pressed() {
	if (command_input) {
		_on_execute_entered(command_input->get_text());
	}
}

void ZelynREPLDock::_on_clear_pressed() {
	if (output_display) {
		output_display->clear();
		output_display->append_text("[color=gray][b]Zelyn Interactive REPL Console[/b] - Ready[/color]\n");
	}
}

void ZelynREPLDock::evaluate_expression(const String &p_expr) {
	if (!output_display) return;

	output_display->append_text("[color=cyan]> " + p_expr + "[/color]\n");

	CharString utf8 = p_expr.utf8();
	std::string code_str(utf8.get_data(), utf8.length());

	try {
		std::vector<Token> tokens = tokenize(code_str);
		Compiler comp;
		comp.isREPL = true;
		Chunk chunk = comp.compile(tokens);
		Value res = repl_vm.run(chunk);

		Variant var_res = ZelynVariantBridge::zelyn_to_variant(res);
		output_display->append_text("[color=green]  => " + String(var_res.stringify()) + "[/color]\n");
	} catch (const std::exception &e) {
		output_display->append_text("[color=red]  Error: " + String(e.what()) + "[/color]\n");
	} catch (...) {
		output_display->append_text("[color=red]  Error: Execution failed.[/color]\n");
	}
}

ZelynREPLDock::ZelynREPLDock() {
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	output_display = memnew(RichTextLabel);
	output_display->set_use_bbcode(true);
	output_display->set_scroll_follow(true);
	output_display->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	output_display->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(output_display);

	output_display->append_text("[color=gray][b]Zelyn Interactive REPL Console[/b] - Ready[/color]\n");

	HBoxContainer *input_bar = memnew(HBoxContainer);
	input_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(input_bar);

	command_input = memnew(LineEdit);
	command_input->set_placeholder("Type Zelyn expression (e.g. out(1 + 2 * 3); or self.speed = 300;)...");
	command_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	command_input->connect("text_submitted", callable_mp(this, &ZelynREPLDock::_on_execute_entered));
	input_bar->add_child(command_input);

	eval_button = memnew(Button("Run"));
	eval_button->connect("pressed", callable_mp(this, &ZelynREPLDock::_on_eval_pressed));
	input_bar->add_child(eval_button);

	clear_button = memnew(Button("Clear"));
	clear_button->connect("pressed", callable_mp(this, &ZelynREPLDock::_on_clear_pressed));
	input_bar->add_child(clear_button);
}

ZelynREPLDock::~ZelynREPLDock() {}
