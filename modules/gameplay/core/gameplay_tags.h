/**************************************************************************/
/*  gameplay_tags.h                                                       */
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

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/vector.h"

class GameplayTagDictionary : public Object {
	GDCLASS(GameplayTagDictionary, Object);

private:
	static GameplayTagDictionary *singleton;
	HashMap<StringName, String> registered_tags; // tag_name -> description

protected:
	static void _bind_methods();

public:
	static GameplayTagDictionary *get_singleton() { return singleton; }

	bool register_tag(const StringName &p_tag, const String &p_description = String());
	bool has_tag(const StringName &p_tag) const;
	String get_tag_description(const StringName &p_tag) const;
	PackedStringArray get_registered_tags() const;

	static StringName get_parent_tag(const StringName &p_tag);
	static bool is_tag_parent_of(const StringName &p_parent, const StringName &p_child);
	static bool matches_tag(const StringName &p_tag_in_container, const StringName &p_query_tag);

	GameplayTagDictionary();
	~GameplayTagDictionary();
};

class GameplayTagContainer : public Resource {
	GDCLASS(GameplayTagContainer, Resource);

private:
	HashSet<StringName> tags;

protected:
	static void _bind_methods();

public:
	bool add_tag(const StringName &p_tag);
	bool remove_tag(const StringName &p_tag);
	void clear();

	bool has_tag_exact(const StringName &p_tag) const;
	bool has_tag(const StringName &p_tag) const;
	bool has_any(const PackedStringArray &p_tags) const;
	bool has_all(const PackedStringArray &p_tags) const;
	bool has_none(const PackedStringArray &p_tags) const;

	PackedStringArray get_tags() const;
	void set_tags(const PackedStringArray &p_tags);

	int get_tag_count() const { return tags.size(); }
	bool is_empty() const { return tags.is_empty(); }

	Ref<GameplayTagContainer> duplicate_tags() const;

	GameplayTagContainer() {}
	~GameplayTagContainer() {}
};
