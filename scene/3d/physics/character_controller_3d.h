/**************************************************************************/
/*  character_controller_3d.h                                             */
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

#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/spring_arm_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/material.h"
#include "scene/resources/packed_scene.h"

class CharacterController3D : public CharacterBody3D {
	GDCLASS(CharacterController3D, CharacterBody3D);

public:
	enum ViewMode {
		VIEW_FIRST_PERSON,
		VIEW_THIRD_PERSON,
		VIEW_DYNAMIC_TOGGLE,
	};

	enum FacingMode {
		FACING_CAMERA_DIRECTION, // Strafe mode (Shooter style)
		FACING_MOVEMENT_DIRECTION, // Orient to movement direction (Adventure/RPG style)
	};

	enum AvatarVisibility {
		AVATAR_VISIBLE,
		AVATAR_SHADOWS_ONLY,
		AVATAR_HIDDEN,
	};

	enum LocomotionState {
		STATE_IDLE,
		STATE_WALK,
		STATE_SPRINT,
		STATE_CROUCH,
		STATE_JUMP_RISE,
		STATE_AIR_FALL,
		STATE_LAND,
		STATE_SLIDE,
	};

private:
	// Internal Sub-Nodes
	Node3D *avatar_root = nullptr;
	MeshInstance3D *default_capsule_mesh = nullptr;
	Node3D *custom_avatar_instance = nullptr;
	CollisionShape3D *internal_collision_shape = nullptr;
	Ref<CapsuleShape3D> internal_capsule_shape;

	Node3D *head_pivot = nullptr;
	SpringArm3D *spring_arm = nullptr;
	Camera3D *camera = nullptr;

	// View & Camera Properties
	ViewMode view_mode = VIEW_FIRST_PERSON;
	FacingMode facing_mode = FACING_CAMERA_DIRECTION;
	real_t eye_height = 1.6;
	real_t crouch_eye_height = 0.9;
	real_t mouse_sensitivity_x = 0.003;
	real_t mouse_sensitivity_y = 0.003;
	real_t mouse_smoothing = 0.0;
	bool invert_mouse_y = false;
	real_t min_pitch_angle = -89.0;
	real_t max_pitch_angle = 89.0;
	real_t third_person_distance = 3.5;
	Vector3 third_person_offset = Vector3(0.4, 0.2, 0.0);
	real_t first_person_fov = 75.0;
	real_t third_person_fov = 75.0;
	bool auto_capture_mouse = true;
	bool active_camera = true;

	// Camera Polish Properties
	bool head_bob_enabled = true;
	real_t head_bob_frequency = 2.4;
	real_t head_bob_amplitude_h = 0.03;
	real_t head_bob_amplitude_v = 0.05;
	bool sprint_fov_kick_enabled = true;
	real_t sprint_fov_kick_amount = 10.0;
	bool strafe_roll_enabled = true;
	real_t strafe_roll_max_angle = 2.0;

	// Movement Properties
	real_t walk_speed = 5.0;
	real_t sprint_speed = 8.5;
	real_t crouch_speed = 2.5;
	real_t ground_acceleration = 14.0;
	real_t ground_deceleration = 12.0;
	real_t air_acceleration = 4.0;
	real_t air_control_multiplier = 0.4;
	real_t air_drag = 0.2;
	real_t rotation_smooth_rate = 12.0;

	// Jump & Gravity Properties
	real_t jump_height = 1.25;
	int max_air_jumps = 0;
	real_t coyote_time = 0.15;
	real_t jump_buffer_time = 0.15;
	bool variable_jump_height = true;
	real_t gravity_scale = 1.0;
	real_t fall_gravity_multiplier = 1.5;

	// Crouch & Collision Dimensions
	real_t standing_height = 1.8;
	real_t crouch_height = 1.0;
	real_t capsule_radius = 0.4;
	real_t crouch_transition_speed = 10.0;

	// Stairs & Slopes
	bool stair_stepping_enabled = true;
	real_t max_step_height = 0.35;
	real_t step_forward_distance = 0.45;
	bool slide_on_steep_slopes = true;

	// Avatar / Model Ingestion
	Ref<PackedScene> custom_avatar_scene;
	Vector3 avatar_offset = Vector3(0, 0, 0);
	Vector3 avatar_rotation_offset = Vector3(0, 0, 0);
	AvatarVisibility first_person_avatar_visibility = AVATAR_SHADOWS_ONLY;
	bool show_capsule_in_editor = true;

	// Input Action Names (with fallback to default keys if not mapped)
	StringName action_move_forward = "move_forward";
	StringName action_move_back = "move_back";
	StringName action_move_left = "move_left";
	StringName action_move_right = "move_right";
	StringName action_jump = "jump";
	StringName action_sprint = "sprint";
	StringName action_crouch = "crouch";
	StringName action_toggle_view = "toggle_camera_mode";

	// Runtime State Tracking
	LocomotionState current_state = STATE_IDLE;
	Vector2 input_vector_raw;
	Vector2 input_vector_smooth;
	Vector2 mouse_delta_accum;
	real_t camera_pitch = 0.0;
	real_t camera_yaw = 0.0;
	real_t target_yaw = 0.0;
	real_t avatar_yaw = 0.0;

	real_t current_capsule_height = 1.8;
	real_t target_capsule_height = 1.8;
	real_t current_eye_height = 1.6;
	real_t target_eye_height = 1.6;

	int remaining_air_jumps = 0;
	real_t coyote_timer = 0.0;
	real_t jump_buffer_timer = 0.0;
	real_t time_in_air = 0.0;
	bool is_grounded_cached = true;
	bool is_crouching_active = false;
	bool is_sprinting_active = false;

	real_t head_bob_cycle = 0.0;
	real_t footstep_cycle_accum = 0.0;
	int last_foot_index = 0;
	real_t landing_recovery_timer = 0.0;
	real_t last_fall_velocity = 0.0;

	// Private Helper Methods
	void _update_internal_hierarchy();
	void _update_capsule_mesh();
	void _update_avatar_instance();
	void _update_avatar_visibility();
	void _update_camera_mode();
	void _handle_mouse_look(real_t p_delta);
	void _handle_movement_physics(real_t p_delta);
	void _handle_stair_stepping(real_t p_delta, const Vector3 &p_move_dir);
	bool _can_uncrouch() const;
	void _apply_camera_polish(real_t p_delta, real_t p_speed_ratio);
	void _update_telemetry_and_state();

protected:
	void _notification(int p_what);
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;
	static void _bind_methods();

public:
	// View & Camera Set/Get
	void set_view_mode(ViewMode p_mode);
	ViewMode get_view_mode() const;

	void set_facing_mode(FacingMode p_mode);
	FacingMode get_facing_mode() const;

	void set_eye_height(real_t p_height);
	real_t get_eye_height() const;

	void set_crouch_eye_height(real_t p_height);
	real_t get_crouch_eye_height() const;

	void set_mouse_sensitivity_x(real_t p_val);
	real_t get_mouse_sensitivity_x() const;

	void set_mouse_sensitivity_y(real_t p_val);
	real_t get_mouse_sensitivity_y() const;

	void set_mouse_smoothing(real_t p_val);
	real_t get_mouse_smoothing() const;

	void set_invert_mouse_y(bool p_invert);
	bool is_mouse_y_inverted() const;

	void set_min_pitch_angle(real_t p_angle);
	real_t get_min_pitch_angle() const;

	void set_max_pitch_angle(real_t p_angle);
	real_t get_max_pitch_angle() const;

	void set_third_person_distance(real_t p_distance);
	real_t get_third_person_distance() const;

	void set_third_person_offset(const Vector3 &p_offset);
	Vector3 get_third_person_offset() const;

	void set_first_person_fov(real_t p_fov);
	real_t get_first_person_fov() const;

	void set_third_person_fov(real_t p_fov);
	real_t get_third_person_fov() const;

	void set_auto_capture_mouse(bool p_capture);
	bool is_auto_capture_mouse_enabled() const;

	void set_active_camera(bool p_active);
	bool is_active_camera() const;

	// Camera Polish Set/Get
	void set_head_bob_enabled(bool p_enabled);
	bool is_head_bob_enabled() const;

	void set_head_bob_frequency(real_t p_val);
	real_t get_head_bob_frequency() const;

	void set_head_bob_amplitude_h(real_t p_val);
	real_t get_head_bob_amplitude_h() const;

	void set_head_bob_amplitude_v(real_t p_val);
	real_t get_head_bob_amplitude_v() const;

	void set_sprint_fov_kick_enabled(bool p_enabled);
	bool is_sprint_fov_kick_enabled() const;

	void set_sprint_fov_kick_amount(real_t p_val);
	real_t get_sprint_fov_kick_amount() const;

	void set_strafe_roll_enabled(bool p_enabled);
	bool is_strafe_roll_enabled() const;

	void set_strafe_roll_max_angle(real_t p_val);
	real_t get_strafe_roll_max_angle() const;

	// Movement Set/Get
	void set_walk_speed(real_t p_speed);
	real_t get_walk_speed() const;

	void set_sprint_speed(real_t p_speed);
	real_t get_sprint_speed() const;

	void set_crouch_speed(real_t p_speed);
	real_t get_crouch_speed() const;

	void set_ground_acceleration(real_t p_accel);
	real_t get_ground_acceleration() const;

	void set_ground_deceleration(real_t p_decel);
	real_t get_ground_deceleration() const;

	void set_air_acceleration(real_t p_accel);
	real_t get_air_acceleration() const;

	void set_air_control_multiplier(real_t p_mult);
	real_t get_air_control_multiplier() const;

	void set_air_drag(real_t p_drag);
	real_t get_air_drag() const;

	void set_rotation_smooth_rate(real_t p_rate);
	real_t get_rotation_smooth_rate() const;

	// Jump & Gravity Set/Get
	void set_jump_height(real_t p_height);
	real_t get_jump_height() const;

	void set_max_air_jumps(int p_count);
	int get_max_air_jumps() const;

	void set_coyote_time(real_t p_time);
	real_t get_coyote_time() const;

	void set_jump_buffer_time(real_t p_time);
	real_t get_jump_buffer_time() const;

	void set_variable_jump_height(bool p_enabled);
	bool is_variable_jump_height_enabled() const;

	void set_gravity_scale(real_t p_scale);
	real_t get_gravity_scale() const;

	void set_fall_gravity_multiplier(real_t p_mult);
	real_t get_fall_gravity_multiplier() const;

	// Crouch & Collision Dimensions Set/Get
	void set_standing_height(real_t p_height);
	real_t get_standing_height() const;

	void set_crouch_height(real_t p_height);
	real_t get_crouch_height() const;

	void set_capsule_radius(real_t p_radius);
	real_t get_capsule_radius() const;

	void set_crouch_transition_speed(real_t p_speed);
	real_t get_crouch_transition_speed() const;

	// Stairs & Slopes Set/Get
	void set_stair_stepping_enabled(bool p_enabled);
	bool is_stair_stepping_enabled() const;

	void set_max_step_height(real_t p_height);
	real_t get_max_step_height() const;

	void set_step_forward_distance(real_t p_dist);
	real_t get_step_forward_distance() const;

	void set_slide_on_steep_slopes(bool p_slide);
	bool is_slide_on_steep_slopes_enabled() const;

	// Avatar Ingestion Set/Get
	void set_custom_avatar_scene(const Ref<PackedScene> &p_scene);
	Ref<PackedScene> get_custom_avatar_scene() const;

	void set_avatar_offset(const Vector3 &p_offset);
	Vector3 get_avatar_offset() const;

	void set_avatar_rotation_offset(const Vector3 &p_rot_deg);
	Vector3 get_avatar_rotation_offset() const;

	void set_first_person_avatar_visibility(AvatarVisibility p_visibility);
	AvatarVisibility get_first_person_avatar_visibility() const;

	void set_show_capsule_in_editor(bool p_show);
	bool is_show_capsule_in_editor_enabled() const;

	// Input Action Names Set/Get
	void set_action_move_forward(const StringName &p_name);
	StringName get_action_move_forward() const;

	void set_action_move_back(const StringName &p_name);
	StringName get_action_move_back() const;

	void set_action_move_left(const StringName &p_name);
	StringName get_action_move_left() const;

	void set_action_move_right(const StringName &p_name);
	StringName get_action_move_right() const;

	void set_action_jump(const StringName &p_name);
	StringName get_action_jump() const;

	void set_action_sprint(const StringName &p_name);
	StringName get_action_sprint() const;

	void set_action_crouch(const StringName &p_name);
	StringName get_action_crouch() const;

	void set_action_toggle_view(const StringName &p_name);
	StringName get_action_toggle_view() const;

	// Telemetry & Phase 2 Locomotion API
	LocomotionState get_locomotion_state() const;
	Vector2 get_movement_vector_2d() const;
	real_t get_horizontal_speed() const;
	real_t get_turn_rate() const;
	bool is_sprinting() const;
	bool is_crouching() const;
	real_t get_time_in_air() const;

	// Sub-Node Accessors
	Node3D *get_avatar_root() const;
	Node3D *get_head_pivot() const;
	SpringArm3D *get_spring_arm() const;
	Camera3D *get_camera() const;

	CharacterController3D();
	~CharacterController3D();
};

VARIANT_ENUM_CAST(CharacterController3D::ViewMode);
VARIANT_ENUM_CAST(CharacterController3D::FacingMode);
VARIANT_ENUM_CAST(CharacterController3D::AvatarVisibility);
VARIANT_ENUM_CAST(CharacterController3D::LocomotionState);
