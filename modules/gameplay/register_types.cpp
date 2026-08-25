/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "core/event_bus.h"
#include "core/gameplay_tag_query.h"
#include "core/gameplay_tags.h"
#include "core/resource_validator.h"

#include "attributes/attribute_set.h"
#include "attributes/gameplay_effect.h"
#include "attributes/gameplay_effect_execution.h"

#include "collision/damage_resolver.h"
#include "collision/hitbox_3d.h"
#include "collision/hurtbox_3d.h"

#include "actions/action_timeline.h"
#include "actions/combat_action.h"
#include "actions/combo_graph.h"
#include "actions/input_buffer_component.h"
#include "actions/timeline_track_base.h"
#include "actions/timeline_tracks.h"

#include "defense/contextual_reaction_rule.h"
#include "defense/defense_component.h"

#include "paired/paired_interaction.h"
#include "paired/sync_point_component.h"

#include "ai/combat_brain_3d.h"
#include "ai/move_consideration.h"
#include "ai/phase_transition_node.h"

#include "feedback/camera_shake_emitter.h"
#include "feedback/camera_shake_profile.h"
#include "feedback/hitstop_manager.h"

#include "debug/combat_telemetry.h"
#include "debug/gameplay_debug_overlay_3d.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"

#ifdef TOOLS_ENABLED
#include "editor/action_timeline_editor_plugin.h"
#include "editor/editor_node.h"

static void _gameplay_editor_init() {
	EditorPlugins::add_by_type<ActionTimelineEditorPlugin>();
}
#endif // TOOLS_ENABLED

static EventBus *event_bus = nullptr;
static GameplayTagDictionary *gameplay_tag_dictionary = nullptr;
static ResourceValidator *resource_validator = nullptr;
static DamageResolver *damage_resolver = nullptr;
static ContextualReactionEngine *contextual_reaction_engine = nullptr;
static HitstopManager *hitstop_manager = nullptr;
static CombatTelemetry *combat_telemetry = nullptr;

void initialize_gameplay_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(GameplayTagDictionary);
		GDREGISTER_CLASS(GameplayTagContainer);
		GDREGISTER_CLASS(GameplayTagQuery);
		GDREGISTER_CLASS(EventBus);
		GDREGISTER_CLASS(ResourceValidator);

		GDREGISTER_CLASS(AttributeModifier);
		GDREGISTER_CLASS(AttributeSet);
		GDREGISTER_CLASS(GameplayEffectExecution);
		GDREGISTER_CLASS(StandardDamageExecution);
		GDREGISTER_CLASS(GameplayEffect);
		GDREGISTER_CLASS(ActiveGameplayEffect);

		GDREGISTER_CLASS(DamageResolver);
		GDREGISTER_CLASS(Hitbox3D);
		GDREGISTER_CLASS(Hurtbox3D);

		GDREGISTER_CLASS(TimelineTrackBase);
		GDREGISTER_CLASS(HitboxTimelineTrack);
		GDREGISTER_CLASS(CancelWindowTimelineTrack);
		GDREGISTER_CLASS(DefenseWindowTimelineTrack);
		GDREGISTER_CLASS(TagWindowTimelineTrack);
		GDREGISTER_CLASS(NotifyTimelineTrack);
		GDREGISTER_CLASS(ScriptCallbackTimelineTrack);
		GDREGISTER_CLASS(ActionTimeline);
		GDREGISTER_CLASS(InputBufferComponent);
		GDREGISTER_CLASS(CombatAction);

		GDREGISTER_CLASS(ComboEdge);
		GDREGISTER_CLASS(ComboNode);
		GDREGISTER_CLASS(ComboGraph);
		GDREGISTER_CLASS(DefenseComponent);
		GDREGISTER_CLASS(ContextualReactionRule);
		GDREGISTER_CLASS(ContextualReactionEngine);

		GDREGISTER_CLASS(PairedInteraction);
		GDREGISTER_CLASS(SyncPointComponent);

		GDREGISTER_CLASS(MoveConsideration);
		GDREGISTER_CLASS(CombatBrain3D);
		GDREGISTER_CLASS(PhaseTransitionRule);
		GDREGISTER_CLASS(PhaseTransitionController);

		GDREGISTER_CLASS(HitstopManager);
		GDREGISTER_CLASS(CameraShakeProfile);
		GDREGISTER_CLASS(CameraShakeEmitter);

		GDREGISTER_CLASS(GameplayDebugOverlay3D);
		GDREGISTER_CLASS(CombatTelemetry);

		gameplay_tag_dictionary = memnew(GameplayTagDictionary);
		Engine::get_singleton()->add_singleton(Engine::Singleton("GameplayTagDictionary", gameplay_tag_dictionary));

		event_bus = memnew(EventBus);
		Engine::get_singleton()->add_singleton(Engine::Singleton("EventBus", event_bus));

		resource_validator = memnew(ResourceValidator);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ResourceValidator", resource_validator));

		damage_resolver = memnew(DamageResolver);
		Engine::get_singleton()->add_singleton(Engine::Singleton("DamageResolver", damage_resolver));

		contextual_reaction_engine = memnew(ContextualReactionEngine);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ContextualReactionEngine", contextual_reaction_engine));

		hitstop_manager = memnew(HitstopManager);
		Engine::get_singleton()->add_singleton(Engine::Singleton("HitstopManager", hitstop_manager));

		combat_telemetry = memnew(CombatTelemetry);
		Engine::get_singleton()->add_singleton(Engine::Singleton("CombatTelemetry", combat_telemetry));
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorNode::add_init_callback(_gameplay_editor_init);
	}
#endif // TOOLS_ENABLED
}

void uninitialize_gameplay_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		if (combat_telemetry) {
			Engine::get_singleton()->remove_singleton("CombatTelemetry");
			memdelete(combat_telemetry);
			combat_telemetry = nullptr;
		}
		if (hitstop_manager) {
			Engine::get_singleton()->remove_singleton("HitstopManager");
			memdelete(hitstop_manager);
			hitstop_manager = nullptr;
		}
		if (contextual_reaction_engine) {
			Engine::get_singleton()->remove_singleton("ContextualReactionEngine");
			memdelete(contextual_reaction_engine);
			contextual_reaction_engine = nullptr;
		}
		if (damage_resolver) {
			Engine::get_singleton()->remove_singleton("DamageResolver");
			memdelete(damage_resolver);
			damage_resolver = nullptr;
		}
		if (resource_validator) {
			Engine::get_singleton()->remove_singleton("ResourceValidator");
			memdelete(resource_validator);
			resource_validator = nullptr;
		}

		if (event_bus) {
			Engine::get_singleton()->remove_singleton("EventBus");
			memdelete(event_bus);
			event_bus = nullptr;
		}

		if (gameplay_tag_dictionary) {
			Engine::get_singleton()->remove_singleton("GameplayTagDictionary");
			memdelete(gameplay_tag_dictionary);
			gameplay_tag_dictionary = nullptr;
		}
	}
}
