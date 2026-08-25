/**************************************************************************/
/*  hurtbox_3d.h                                                          */
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

#include "../attributes/attribute_set.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "scene/3d/physics/area_3d.h"

class Hitbox3D;

class Hurtbox3D : public Area3D {
	GDCLASS(Hurtbox3D, Area3D);

private:
	uint32_t team_id = 1;
	Ref<AttributeSet> attribute_set;
	Ref<GameplayTagContainer> state_tags;
	Ref<GameplayTagQuery> invulnerability_query;

protected:
	static void _bind_methods();

public:
	void set_team_id(uint32_t p_id) { team_id = p_id; }
	uint32_t get_team_id() const { return team_id; }

	void set_attribute_set(const Ref<AttributeSet> &p_set) { attribute_set = p_set; }
	Ref<AttributeSet> get_attribute_set() const { return attribute_set; }

	void set_state_tags(const Ref<GameplayTagContainer> &p_tags) { state_tags = p_tags; }
	Ref<GameplayTagContainer> get_state_tags() const { return state_tags; }

	void set_invulnerability_query(const Ref<GameplayTagQuery> &p_query) { invulnerability_query = p_query; }
	Ref<GameplayTagQuery> get_invulnerability_query() const { return invulnerability_query; }

	bool can_receive_hit(Hitbox3D *p_hitbox) const;
	Dictionary receive_hit(Hitbox3D *p_hitbox, const Vector3 &p_hit_point, const Vector3 &p_hit_normal);

	Hurtbox3D();
	~Hurtbox3D();
};
