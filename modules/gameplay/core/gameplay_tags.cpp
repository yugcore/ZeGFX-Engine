/**************************************************************************/
/*  gameplay_tags.cpp                                                     */
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

#include "gameplay_tags.h"

// -----------------------------------------------------------------------------
// GameplayTagDictionary
// -----------------------------------------------------------------------------

GameplayTagDictionary *GameplayTagDictionary::singleton = nullptr;

bool GameplayTagDictionary::register_tag(const StringName &p_tag, const String &p_description) {
	if (p_tag == StringName()) {
		return false;
	}
	registered_tags[p_tag] = p_description;
	return true;
}

bool GameplayTagDictionary::has_tag(const StringName &p_tag) const {
	return registered_tags.has(p_tag);
}

String GameplayTagDictionary::get_tag_description(const StringName &p_tag) const {
	if (registered_tags.has(p_tag)) {
		return registered_tags[p_tag];
	}
	return String();
}

PackedStringArray GameplayTagDictionary::get_registered_tags() const {
	PackedStringArray arr;
	arr.resize(registered_tags.size());
	int i = 0;
	for (const KeyValue<StringName, String> &E : registered_tags) {
		arr.set(i++, String(E.key));
	}
	return arr;
}

StringName GameplayTagDictionary::get_parent_tag(const StringName &p_tag) {
	String s = String(p_tag);
	int last_dot = s.rfind(".");
	if (last_dot <= 0) {
		return StringName();
	}
	return StringName(s.substr(0, last_dot));
}

bool GameplayTagDictionary::is_tag_parent_of(const StringName &p_parent, const StringName &p_child) {
	String parent_str = String(p_parent);
	String child_str = String(p_child);
	if (parent_str.is_empty() || child_str.is_empty() || parent_str.length() >= child_str.length()) {
		return false;
	}
	if (child_str.begins_with(parent_str)) {
		if (child_str[parent_str.length()] == '.') {
			return true;
		}
	}
	return false;
}

bool GameplayTagDictionary::matches_tag(const StringName &p_tag_in_container, const StringName &p_query_tag) {
	if (p_tag_in_container == p_query_tag) {
		return true;
	}
	return is_tag_parent_of(p_query_tag, p_tag_in_container);
}

void GameplayTagDictionary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_tag", "tag", "description"), &GameplayTagDictionary::register_tag, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("has_tag", "tag"), &GameplayTagDictionary::has_tag);
	ClassDB::bind_method(D_METHOD("get_tag_description", "tag"), &GameplayTagDictionary::get_tag_description);
	ClassDB::bind_method(D_METHOD("get_registered_tags"), &GameplayTagDictionary::get_registered_tags);

	ClassDB::bind_static_method("GameplayTagDictionary", D_METHOD("get_parent_tag", "tag"), &GameplayTagDictionary::get_parent_tag);
	ClassDB::bind_static_method("GameplayTagDictionary", D_METHOD("is_tag_parent_of", "parent", "child"), &GameplayTagDictionary::is_tag_parent_of);
	ClassDB::bind_static_method("GameplayTagDictionary", D_METHOD("matches_tag", "tag_in_container", "query_tag"), &GameplayTagDictionary::matches_tag);
}

GameplayTagDictionary::GameplayTagDictionary() {
	singleton = this;
}

GameplayTagDictionary::~GameplayTagDictionary() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

// -----------------------------------------------------------------------------
// GameplayTagContainer
// -----------------------------------------------------------------------------

bool GameplayTagContainer::add_tag(const StringName &p_tag) {
	if (p_tag == StringName()) {
		return false;
	}
	if (!tags.has(p_tag)) {
		tags.insert(p_tag);
		emit_signal(SNAME("tag_added"), p_tag);
		emit_changed();
		return true;
	}
	return false;
}

bool GameplayTagContainer::remove_tag(const StringName &p_tag) {
	if (tags.erase(p_tag)) {
		emit_signal(SNAME("tag_removed"), p_tag);
		emit_changed();
		return true;
	}
	return false;
}

void GameplayTagContainer::clear() {
	if (!tags.is_empty()) {
		tags.clear();
		emit_changed();
	}
}

bool GameplayTagContainer::has_tag_exact(const StringName &p_tag) const {
	return tags.has(p_tag);
}

bool GameplayTagContainer::has_tag(const StringName &p_tag) const {
	if (tags.has(p_tag)) {
		return true;
	}
	for (const StringName &t : tags) {
		if (GameplayTagDictionary::matches_tag(t, p_tag)) {
			return true;
		}
	}
	return false;
}

bool GameplayTagContainer::has_any(const PackedStringArray &p_tags) const {
	for (int i = 0; i < p_tags.size(); i++) {
		if (has_tag(StringName(p_tags[i]))) {
			return true;
		}
	}
	return false;
}

bool GameplayTagContainer::has_all(const PackedStringArray &p_tags) const {
	for (int i = 0; i < p_tags.size(); i++) {
		if (!has_tag(StringName(p_tags[i]))) {
			return false;
		}
	}
	return true;
}

bool GameplayTagContainer::has_none(const PackedStringArray &p_tags) const {
	return !has_any(p_tags);
}

PackedStringArray GameplayTagContainer::get_tags() const {
	PackedStringArray arr;
	arr.resize(tags.size());
	int i = 0;
	for (const StringName &t : tags) {
		arr.set(i++, String(t));
	}
	return arr;
}

void GameplayTagContainer::set_tags(const PackedStringArray &p_tags) {
	tags.clear();
	for (int i = 0; i < p_tags.size(); i++) {
		if (!p_tags[i].is_empty()) {
			tags.insert(StringName(p_tags[i]));
		}
	}
	emit_changed();
}

Ref<GameplayTagContainer> GameplayTagContainer::duplicate_tags() const {
	Ref<GameplayTagContainer> copy;
	copy.instantiate();
	copy->tags = tags;
	return copy;
}

void GameplayTagContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_tag", "tag"), &GameplayTagContainer::add_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "tag"), &GameplayTagContainer::remove_tag);
	ClassDB::bind_method(D_METHOD("clear"), &GameplayTagContainer::clear);

	ClassDB::bind_method(D_METHOD("has_tag_exact", "tag"), &GameplayTagContainer::has_tag_exact);
	ClassDB::bind_method(D_METHOD("has_tag", "tag"), &GameplayTagContainer::has_tag);
	ClassDB::bind_method(D_METHOD("has_any", "tags"), &GameplayTagContainer::has_any);
	ClassDB::bind_method(D_METHOD("has_all", "tags"), &GameplayTagContainer::has_all);
	ClassDB::bind_method(D_METHOD("has_none", "tags"), &GameplayTagContainer::has_none);

	ClassDB::bind_method(D_METHOD("get_tags"), &GameplayTagContainer::get_tags);
	ClassDB::bind_method(D_METHOD("set_tags", "tags"), &GameplayTagContainer::set_tags);
	ClassDB::bind_method(D_METHOD("get_tag_count"), &GameplayTagContainer::get_tag_count);
	ClassDB::bind_method(D_METHOD("is_empty"), &GameplayTagContainer::is_empty);
	ClassDB::bind_method(D_METHOD("duplicate_tags"), &GameplayTagContainer::duplicate_tags);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags");

	ADD_SIGNAL(MethodInfo("tag_added", PropertyInfo(Variant::STRING_NAME, "tag")));
	ADD_SIGNAL(MethodInfo("tag_removed", PropertyInfo(Variant::STRING_NAME, "tag")));
}
