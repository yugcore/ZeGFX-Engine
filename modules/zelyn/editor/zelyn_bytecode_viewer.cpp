/**************************************************************************/
/*  zelyn_bytecode_viewer.cpp                                             */
/**************************************************************************/

#include "zelyn_bytecode_viewer.h"
#include "bridge/zelyn_variant_bridge.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"

void ZelynBytecodeViewer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_refresh_pressed"), &ZelynBytecodeViewer::_on_refresh_pressed);
}

String ZelynBytecodeViewer::_get_opcode_name(uint8_t p_opcode) {
	switch (p_opcode) {
		case 0: return "ZOP_LOADK";
		case 1: return "ZOP_LOADNIL";
		case 2: return "ZOP_LOADBOOL";
		case 3: return "ZOP_MOVE";
		case 4: return "ZOP_GET_LOCAL";
		case 5: return "ZOP_SET_LOCAL";
		case 6: return "ZOP_GET_GLOBAL";
		case 7: return "ZOP_SET_GLOBAL";
		case 8: return "ZOP_ADD";
		case 9: return "ZOP_SUBRACT";
		case 10: return "ZOP_SUB";
		case 11: return "ZOP_MUL";
		case 12: return "ZOP_DIV";
		case 13: return "ZOP_MOD";
		case 14: return "ZOP_UNM";
		case 15: return "ZOP_NOT";
		case 16: return "ZOP_ADDK";
		case 17: return "ZOP_MULK";
		case 18: return "ZOP_EQ";
		case 19: return "ZOP_LT";
		case 20: return "ZOP_LE";
		case 21: return "ZOP_JUMP";
		case 22: return "ZOP_JUMPBACK";
		case 23: return "ZOP_JUMPIF";
		case 24: return "ZOP_JUMPIFNOT";
		case 25: return "ZOP_CALL";
		case 26: return "ZOP_RETURN";
		case 27: return "ZOP_TAILCALL";
		case 42: return "ZOP_FASTNATIVE";
		default: return "ZOP_" + String::num_int64(p_opcode);
	}
}

void ZelynBytecodeViewer::_on_refresh_pressed() {
	disassemble();
}

void ZelynBytecodeViewer::set_script(const Ref<ZelynScript> &p_script) {
	current_script = p_script;
	disassemble();
}

void ZelynBytecodeViewer::disassemble() {
	if (!disassembly_tree) return;
	disassembly_tree->clear();

	if (current_script.is_null()) {
		if (title_label) title_label->set_text("No active Zelyn script");
		if (reg_info_label) reg_info_label->set_text("Registers: 0 / 4096");
		if (reg_pressure_bar) reg_pressure_bar->set_value(0);
		return;
	}

	const Chunk &chunk = current_script->get_compiled_chunk();

	if (title_label) {
		String p = current_script->get_path();
		title_label->set_text(p.is_empty() ? "Unsaved Zelyn Script" : p.get_file());
	}

	uint32_t max_regs = chunk.maxRegisters;
	if (reg_info_label) {
		reg_info_label->set_text(vformat("Max Registers Used: %d / 4096", max_regs));
	}
	if (reg_pressure_bar) {
		double ratio = (double)max_regs / 64.0; // scale against typical frame budget
		if (ratio > 1.0) ratio = 1.0;
		reg_pressure_bar->set_value(ratio * 100.0);
	}

	TreeItem *root = disassembly_tree->create_item();

	for (size_t i = 0; i < chunk.code.size(); i++) {
		uint32_t instr = chunk.code[i];
		uint8_t op = static_cast<uint8_t>(instr >> 24);
		uint8_t rA = static_cast<uint8_t>((instr >> 16) & 0xFF);
		uint8_t rB = static_cast<uint8_t>((instr >> 8) & 0xFF);
		uint8_t rC = static_cast<uint8_t>(instr & 0xFF);

		TreeItem *item = disassembly_tree->create_item(root);
		item->set_text(0, vformat("[%04d]", (int)i));
		item->set_text(1, _get_opcode_name(op));
		item->set_text(2, vformat("r%d, r%d, r%d", rA, rB, rC));

		// Constant lookup comment if applicable
		if (op == 0 /* LOADK */ && rB < chunk.constants.size()) {
			Variant v = ZelynVariantBridge::zelyn_to_variant(chunk.constants[rB]);
			item->set_text(3, vformat("; const #%d = %s", rB, String(v.stringify())));
		}
	}
}

ZelynBytecodeViewer::ZelynBytecodeViewer() {
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *header = memnew(HBoxContainer);
	header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(header);

	title_label = memnew(Label("Zelyn Bytecode Disassembler"));
	title_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title_label);

	refresh_button = memnew(Button("Refresh"));
	refresh_button->connect("pressed", callable_mp(this, &ZelynBytecodeViewer::_on_refresh_pressed));
	header->add_child(refresh_button);

	HBoxContainer *meta_bar = memnew(HBoxContainer);
	meta_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(meta_bar);

	reg_info_label = memnew(Label("Max Registers: 0 / 4096"));
	meta_bar->add_child(reg_info_label);

	reg_pressure_bar = memnew(ProgressBar);
	reg_pressure_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	reg_pressure_bar->set_custom_minimum_size(Size2(120, 16));
	meta_bar->add_child(reg_pressure_bar);

	disassembly_tree = memnew(Tree);
	disassembly_tree->set_columns(4);
	disassembly_tree->set_column_title(0, "Offset");
	disassembly_tree->set_column_title(1, "Instruction");
	disassembly_tree->set_column_title(2, "Registers");
	disassembly_tree->set_column_title(3, "Comments");
	disassembly_tree->set_column_titles_visible(true);
	disassembly_tree->set_column_expand(0, false);
	disassembly_tree->set_column_custom_minimum_width(0, 80);
	disassembly_tree->set_column_expand(1, false);
	disassembly_tree->set_column_custom_minimum_width(1, 160);
	disassembly_tree->set_column_expand(2, false);
	disassembly_tree->set_column_custom_minimum_width(2, 120);
	disassembly_tree->set_column_expand(3, true);
	disassembly_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	disassembly_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(disassembly_tree);
}

ZelynBytecodeViewer::~ZelynBytecodeViewer() {}
