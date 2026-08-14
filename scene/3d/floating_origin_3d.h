/**************************************************************************/
/*  floating_origin_3d.h                                                  */
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

#include "scene/3d/node_3d.h"

class FloatingOrigin3D : public Node3D {
	GDCLASS(FloatingOrigin3D, Node3D);

private:
	float threshold = 2000.0f;
	bool auto_rebase_enabled = true;
	bool rebase_physics_bodies = true;
	bool rebase_in_editor = false;
	NodePath target_node_path;

	Vector3 total_world_offset;
	int shift_count = 0;

	void _perform_rebase(const Vector3 &p_shift_delta);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	FloatingOrigin3D();
	~FloatingOrigin3D();

	void set_threshold(float p_dist);
	float get_threshold() const;

	void set_auto_rebase_enabled(bool p_enabled);
	bool is_auto_rebase_enabled() const;

	void set_rebase_physics_bodies(bool p_enabled);
	bool is_rebase_physics_bodies() const;

	void set_rebase_in_editor(bool p_enabled);
	bool is_rebase_in_editor() const;

	void set_target_node_path(const NodePath &p_path);
	NodePath get_target_node_path() const;

	Vector3 get_total_world_offset() const;
	int get_shift_count() const;

	Vector3 local_to_universe(const Vector3 &p_local_pos) const;
	Vector3 universe_to_local(const Vector3 &p_universe_pos) const;

	void shift_world_origin(const Vector3 &p_delta);
	void rebase_to_position(const Vector3 &p_target_pos);
	void reset_universe_origin();
};
