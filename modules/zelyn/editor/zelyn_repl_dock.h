/**************************************************************************/
/*  zelyn_repl_dock.h                                                     */
/**************************************************************************/

#pragma once

#include "scene/gui/box_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"
#include "zelyn/bytecode_vm.h"
#include "zelyn/compiler.h"

class ZelynREPLDock : public VBoxContainer {
	GDCLASS(ZelynREPLDock, VBoxContainer);

private:
	RichTextLabel *output_display = nullptr;
	LineEdit *command_input = nullptr;
	Button *eval_button = nullptr;
	Button *clear_button = nullptr;

	Vector<String> history;
	int history_index = -1;

	BytecodeVM repl_vm;
	Compiler repl_compiler;

	void _on_execute_entered(const String &p_text);
	void _on_eval_pressed();
	void _on_clear_pressed();

protected:
	static void _bind_methods();

public:
	void evaluate_expression(const String &p_expr);

	ZelynREPLDock();
	~ZelynREPLDock();
};
