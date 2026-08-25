/**************************************************************************/
/*  input_buffer_component.h                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

struct BufferedInput {
	StringName action_name;
	uint64_t timestamp_usec = 0;
	int priority = 0;
	bool is_held = false;
	bool is_double_tap = false;
	bool is_negative_edge = false;
};

class InputBufferComponent : public Node {
	GDCLASS(InputBufferComponent, Node);

private:
	real_t buffer_window = 0.25; // seconds
	Vector<BufferedInput> buffer;

	void _prune_expired();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_buffer_window(real_t p_window) { buffer_window = MAX(0.01, p_window); }
	real_t get_buffer_window() const { return buffer_window; }

	void record_input(const StringName &p_action, int p_priority = 0, bool p_held = false, bool p_double_tap = false, bool p_negative_edge = false);
	bool has_buffered_input(const StringName &p_action);
	bool consume_input(const StringName &p_action);
	StringName pop_highest_priority_input();
	void clear_buffer();

	int get_buffered_input_count() const { return buffer.size(); }

	InputBufferComponent();
	~InputBufferComponent();
};
