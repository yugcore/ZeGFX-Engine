/**************************************************************************/
/*  input_buffer_component.cpp                                            */
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

#include "input_buffer_component.h"
#include "core/os/time.h"

void InputBufferComponent::_prune_expired() {
	uint64_t now = Time::get_singleton()->get_ticks_usec();
	uint64_t max_age = uint64_t(buffer_window * 1000000.0);

	for (int i = buffer.size() - 1; i >= 0; i--) {
		if (now - buffer[i].timestamp_usec > max_age) {
			buffer.remove_at(i);
		}
	}
}

void InputBufferComponent::record_input(const StringName &p_action, int p_priority, bool p_held, bool p_double_tap, bool p_negative_edge) {
	if (p_action == StringName()) {
		return;
	}
	_prune_expired();

	BufferedInput input;
	input.action_name = p_action;
	input.timestamp_usec = Time::get_singleton()->get_ticks_usec();
	input.priority = p_priority;
	input.is_held = p_held;
	input.is_double_tap = p_double_tap;
	input.is_negative_edge = p_negative_edge;

	buffer.push_back(input);
}

bool InputBufferComponent::has_buffered_input(const StringName &p_action) {
	_prune_expired();
	for (int i = 0; i < buffer.size(); i++) {
		if (buffer[i].action_name == p_action) {
			return true;
		}
	}
	return false;
}

bool InputBufferComponent::consume_input(const StringName &p_action) {
	_prune_expired();
	for (int i = 0; i < buffer.size(); i++) {
		if (buffer[i].action_name == p_action) {
			buffer.remove_at(i);
			return true;
		}
	}
	return false;
}

StringName InputBufferComponent::pop_highest_priority_input() {
	_prune_expired();
	if (buffer.is_empty()) {
		return StringName();
	}

	int best_idx = 0;
	int highest_prio = buffer[0].priority;

	for (int i = 1; i < buffer.size(); i++) {
		if (buffer[i].priority > highest_prio) {
			highest_prio = buffer[i].priority;
			best_idx = i;
		}
	}

	StringName result = buffer[best_idx].action_name;
	buffer.remove_at(best_idx);
	return result;
}

void InputBufferComponent::clear_buffer() {
	buffer.clear();
}

void InputBufferComponent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS:
		case NOTIFICATION_PHYSICS_PROCESS: {
			_prune_expired();
		} break;
	}
}

void InputBufferComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_buffer_window", "window"), &InputBufferComponent::set_buffer_window);
	ClassDB::bind_method(D_METHOD("get_buffer_window"), &InputBufferComponent::get_buffer_window);

	ClassDB::bind_method(D_METHOD("record_input", "action", "priority", "held", "double_tap", "negative_edge"), &InputBufferComponent::record_input, DEFVAL(0), DEFVAL(false), DEFVAL(false), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("has_buffered_input", "action"), &InputBufferComponent::has_buffered_input);
	ClassDB::bind_method(D_METHOD("consume_input", "action"), &InputBufferComponent::consume_input);
	ClassDB::bind_method(D_METHOD("pop_highest_priority_input"), &InputBufferComponent::pop_highest_priority_input);
	ClassDB::bind_method(D_METHOD("clear_buffer"), &InputBufferComponent::clear_buffer);
	ClassDB::bind_method(D_METHOD("get_buffered_input_count"), &InputBufferComponent::get_buffered_input_count);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "buffer_window"), "set_buffer_window", "get_buffer_window");
}

InputBufferComponent::InputBufferComponent() {
	set_process(true);
}

InputBufferComponent::~InputBufferComponent() {}
