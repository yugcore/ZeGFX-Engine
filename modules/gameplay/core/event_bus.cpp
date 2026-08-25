/**************************************************************************/
/*  event_bus.cpp                                                         */
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

#include "event_bus.h"
#include "gameplay_tags.h"

EventBus *EventBus::singleton = nullptr;

void EventBus::publish(const StringName &p_event, const Array &p_args) {
	emit_signal(SNAME("event_published"), p_event, p_args);

	if (event_listeners.has(p_event)) {
		const Vector<Callable> &listeners = event_listeners[p_event];
		const Variant **args = (const Variant **)alloca(sizeof(Variant *) * p_args.size());
		for (int i = 0; i < p_args.size(); i++) {
			args[i] = &p_args[i];
		}
		for (int i = 0; i < listeners.size(); i++) {
			Callable c = listeners[i];
			if (c.is_valid()) {
				Callable::CallError ce;
				Variant ret;
				c.callp(args, p_args.size(), ret, ce);
			}
		}
	}
}

void EventBus::publish_tagged(const StringName &p_tag, const Array &p_args) {
	emit_signal(SNAME("tagged_event_published"), p_tag, p_args);

	const Variant **args = (const Variant **)alloca(sizeof(Variant *) * p_args.size());
	for (int i = 0; i < p_args.size(); i++) {
		args[i] = &p_args[i];
	}

	for (const KeyValue<StringName, Vector<Callable>> &E : tagged_listeners) {
		if (GameplayTagDictionary::matches_tag(p_tag, E.key)) {
			const Vector<Callable> &listeners = E.value;
			for (int i = 0; i < listeners.size(); i++) {
				Callable c = listeners[i];
				if (c.is_valid()) {
					Callable::CallError ce;
					Variant ret;
					c.callp(args, p_args.size(), ret, ce);
				}
			}
		}
	}
}

void EventBus::subscribe(const StringName &p_event, const Callable &p_callable) {
	if (!p_callable.is_valid()) {
		return;
	}
	Vector<Callable> &listeners = event_listeners[p_event];
	if (!listeners.has(p_callable)) {
		listeners.push_back(p_callable);
	}
}

void EventBus::unsubscribe(const StringName &p_event, const Callable &p_callable) {
	if (event_listeners.has(p_event)) {
		event_listeners[p_event].erase(p_callable);
		if (event_listeners[p_event].is_empty()) {
			event_listeners.erase(p_event);
		}
	}
}

void EventBus::subscribe_tagged(const StringName &p_tag, const Callable &p_callable) {
	if (!p_callable.is_valid()) {
		return;
	}
	Vector<Callable> &listeners = tagged_listeners[p_tag];
	if (!listeners.has(p_callable)) {
		listeners.push_back(p_callable);
	}
}

void EventBus::unsubscribe_tagged(const StringName &p_tag, const Callable &p_callable) {
	if (tagged_listeners.has(p_tag)) {
		tagged_listeners[p_tag].erase(p_callable);
		if (tagged_listeners[p_tag].is_empty()) {
			tagged_listeners.erase(p_tag);
		}
	}
}

void EventBus::clear_listeners() {
	event_listeners.clear();
	tagged_listeners.clear();
}

void EventBus::_bind_methods() {
	ClassDB::bind_method(D_METHOD("publish", "event", "args"), &EventBus::publish, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("publish_tagged", "tag", "args"), &EventBus::publish_tagged, DEFVAL(Array()));

	ClassDB::bind_method(D_METHOD("subscribe", "event", "callable"), &EventBus::subscribe);
	ClassDB::bind_method(D_METHOD("unsubscribe", "event", "callable"), &EventBus::unsubscribe);

	ClassDB::bind_method(D_METHOD("subscribe_tagged", "tag", "callable"), &EventBus::subscribe_tagged);
	ClassDB::bind_method(D_METHOD("unsubscribe_tagged", "tag", "callable"), &EventBus::unsubscribe_tagged);

	ClassDB::bind_method(D_METHOD("clear_listeners"), &EventBus::clear_listeners);

	ADD_SIGNAL(MethodInfo("event_published", PropertyInfo(Variant::STRING_NAME, "event"), PropertyInfo(Variant::ARRAY, "args")));
	ADD_SIGNAL(MethodInfo("tagged_event_published", PropertyInfo(Variant::STRING_NAME, "tag"), PropertyInfo(Variant::ARRAY, "args")));
}

EventBus::EventBus() {
	singleton = this;
}

EventBus::~EventBus() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
