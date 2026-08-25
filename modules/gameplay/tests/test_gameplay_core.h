/**************************************************************************/
/*  test_gameplay_core.h                                                  */
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

#include "../core/event_bus.h"
#include "../core/gameplay_tag_query.h"
#include "../core/gameplay_tags.h"
#include "../core/resource_validator.h"

#include "../attributes/attribute_set.h"
#include "../attributes/gameplay_effect.h"
#include "../attributes/gameplay_effect_execution.h"

#include "../collision/damage_resolver.h"
#include "../collision/hitbox_3d.h"
#include "../collision/hurtbox_3d.h"

#include "../actions/action_timeline.h"
#include "../actions/combat_action.h"
#include "../actions/combo_graph.h"
#include "../actions/input_buffer_component.h"
#include "../actions/timeline_track_base.h"
#include "../actions/timeline_tracks.h"

#include "../defense/contextual_reaction_rule.h"
#include "../defense/defense_component.h"

#include "../paired/paired_interaction.h"
#include "../paired/sync_point_component.h"

#include "../ai/combat_brain_3d.h"
#include "../ai/move_consideration.h"
#include "../ai/phase_transition_node.h"

#include "../feedback/camera_shake_emitter.h"
#include "../feedback/camera_shake_profile.h"
#include "../feedback/hitstop_manager.h"

#include "../debug/combat_telemetry.h"
#include "../debug/gameplay_debug_overlay_3d.h"

#include "core/object/callable_mp.h"
#include "tests/test_macros.h"

namespace TestGameplayCore {

TEST_CASE("[Gameplay] Tag Dictionary hierarchy and parent resolution") {
	GameplayTagDictionary dict;
	dict.register_tag("Combat.State.Guarding", "Character is in active guard pose");
	dict.register_tag("Combat.State.Parrying", "Character is in parry active frames");
	dict.register_tag("Damage.Physical.Slash", "Slashing physical damage");

	CHECK(dict.has_tag("Combat.State.Guarding"));
	CHECK(dict.get_tag_description("Combat.State.Guarding") == "Character is in active guard pose");

	CHECK(GameplayTagDictionary::get_parent_tag("Combat.State.Guarding") == StringName("Combat.State"));
	CHECK(GameplayTagDictionary::get_parent_tag("Combat.State") == StringName("Combat"));
	CHECK(GameplayTagDictionary::get_parent_tag("Combat") == StringName());

	CHECK(GameplayTagDictionary::is_tag_parent_of("Combat", "Combat.State.Guarding"));
	CHECK(GameplayTagDictionary::is_tag_parent_of("Combat.State", "Combat.State.Guarding"));
	CHECK_FALSE(GameplayTagDictionary::is_tag_parent_of("Combat.State.Guarding", "Combat"));
	CHECK_FALSE(GameplayTagDictionary::is_tag_parent_of("Damage", "Combat.State.Guarding"));
}

TEST_CASE("[Gameplay] Tag Container exact and hierarchical query") {
	Ref<GameplayTagContainer> container;
	container.instantiate();

	container->add_tag("Combat.State.Guarding");
	container->add_tag("Status.Buff.AttackUp");

	// Exact check
	CHECK(container->has_tag_exact("Combat.State.Guarding"));
	CHECK_FALSE(container->has_tag_exact("Combat.State"));
	CHECK_FALSE(container->has_tag_exact("Combat"));

	// Hierarchical check
	CHECK(container->has_tag("Combat.State.Guarding"));
	CHECK(container->has_tag("Combat.State"));
	CHECK(container->has_tag("Combat"));
	CHECK(container->has_tag("Status.Buff"));
	CHECK_FALSE(container->has_tag("Damage"));

	// Array checks
	PackedStringArray any_check;
	any_check.push_back("Damage.Fire");
	any_check.push_back("Combat");
	CHECK(container->has_any(any_check));

	PackedStringArray all_check;
	all_check.push_back("Combat.State");
	all_check.push_back("Status.Buff");
	CHECK(container->has_all(all_check));

	PackedStringArray none_check;
	none_check.push_back("Damage.Fire");
	none_check.push_back("Damage.Lightning");
	CHECK(container->has_none(none_check));

	// Remove tag
	container->remove_tag("Combat.State.Guarding");
	CHECK_FALSE(container->has_tag("Combat"));
}

TEST_CASE("[Gameplay] Tag Query expressions (ANY, ALL, NONE, EXPRESSION)") {
	Ref<GameplayTagContainer> container;
	container.instantiate();
	container->add_tag("Weapon.Melee.Sword");
	container->add_tag("Combat.Stance.Aggressive");

	PackedStringArray tags_match;
	tags_match.push_back("Weapon.Melee");
	tags_match.push_back("Magic");

	Ref<GameplayTagQuery> query_any = GameplayTagQuery::create_match_any(tags_match);
	CHECK(query_any->evaluate(container));

	Ref<GameplayTagQuery> query_all = GameplayTagQuery::create_match_all(tags_match);
	CHECK_FALSE(query_all->evaluate(container)); // Missing Magic

	PackedStringArray tags_magic;
	tags_magic.push_back("Magic.Fire");
	Ref<GameplayTagQuery> query_none = GameplayTagQuery::create_match_none(tags_magic);
	CHECK(query_none->evaluate(container));

	// Expression: (Weapon.Melee AND NOT Magic)
	Ref<GameplayTagQuery> q_weapon = GameplayTagQuery::create_match_any(PackedStringArray{ "Weapon.Melee" });
	Ref<GameplayTagQuery> q_magic = GameplayTagQuery::create_match_any(PackedStringArray{ "Magic" });

	Ref<GameplayTagQuery> q_not_magic;
	q_not_magic.instantiate();
	q_not_magic->set_query_type(GameplayTagQuery::QUERY_MATCH_EXPRESSION);
	q_not_magic->set_expression_op(GameplayTagQuery::OP_NOT);
	TypedArray<GameplayTagQuery> not_sub;
	not_sub.push_back(q_magic);
	q_not_magic->set_sub_queries(not_sub);

	Ref<GameplayTagQuery> q_composite;
	q_composite.instantiate();
	q_composite->set_query_type(GameplayTagQuery::QUERY_MATCH_EXPRESSION);
	q_composite->set_expression_op(GameplayTagQuery::OP_AND);
	TypedArray<GameplayTagQuery> comp_sub;
	comp_sub.push_back(q_weapon);
	comp_sub.push_back(q_not_magic);
	q_composite->set_sub_queries(comp_sub);

	CHECK(q_composite->evaluate(container));
}

struct TestEventListener : public RefCounted {
	GDCLASS(TestEventListener, RefCounted);

public:
	int event_count = 0;
	int tagged_count = 0;

	void on_event() {
		event_count++;
	}

	void on_tagged() {
		tagged_count++;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("on_event"), &TestEventListener::on_event);
		ClassDB::bind_method(D_METHOD("on_tagged"), &TestEventListener::on_tagged);
	}
};

TEST_CASE("[Gameplay] EventBus publication and tagged routing") {
	EventBus bus;
	Ref<TestEventListener> listener;
	listener.instantiate();

	Callable c_event = callable_mp(listener.ptr(), &TestEventListener::on_event);
	Callable c_tagged = callable_mp(listener.ptr(), &TestEventListener::on_tagged);

	bus.subscribe("combat_hit", c_event);
	bus.subscribe_tagged("Combat.Hit", c_tagged);

	bus.publish("combat_hit");
	CHECK(listener->event_count == 1);

	bus.publish_tagged("Combat.Hit.Critical");
	CHECK(listener->tagged_count == 1);

	bus.unsubscribe("combat_hit", c_event);
	bus.publish("combat_hit");
	CHECK(listener->event_count == 1); // Not incremented
}

TEST_CASE("[Gameplay] AttributeSet modifier aggregator evaluation order") {
	Ref<AttributeSet> attrs;
	attrs.instantiate();
	attrs->add_attribute("Health", 100.0, 0.0, 100.0);

	// Formula: (Base + AdditiveBonus) * (1 + Multiplicative)
	// Base: 100
	// Additive Base (+20) -> Base becomes 120
	Ref<AttributeModifier> mod_base;
	mod_base.instantiate();
	mod_base->set_target_attribute("Health");
	mod_base->set_op(AttributeModifier::OP_ADDITIVE_BASE);
	mod_base->set_value(20.0);
	attrs->add_modifier(mod_base);
	CHECK(attrs->get_attribute_max("Health") == 120.0);

	// Additive Bonus (+30) -> Base 120 + 30 = 150
	Ref<AttributeModifier> mod_bonus;
	mod_bonus.instantiate();
	mod_bonus->set_target_attribute("Health");
	mod_bonus->set_op(AttributeModifier::OP_ADDITIVE_BONUS);
	mod_bonus->set_value(30.0);
	attrs->add_modifier(mod_bonus);
	CHECK(attrs->get_attribute_max("Health") == 150.0);

	// Multiplicative (+0.5 = +50%) -> 150 * 1.5 = 225
	Ref<AttributeModifier> mod_mult;
	mod_mult.instantiate();
	mod_mult->set_target_attribute("Health");
	mod_mult->set_op(AttributeModifier::OP_MULTIPLICATIVE);
	mod_mult->set_value(0.5);
	attrs->add_modifier(mod_mult);
	CHECK(attrs->get_attribute_max("Health") == 225.0);

	// Override (999, prio 10) -> 999
	Ref<AttributeModifier> mod_override;
	mod_override.instantiate();
	mod_override->set_target_attribute("Health");
	mod_override->set_op(AttributeModifier::OP_OVERRIDE);
	mod_override->set_value(999.0);
	mod_override->set_priority(10);
	attrs->add_modifier(mod_override);
	CHECK(attrs->get_attribute_max("Health") == 999.0);

	// Remove override -> restores 225
	attrs->remove_modifier(mod_override);
	CHECK(attrs->get_attribute_max("Health") == 225.0);
}

TEST_CASE("[Gameplay] AttributeSet regen grace delay and step curve") {
	Ref<AttributeSet> attrs;
	attrs.instantiate();

	Ref<Curve> step_curve;
	step_curve.instantiate();
	step_curve->add_point(Vector2(0.0, 0.25)); // At 0% HP, regen rate is 25%
	step_curve->add_point(Vector2(1.0, 1.0)); // At 100% HP, regen rate is 100%

	// Attribute: Posture (0 to 100), base 100, regen rate 20/s, delay 1.0s
	attrs->add_attribute("Posture", 100.0, 0.0, 100.0, 20.0, 1.0, step_curve);
	attrs->set_attribute_current("Posture", 50.0);

	// Apply damage to reset delay timer
	attrs->apply_damage("Posture", 10.0); // Now 40.0, regen timer set to 1.0s
	CHECK(attrs->get_attribute_current("Posture") == 40.0);

	// Tick 0.5s (inside grace delay) -> no regen
	attrs->tick_regen(0.5);
	CHECK(attrs->get_attribute_current("Posture") == 40.0);

	// Tick 0.6s (total 1.1s > 1.0s delay) -> 0.1s of regen applied
	attrs->tick_regen(0.6);
	CHECK(attrs->get_attribute_current("Posture") > 40.0);
}

TEST_CASE("[Gameplay] Multi-channel damage execution with cut-rate mitigation") {
	Ref<AttributeSet> defender;
	defender.instantiate();
	defender->add_attribute("Health", 1000.0, 0.0, 1000.0);
	defender->add_attribute("Posture", 500.0, 0.0, 500.0);

	// Defender resists Slash by 50% (cut rate 0.5), Fire by 0% (cut rate 1.0), Posture by 20% (cut rate 0.8)
	defender->set_cut_rate("Damage.Physical.Slash", 0.5);
	defender->set_cut_rate("Damage.Elemental.Fire", 1.0);
	defender->set_cut_rate("Damage.Posture", 0.8);

	Ref<StandardDamageExecution> exec;
	exec.instantiate();

	Dictionary channels;
	channels["Damage.Physical.Slash"] = 100.0; // 100 * 0.5 = 50
	channels["Damage.Elemental.Fire"] = 40.0; // 40 * 1.0 = 40
	channels["Damage.Posture"] = 50.0; // 50 * 0.8 = 40

	Dictionary params;
	params["damage_channels"] = channels;

	Dictionary result = exec->execute_calculation(Ref<AttributeSet>(), defender, params);

	CHECK(double(result["vitality_damage"]) == doctest::Approx(90.0)); // 50 + 40
	CHECK(double(result["posture_damage"]) == doctest::Approx(40.0)); // 40
}

TEST_CASE("[Gameplay] ActiveGameplayEffect duration and stacking policies") {
	Ref<GameplayEffect> effect;
	effect.instantiate();
	effect->set_effect_id("BleedDoT");
	effect->set_duration_type(GameplayEffect::DURATION_HAS_DURATION);
	effect->set_duration(3.0);
	effect->set_period(1.0);
	effect->set_stacking_policy(GameplayEffect::STACK_ACCUMULATE);
	effect->set_max_stacks(3);

	Ref<ActiveGameplayEffect> active;
	active.instantiate();
	active->init(effect, "SwordSwing");

	CHECK(active->get_current_stacks() == 1);
	CHECK(active->get_time_remaining() == 3.0);

	// Add stack
	bool added = active->add_stack();
	CHECK(added);
	CHECK(active->get_current_stacks() == 2);

	// Tick 1.0s -> periodic execution triggers
	bool do_periodic = false;
	bool still_alive = active->tick(1.0, do_periodic);
	CHECK(still_alive);
	CHECK(do_periodic);
	CHECK(active->get_time_remaining() == 2.0);

	// Tick 2.1s -> expires
	still_alive = active->tick(2.1, do_periodic);
	CHECK_FALSE(still_alive);
}

TEST_CASE("[Gameplay] DamageResolver Parry, Block, Hit, and Immunity outcomes") {
	Ref<AttributeSet> attacker_attrs;
	attacker_attrs.instantiate();
	attacker_attrs->add_attribute("Health", 500.0, 0.0, 500.0);
	attacker_attrs->add_attribute("Posture", 300.0, 0.0, 300.0);

	Ref<AttributeSet> defender_attrs;
	defender_attrs.instantiate();
	defender_attrs->add_attribute("Health", 1000.0, 0.0, 1000.0);
	defender_attrs->add_attribute("Posture", 400.0, 0.0, 400.0);

	Ref<GameplayTagContainer> defender_tags;
	defender_tags.instantiate();

	Dictionary channels;
	channels["Damage.Physical.Slash"] = 80.0;
	channels["Damage.Posture"] = 30.0;

	DamageResolver resolver;

	// Case 1: Neutral Clean Hit
	Dictionary res_hit = resolver.resolve_interaction(attacker_attrs, Ref<GameplayTagContainer>(), channels, defender_attrs, defender_tags);
	CHECK(int(res_hit["outcome"]) == DamageResolver::OUTCOME_HIT);
	CHECK(double(res_hit["vitality_damage_dealt"]) == doctest::Approx(80.0));
	CHECK(defender_attrs->get_attribute_current("Health") == 920.0);

	// Case 2: Block (Guarding)
	defender_tags->add_tag("Combat.State.Guarding");
	Dictionary res_block = resolver.resolve_interaction(attacker_attrs, Ref<GameplayTagContainer>(), channels, defender_attrs, defender_tags);
	CHECK(int(res_block["outcome"]) == DamageResolver::OUTCOME_BLOCK);
	CHECK(defender_attrs->get_attribute_current("Health") == 920.0); // Health preserved
	CHECK(defender_attrs->get_attribute_current("Posture") > 0.0); // Posture damage taken

	// Case 3: Parry (Parrying)
	defender_tags->clear();
	defender_tags->add_tag("Combat.State.Parrying");
	Dictionary res_parry = resolver.resolve_interaction(attacker_attrs, Ref<GameplayTagContainer>(), channels, defender_attrs, defender_tags);
	CHECK(int(res_parry["outcome"]) == DamageResolver::OUTCOME_PARRY);
	CHECK(attacker_attrs->get_attribute_current("Posture") > 0.0); // Attacker suffers posture recoil!

	// Case 4: Invulnerability
	defender_tags->clear();
	defender_tags->add_tag("Combat.State.Invulnerable");
	Dictionary res_immune = resolver.resolve_interaction(attacker_attrs, Ref<GameplayTagContainer>(), channels, defender_attrs, defender_tags);
	CHECK(int(res_immune["outcome"]) == DamageResolver::OUTCOME_IMMUNE);
}

TEST_CASE("[Gameplay] ActionTimeline phase evaluation and cancel windows") {
	Ref<ActionTimeline> timeline;
	timeline.instantiate();
	timeline->set_duration(1.0);

	Ref<CancelWindowTimelineTrack> cancel_track;
	cancel_track.instantiate();
	cancel_track->set_start_phase(0.6);
	cancel_track->set_end_phase(1.0);
	cancel_track->set_allowed_cancel_actions(PackedStringArray{ "Action.Dodge", "Action.ComboNext" });
	timeline->add_track(cancel_track);

	// Phase 0.3 -> No cancel
	CHECK_FALSE(timeline->is_cancel_available(0.3, "Action.Dodge"));

	// Phase 0.7 -> Dodge and ComboNext allowed, Attack not allowed
	CHECK(timeline->is_cancel_available(0.7, "Action.Dodge"));
	CHECK(timeline->is_cancel_available(0.7, "Action.ComboNext"));
	CHECK_FALSE(timeline->is_cancel_available(0.7, "Action.HeavyAttack"));
}

TEST_CASE("[Gameplay] InputBufferComponent priority and rolling expiration") {
	InputBufferComponent buffer;
	buffer.set_buffer_window(0.2); // 200ms

	buffer.record_input("LightAttack", 1);
	buffer.record_input("Dodge", 5); // Higher priority!
	buffer.record_input("Jump", 2);

	CHECK(buffer.get_buffered_input_count() == 3);
	CHECK(buffer.has_buffered_input("Dodge"));

	// Pop highest priority -> "Dodge" (priority 5)
	StringName popped = buffer.pop_highest_priority_input();
	CHECK(popped == StringName("Dodge"));
	CHECK(buffer.get_buffered_input_count() == 2);

	// Pop next -> "Jump" (priority 2)
	popped = buffer.pop_highest_priority_input();
	CHECK(popped == StringName("Jump"));

	// Consume remaining
	bool consumed = buffer.consume_input("LightAttack");
	CHECK(consumed);
	CHECK(buffer.get_buffered_input_count() == 0);
}

TEST_CASE("[Gameplay] CombatAction resource cost and tag requirement gating") {
	Ref<CombatAction> action;
	action.instantiate();
	action->set_action_name("HeavySlash");

	Dictionary costs;
	costs["Stamina"] = 30.0;
	action->set_resource_costs(costs);

	PackedStringArray req_tags;
	req_tags.push_back("Weapon.Equipped");
	action->set_activation_required_tags(GameplayTagQuery::create_match_all(req_tags));

	PackedStringArray blk_tags;
	blk_tags.push_back("Combat.State.Staggered");
	action->set_activation_blocked_tags(GameplayTagQuery::create_match_any(blk_tags));

	Ref<AttributeSet> attrs;
	attrs.instantiate();
	attrs->add_attribute("Stamina", 100.0, 0.0, 100.0);

	Ref<GameplayTagContainer> tags;
	tags.instantiate();

	// Missing Weapon.Equipped tag
	CHECK_FALSE(action->can_activate(attrs, tags));

	// Add Weapon.Equipped tag
	tags->add_tag("Weapon.Equipped");
	CHECK(action->can_activate(attrs, tags));

	// Add Staggered tag -> blocked
	tags->add_tag("Combat.State.Staggered");
	CHECK_FALSE(action->can_activate(attrs, tags));
	tags->remove_tag("Combat.State.Staggered");

	// Drain stamina below 30 -> blocked
	attrs->set_attribute_current("Stamina", 15.0);
	CHECK_FALSE(action->can_activate(attrs, tags));

	// Restore stamina and consume costs
	attrs->set_attribute_current("Stamina", 50.0);
	CHECK(action->can_activate(attrs, tags));
	action->consume_costs(attrs);
	CHECK(attrs->get_attribute_current("Stamina") == 20.0);
}

TEST_CASE("[Gameplay] ComboGraph branching and stance requirement routing") {
	Ref<ComboGraph> graph;
	graph.instantiate();
	graph->set_initial_action_name("Slash1");

	Ref<ComboNode> node_slash1;
	node_slash1.instantiate();
	node_slash1->set_action_name("Slash1");

	// Edge 1: LightAttack -> Slash2
	Ref<ComboEdge> edge_light;
	edge_light.instantiate();
	edge_light->set_input_action_name("LightAttack");
	edge_light->set_target_action_name("Slash2");
	node_slash1->add_edge(edge_light);

	// Edge 2: HeavyAttack -> HeavyFinisher
	Ref<ComboEdge> edge_heavy;
	edge_heavy.instantiate();
	edge_heavy->set_input_action_name("HeavyAttack");
	edge_heavy->set_target_action_name("HeavyFinisher");
	node_slash1->add_edge(edge_heavy);

	// Edge 3: LightAttack with Stance.Aggressive -> Whirlwind (higher priority 10)
	Ref<ComboEdge> edge_stance;
	edge_stance.instantiate();
	edge_stance->set_input_action_name("LightAttack");
	edge_stance->set_target_action_name("Whirlwind");
	edge_stance->set_priority(10);
	edge_stance->set_required_tags(GameplayTagQuery::create_match_all(PackedStringArray{ "Combat.Stance.Aggressive" }));
	node_slash1->add_edge(edge_stance);

	graph->add_node(node_slash1);

	Ref<GameplayTagContainer> tags;
	tags.instantiate();

	// Test 1: Standard Light Attack -> Slash2
	StringName next = graph->get_next_action("Slash1", "LightAttack", tags);
	CHECK(next == StringName("Slash2"));

	// Test 2: Standard Heavy Attack -> HeavyFinisher
	next = graph->get_next_action("Slash1", "HeavyAttack", tags);
	CHECK(next == StringName("HeavyFinisher"));

	// Test 3: Light Attack with Stance -> Whirlwind
	tags->add_tag("Combat.Stance.Aggressive");
	next = graph->get_next_action("Slash1", "LightAttack", tags);
	CHECK(next == StringName("Whirlwind"));
}

TEST_CASE("[Gameplay] ContextualReactionEngine deterministic counter matching") {
	ContextualReactionEngine engine;

	Ref<ContextualReactionRule> thrust_counter;
	thrust_counter.instantiate();
	thrust_counter->set_rule_name("ThrustCounter");
	thrust_counter->set_incoming_threat_query(GameplayTagQuery::create_match_all(PackedStringArray{ "Combat.Telegraph.Thrust" }));
	thrust_counter->set_required_input_action("Action.DodgeForward");
	thrust_counter->set_resulting_action_name("Action.MikiriStomp");

	Ref<ContextualReactionRule> sweep_counter;
	sweep_counter.instantiate();
	sweep_counter->set_rule_name("SweepCounter");
	sweep_counter->set_incoming_threat_query(GameplayTagQuery::create_match_all(PackedStringArray{ "Combat.Telegraph.Sweep" }));
	sweep_counter->set_required_input_action("Action.Jump");
	sweep_counter->set_resulting_action_name("Action.JumpHeadKick");

	engine.add_rule(thrust_counter);
	engine.add_rule(sweep_counter);

	Ref<GameplayTagContainer> incoming_thrust;
	incoming_thrust.instantiate();
	incoming_thrust->add_tag("Combat.Telegraph.Thrust");

	Ref<GameplayTagContainer> incoming_sweep;
	incoming_sweep.instantiate();
	incoming_sweep->add_tag("Combat.Telegraph.Sweep");

	Ref<GameplayTagContainer> self_tags;
	self_tags.instantiate();

	// Dodge forward against thrust -> Mikiri Stomp
	StringName res_thrust = engine.evaluate_reaction(incoming_thrust, self_tags, "Action.DodgeForward");
	CHECK(res_thrust == StringName("Action.MikiriStomp"));

	// Jump against sweep -> Jump Head Kick
	StringName res_sweep = engine.evaluate_reaction(incoming_sweep, self_tags, "Action.Jump");
	CHECK(res_sweep == StringName("Action.JumpHeadKick"));

	// Jump against thrust -> No reaction
	StringName res_mismatch = engine.evaluate_reaction(incoming_thrust, self_tags, "Action.Jump");
	CHECK(res_mismatch == StringName());
}

TEST_CASE("[Gameplay] PairedInteraction and SyncPointComponent execution") {
	Ref<PairedInteraction> deathblow;
	deathblow.instantiate();
	deathblow->set_interaction_id("DeathblowFront");
	deathblow->set_duration(1.0);
	deathblow->set_alignment_duration(0.1);

	Dictionary dmg;
	dmg["Health"] = 500.0;
	deathblow->set_damage_payload(dmg);

	Ref<GameplayTagContainer> invuln_tags;
	invuln_tags.instantiate();
	invuln_tags->add_tag("Combat.State.Invulnerable");
	deathblow->set_attacker_granted_tags(invuln_tags);
	deathblow->set_victim_granted_tags(invuln_tags);

	Node3D attacker;
	Node3D victim;
	victim.set_position(Vector3(0, 0, -3.0));

	Ref<AttributeSet> victim_attrs;
	victim_attrs.instantiate();
	victim_attrs->add_attribute("Health", 1000.0, 0.0, 1000.0);
	victim.set_meta("attribute_set", victim_attrs);

	Ref<GameplayTagContainer> victim_tags;
	victim_tags.instantiate();
	victim.set_meta("state_tags", victim_tags);

	Ref<GameplayTagContainer> attacker_tags;
	attacker_tags.instantiate();
	attacker.set_meta("state_tags", attacker_tags);

	SyncPointComponent sync_comp;
	bool initiated = sync_comp.initiate_interaction(&attacker, &victim, deathblow);
	CHECK(initiated);
	CHECK(sync_comp.get_current_state() == SyncPointComponent::SYNC_ALIGNING);

	// Attacker & victim granted invulnerability tags during interaction
	CHECK(attacker_tags->has_tag("Combat.State.Invulnerable"));
	CHECK(victim_tags->has_tag("Combat.State.Invulnerable"));

	// Finish interaction
	sync_comp.finish_interaction();
	CHECK(sync_comp.get_current_state() == SyncPointComponent::SYNC_IDLE);

	// Damage payload applied to victim
	CHECK(victim_attrs->get_attribute_current("Health") == 500.0);

	// Invulnerability tags stripped after interaction finishes
	CHECK_FALSE(attacker_tags->has_tag("Combat.State.Invulnerable"));
	CHECK_FALSE(victim_tags->has_tag("Combat.State.Invulnerable"));
}

TEST_CASE("[Gameplay] MoveConsideration response curves and distance scoring") {
	Node3D self_node;
	self_node.set_position(Vector3(0, 0, 0));

	Node3D target_node;
	target_node.set_position(Vector3(5, 0, 0)); // 5 meters away

	Ref<Curve> close_range_curve;
	close_range_curve.instantiate();
	close_range_curve->add_point(Vector2(0.0, 1.0)); // High utility when close (0m)
	close_range_curve->add_point(Vector2(1.0, 0.0)); // Low utility when far (10m)

	Ref<MoveConsideration> cons;
	cons.instantiate();
	cons->set_metric(MoveConsideration::METRIC_DISTANCE_TO_TARGET);
	cons->set_min_range(0.0);
	cons->set_max_range(10.0);
	cons->set_response_curve(close_range_curve);

	// Distance 5m is 50% normalized distance -> curve outputs approx 0.5 utility
	real_t score_5m = cons->score(&self_node, &target_node, Ref<AttributeSet>(), Ref<AttributeSet>(), Ref<GameplayTagContainer>());
	CHECK(score_5m > 0.3);
	CHECK(score_5m < 0.7);

	// Distance 0m (close) -> score 1.0
	target_node.set_position(Vector3(0, 0, 0));
	real_t score_0m = cons->score(&self_node, &target_node, Ref<AttributeSet>(), Ref<AttributeSet>(), Ref<GameplayTagContainer>());
	CHECK(score_0m == doctest::Approx(1.0));

	// Distance 10m (far) -> score 0.0
	target_node.set_position(Vector3(10, 0, 0));
	real_t score_10m = cons->score(&self_node, &target_node, Ref<AttributeSet>(), Ref<AttributeSet>(), Ref<GameplayTagContainer>());
	CHECK(score_10m == doctest::Approx(0.0));
}

TEST_CASE("[Gameplay] PhaseTransitionController health threshold triggers") {
	Ref<AttributeSet> boss_attrs;
	boss_attrs.instantiate();
	boss_attrs->add_attribute("Health", 1000.0, 0.0, 1000.0);

	Ref<GameplayTagContainer> boss_tags;
	boss_tags.instantiate();

	PhaseTransitionController controller;
	controller.set_attribute_set(boss_attrs);
	controller.set_state_tags(boss_tags);

	Ref<PhaseTransitionRule> phase_2_rule;
	phase_2_rule.instantiate();
	phase_2_rule->set_phase_index(1);
	phase_2_rule->set_health_threshold_pct(0.5); // Triggers at <= 50% HP

	Ref<GameplayTagContainer> phase_2_tags;
	phase_2_tags.instantiate();
	phase_2_tags->add_tag("Boss.Phase.Enraged");
	phase_2_rule->set_granted_phase_tags(phase_2_tags);

	TypedArray<PhaseTransitionRule> rules;
	rules.push_back(phase_2_rule);
	controller.set_phase_rules(rules);

	CHECK(controller.get_current_phase() == 0);

	// Health at 80% (800 HP) -> no transition
	boss_attrs->set_attribute_current("Health", 800.0);
	controller.check_phase_transition();
	CHECK(controller.get_current_phase() == 0);

	// Health drops to 40% (400 HP) -> triggers phase 1
	boss_attrs->set_attribute_current("Health", 400.0);
	controller.check_phase_transition();
	CHECK(controller.get_current_phase() == 1);
	CHECK(controller.get_is_transitioning());
	CHECK(boss_tags->has_tag("Boss.Phase.Enraged"));

	controller.complete_transition();
	CHECK_FALSE(controller.get_is_transitioning());
}

TEST_CASE("[Gameplay] CameraShakeProfile offset evaluation and decay") {
	Ref<CameraShakeProfile> profile;
	profile.instantiate();
	profile->set_duration(0.5);
	profile->set_frequency(10.0);
	profile->set_amplitude(Vector3(2.0, 2.0, 0.0));

	// At t=0 -> has offset
	Vector3 off_start = profile->evaluate_offset(0.01);
	CHECK(off_start.length() > 0.0);

	// At t=0.5 (end of duration) -> offset decays to zero
	Vector3 off_end = profile->evaluate_offset(0.5);
	CHECK(off_end.length() == doctest::Approx(0.0));

	// CameraShakeEmitter playback
	CameraShakeEmitter emitter;
	emitter.play_shake(profile);
	emitter.stop_all_shakes();
	CHECK(emitter.get_current_shake_offset() == Vector3());
}

TEST_CASE("[Gameplay] CombatTelemetry metrics aggregation and success rate") {
	CombatTelemetry telemetry;
	telemetry.reset_session();

	telemetry.record_hit(100.0);
	telemetry.record_hit(150.0);
	telemetry.record_damage_taken(50.0);
	telemetry.record_parry();
	telemetry.record_parry();
	telemetry.record_block(30.0);

	telemetry.tick_session(2.0); // 2 seconds

	Dictionary summary = telemetry.get_summary();
	CHECK(double(summary["total_damage_dealt"]) == 250.0);
	CHECK(double(summary["total_damage_taken"]) == 50.0);
	CHECK(double(summary["dps"]) == 125.0); // 250 / 2.0s
	CHECK(int(summary["total_hits"]) == 2);
	CHECK(int(summary["total_parries"]) == 2);
	CHECK(int(summary["total_blocks"]) == 1);

	// 2 parries / (2 parries + 1 block) = 2/3 = 0.6667
	CHECK(double(summary["parry_success_rate"]) == doctest::Approx(0.666667));
}

} // namespace TestGameplayCore
