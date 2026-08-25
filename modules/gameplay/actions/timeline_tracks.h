/**************************************************************************/
/*  timeline_tracks.h                                                     */
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

#include "timeline_track_base.h"
#include "../core/gameplay_tags.h"
#include "../core/gameplay_tag_query.h"

// -----------------------------------------------------------------------------
// HitboxTimelineTrack
// -----------------------------------------------------------------------------
class HitboxTimelineTrack : public TimelineTrackBase {
	GDCLASS(HitboxTimelineTrack, TimelineTrackBase);

private:
	StringName hitbox_node_name = "Hitbox3D";
	Dictionary damage_channels;

protected:
	static void _bind_methods();

public:
	void set_hitbox_node_name(const StringName &p_name) { hitbox_node_name = p_name; emit_changed(); }
	StringName get_hitbox_node_name() const { return hitbox_node_name; }

	void set_damage_channels(const Dictionary &p_channels) { damage_channels = p_channels; emit_changed(); }
	Dictionary get_damage_channels() const { return damage_channels; }

	virtual void on_enter(Node *p_actor) override;
	virtual void on_exit(Node *p_actor) override;

	HitboxTimelineTrack() {}
	~HitboxTimelineTrack() {}
};

// -----------------------------------------------------------------------------
// CancelWindowTimelineTrack
// -----------------------------------------------------------------------------
class CancelWindowTimelineTrack : public TimelineTrackBase {
	GDCLASS(CancelWindowTimelineTrack, TimelineTrackBase);

private:
	PackedStringArray allowed_cancel_actions;
	Ref<GameplayTagQuery> cancel_requirements;

protected:
	static void _bind_methods();

public:
	void set_allowed_cancel_actions(const PackedStringArray &p_actions) { allowed_cancel_actions = p_actions; emit_changed(); }
	PackedStringArray get_allowed_cancel_actions() const { return allowed_cancel_actions; }

	void set_cancel_requirements(const Ref<GameplayTagQuery> &p_query) { cancel_requirements = p_query; emit_changed(); }
	Ref<GameplayTagQuery> get_cancel_requirements() const { return cancel_requirements; }

	bool is_cancel_allowed(const StringName &p_action_name) const;

	CancelWindowTimelineTrack() {}
	~CancelWindowTimelineTrack() {}
};

// -----------------------------------------------------------------------------
// DefenseWindowTimelineTrack
// -----------------------------------------------------------------------------
class DefenseWindowTimelineTrack : public TimelineTrackBase {
	GDCLASS(DefenseWindowTimelineTrack, TimelineTrackBase);

public:
	enum DefenseType {
		DEFENSE_PARRY,
		DEFENSE_BLOCK,
		DEFENSE_IFRAME,
	};

private:
	DefenseType defense_type = DEFENSE_PARRY;

protected:
	static void _bind_methods();

public:
	void set_defense_type(DefenseType p_type) { defense_type = p_type; emit_changed(); }
	DefenseType get_defense_type() const { return defense_type; }

	virtual void on_enter(Node *p_actor) override;
	virtual void on_exit(Node *p_actor) override;

	DefenseWindowTimelineTrack() {}
	~DefenseWindowTimelineTrack() {}
};

VARIANT_ENUM_CAST(DefenseWindowTimelineTrack::DefenseType);

// -----------------------------------------------------------------------------
// TagWindowTimelineTrack
// -----------------------------------------------------------------------------
class TagWindowTimelineTrack : public TimelineTrackBase {
	GDCLASS(TagWindowTimelineTrack, TimelineTrackBase);

private:
	PackedStringArray granted_tags;

protected:
	static void _bind_methods();

public:
	void set_granted_tags(const PackedStringArray &p_tags) { granted_tags = p_tags; emit_changed(); }
	PackedStringArray get_granted_tags() const { return granted_tags; }

	virtual void on_enter(Node *p_actor) override;
	virtual void on_exit(Node *p_actor) override;

	TagWindowTimelineTrack() {}
	~TagWindowTimelineTrack() {}
};

// -----------------------------------------------------------------------------
// NotifyTimelineTrack
// -----------------------------------------------------------------------------
class NotifyTimelineTrack : public TimelineTrackBase {
	GDCLASS(NotifyTimelineTrack, TimelineTrackBase);

private:
	StringName signal_name;
	Dictionary parameters;

protected:
	static void _bind_methods();

public:
	void set_signal_name(const StringName &p_name) { signal_name = p_name; emit_changed(); }
	StringName get_signal_name() const { return signal_name; }

	void set_parameters(const Dictionary &p_params) { parameters = p_params; emit_changed(); }
	Dictionary get_parameters() const { return parameters; }

	virtual void on_enter(Node *p_actor) override;

	NotifyTimelineTrack() {}
	~NotifyTimelineTrack() {}
};

// -----------------------------------------------------------------------------
// ScriptCallbackTimelineTrack
// -----------------------------------------------------------------------------
class ScriptCallbackTimelineTrack : public TimelineTrackBase {
	GDCLASS(ScriptCallbackTimelineTrack, TimelineTrackBase);

private:
	Callable callback;

protected:
	static void _bind_methods();

public:
	void set_callback(const Callable &p_cb) { callback = p_cb; emit_changed(); }
	Callable get_callback() const { return callback; }

	virtual void on_enter(Node *p_actor) override;

	ScriptCallbackTimelineTrack() {}
	~ScriptCallbackTimelineTrack() {}
};
