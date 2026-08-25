/**************************************************************************/
/*  sync_point_component.h                                                */
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

#include "paired_interaction.h"
#include "scene/3d/node_3d.h"
#include "scene/main/node.h"

class SyncPointComponent : public Node {
	GDCLASS(SyncPointComponent, Node);

public:
	enum SyncState {
		SYNC_IDLE,
		SYNC_ALIGNING,
		SYNC_EXECUTING,
	};

private:
	SyncState current_state = SYNC_IDLE;
	Node3D *attacker_node = nullptr;
	Node3D *victim_node = nullptr;
	Ref<PairedInteraction> active_interaction;

	real_t current_time = 0.0;
	Transform3D victim_initial_transform;

	void _apply_tags(Node *p_node, const Ref<GameplayTagContainer> &p_tags, bool p_add);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	bool initiate_interaction(Node3D *p_attacker, Node3D *p_victim, const Ref<PairedInteraction> &p_interaction);
	void cancel_interaction();
	void finish_interaction();

	SyncState get_current_state() const { return current_state; }
	Node3D *get_attacker_node() const { return attacker_node; }
	Node3D *get_victim_node() const { return victim_node; }
	Ref<PairedInteraction> get_active_interaction() const { return active_interaction; }

	SyncPointComponent();
	~SyncPointComponent();
};

VARIANT_ENUM_CAST(SyncPointComponent::SyncState);
