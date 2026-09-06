/**************************************************************************/
/*  zelyn_bytecode_viewer.h                                               */
/**************************************************************************/

#pragma once

#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/tree.h"
#include "scene/gui/button.h"
#include "scene/gui/progress_bar.h"
#include "runtime/zelyn_script.h"

class ZelynBytecodeViewer : public VBoxContainer {
	GDCLASS(ZelynBytecodeViewer, VBoxContainer);

private:
	Ref<ZelynScript> current_script;
	Label *title_label = nullptr;
	Label *reg_info_label = nullptr;
	ProgressBar *reg_pressure_bar = nullptr;
	Tree *disassembly_tree = nullptr;
	Button *refresh_button = nullptr;

	void _on_refresh_pressed();
	static String _get_opcode_name(uint8_t p_opcode);

protected:
	static void _bind_methods();

public:
	void set_script(const Ref<ZelynScript> &p_script);
	void disassemble();

	ZelynBytecodeViewer();
	~ZelynBytecodeViewer();
};
