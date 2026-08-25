/**************************************************************************/
/*  combat_action.h                                                       */
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

#include "action_timeline.h"
#include "../attributes/attribute_set.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "core/io/resource.h"

class CombatAction : public Resource {
	GDCLASS(CombatAction, Resource);

private:
	StringName action_name;
	StringName animation_state_name;
	Ref<ActionTimeline> timeline;
	Dictionary resource_costs;
	real_t cooldown = 0.0;
	StringName cooldown_tag_group;

	Ref<GameplayTagQuery> activation_required_tags;
	Ref<GameplayTagQuery> activation_blocked_tags;
	Ref<GameplayTagContainer> granted_tags_while_active;

protected:
	static void _bind_methods();

public:
	void set_action_name(const StringName &p_name) { action_name = p_name; emit_changed(); }
	StringName get_action_name() const { return action_name; }

	void set_animation_state_name(const StringName &p_name) { animation_state_name = p_name; emit_changed(); }
	StringName get_animation_state_name() const { return animation_state_name; }

	void set_timeline(const Ref<ActionTimeline> &p_timeline) { timeline = p_timeline; emit_changed(); }
	Ref<ActionTimeline> get_timeline() const { return timeline; }

	void set_resource_costs(const Dictionary &p_costs) { resource_costs = p_costs; emit_changed(); }
	Dictionary get_resource_costs() const { return resource_costs; }

	void set_cooldown(real_t p_cd) { cooldown = MAX(0.0, p_cd); emit_changed(); }
	real_t get_cooldown() const { return cooldown; }

	void set_cooldown_tag_group(const StringName &p_group) { cooldown_tag_group = p_group; emit_changed(); }
	StringName get_cooldown_tag_group() const { return cooldown_tag_group; }

	void set_activation_required_tags(const Ref<GameplayTagQuery> &p_query) { activation_required_tags = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_activation_required_tags() const { return activation_required_tags; }

	void set_activation_blocked_tags(const Ref<GameplayTagQuery> &p_query) { activation_blocked_tags = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_activation_blocked_tags() const { return activation_blocked_tags; }

	void set_granted_tags_while_active(const Ref<GameplayTagContainer> &p_tags) { granted_tags_while_active = p_tags; emit_changed(); }
	Ref<GameplayTagContainer> get_granted_tags_while_active() const { return granted_tags_while_active; }

	bool can_activate(const Ref<AttributeSet> &p_attrs, const Ref<GameplayTagContainer> &p_tags) const;
	void consume_costs(const Ref<AttributeSet> &p_attrs) const;

	CombatAction() {}
	~CombatAction() {}
};
