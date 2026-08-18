/**************************************************************************/
/*  character_controller_3d.cpp                                           */
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

#include "character_controller_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/object/class_db.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/material.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"

CharacterController3D::CharacterController3D() {
	// Enable floor constant speed and slope stopping by default for clean movement
	set_floor_constant_speed_enabled(true);
	set_floor_stop_on_slope_enabled(true);
	set_floor_max_angle(Math::deg_to_rad(45.0));
	set_floor_snap_length(0.35);

	_update_internal_hierarchy();
}

CharacterController3D::~CharacterController3D() {
}

void CharacterController3D::_update_internal_hierarchy() {
	// 1. Avatar Root (Holds default capsule or custom model)
	if (!avatar_root) {
		avatar_root = memnew(Node3D);
		avatar_root->set_name("AvatarRoot");
		add_child(avatar_root, false, INTERNAL_MODE_FRONT);
	}

	// 2. Default Capsule Mesh
	if (!default_capsule_mesh) {
		default_capsule_mesh = memnew(MeshInstance3D);
		default_capsule_mesh->set_name("DefaultCapsuleMesh");
		avatar_root->add_child(default_capsule_mesh, false, INTERNAL_MODE_FRONT);
	}
	_update_capsule_mesh();

	// 3. Collision Shape
	if (!internal_collision_shape) {
		internal_collision_shape = memnew(CollisionShape3D);
		internal_collision_shape->set_name("InternalCollisionShape");
		internal_capsule_shape.instantiate();
		internal_capsule_shape->set_radius(capsule_radius);
		internal_capsule_shape->set_height(standing_height);
		internal_collision_shape->set_shape(internal_capsule_shape);
		internal_collision_shape->set_position(Vector3(0, standing_height * 0.5, 0));
		add_child(internal_collision_shape, false, INTERNAL_MODE_FRONT);
	}

	// 4. Head Pivot (Eye level)
	if (!head_pivot) {
		head_pivot = memnew(Node3D);
		head_pivot->set_name("HeadPivot");
		head_pivot->set_position(Vector3(0, eye_height, 0));
		add_child(head_pivot, false, INTERNAL_MODE_FRONT);
	}

	// 5. Spring Arm (Third Person Orbit & Collision)
	if (!spring_arm) {
		spring_arm = memnew(SpringArm3D);
		spring_arm->set_name("SpringArm");
		spring_arm->set_length(third_person_distance);
		spring_arm->set_margin(0.25);
		spring_arm->set_position(third_person_offset);
		head_pivot->add_child(spring_arm, false, INTERNAL_MODE_FRONT);
	}

	// 6. Camera
	if (!camera) {
		camera = memnew(Camera3D);
		camera->set_name("ControllerCamera");
		camera->set_fov(view_mode == VIEW_FIRST_PERSON ? first_person_fov : third_person_fov);
		_update_camera_mode();
	}

	_update_avatar_instance();
	_update_avatar_visibility();
}

void CharacterController3D::_update_capsule_mesh() {
	if (!default_capsule_mesh) {
		return;
	}

	if (custom_avatar_scene.is_valid()) {
		default_capsule_mesh->set_visible(false);
		return;
	}

	Ref<CapsuleMesh> cap_mesh = default_capsule_mesh->get_mesh();
	if (cap_mesh.is_null()) {
		cap_mesh.instantiate();
		default_capsule_mesh->set_mesh(cap_mesh);
	}

	cap_mesh->set_radius(capsule_radius);
	cap_mesh->set_height(current_capsule_height);
	default_capsule_mesh->set_position(Vector3(0, current_capsule_height * 0.5, 0));

	// Stylized default capsule material with subtle metallic sheen
	Ref<StandardMaterial3D> mat = cap_mesh->get_material();
	if (mat.is_null()) {
		mat.instantiate();
		mat->set_albedo(Color(0.22, 0.55, 0.95)); // Sleek ZeGFX blue
		mat->set_roughness(0.4);
		mat->set_metallic(0.1);
		cap_mesh->set_material(mat);
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		default_capsule_mesh->set_visible(show_capsule_in_editor);
	} else {
		_update_avatar_visibility();
	}
}

void CharacterController3D::_update_avatar_instance() {
	if (!avatar_root) {
		return;
	}

	if (custom_avatar_instance) {
		avatar_root->remove_child(custom_avatar_instance);
		custom_avatar_instance->queue_free();
		custom_avatar_instance = nullptr;
	}

	if (custom_avatar_scene.is_valid()) {
		Node *inst = custom_avatar_scene->instantiate();
		Node3D *inst3d = Object::cast_to<Node3D>(inst);
		if (inst3d) {
			custom_avatar_instance = inst3d;
			avatar_root->add_child(custom_avatar_instance);
			custom_avatar_instance->set_position(avatar_offset);
			custom_avatar_instance->set_rotation_degrees(avatar_rotation_offset);
		} else if (inst) {
			inst->queue_free();
		}
	}

	_update_capsule_mesh();
	_update_avatar_visibility();
}

void CharacterController3D::_update_avatar_visibility() {
	if (Engine::get_singleton()->is_editor_hint()) {
		if (avatar_root) {
			avatar_root->set_visible(true);
		}
		return;
	}

	if (!avatar_root) {
		return;
	}

	if (view_mode == VIEW_FIRST_PERSON) {
		switch (first_person_avatar_visibility) {
			case AVATAR_HIDDEN: {
				avatar_root->set_visible(false);
			} break;
			case AVATAR_SHADOWS_ONLY: {
				avatar_root->set_visible(true);
				if (default_capsule_mesh) {
					default_capsule_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_SHADOWS_ONLY);
				}
			} break;
			case AVATAR_VISIBLE: {
				avatar_root->set_visible(true);
				if (default_capsule_mesh) {
					default_capsule_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_ON);
				}
			} break;
		}
	} else {
		avatar_root->set_visible(true);
		if (default_capsule_mesh) {
			default_capsule_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_ON);
		}
	}
}

void CharacterController3D::_update_camera_mode() {
	if (!camera || !head_pivot || !spring_arm) {
		return;
	}

	if (view_mode == VIEW_FIRST_PERSON) {
		if (camera->get_parent() != head_pivot) {
			if (camera->get_parent()) {
				camera->get_parent()->remove_child(camera);
			}
			head_pivot->add_child(camera, false, INTERNAL_MODE_FRONT);
		}
		camera->set_position(Vector3(0, 0, 0));
		camera->set_rotation(Vector3(0, 0, 0));
		camera->set_fov(first_person_fov);
		spring_arm->set_process_internal(false);
	} else {
		if (camera->get_parent() != spring_arm) {
			if (camera->get_parent()) {
				camera->get_parent()->remove_child(camera);
			}
			spring_arm->add_child(camera, false, INTERNAL_MODE_FRONT);
		}
		camera->set_position(Vector3(0, 0, 0));
		camera->set_rotation(Vector3(0, 0, 0));
		camera->set_fov(third_person_fov);
		spring_arm->set_length(third_person_distance);
		spring_arm->set_position(third_person_offset);
	}

	if (active_camera && is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		camera->make_current();
	}

	_update_avatar_visibility();
	emit_signal(SNAME("camera_mode_changed"), (int)view_mode);
}

void CharacterController3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_physics_process_internal(true);
				set_process_input(true);
				set_process_unhandled_input(true);

				if (auto_capture_mouse) {
					Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
				}

				if (active_camera && camera) {
					camera->make_current();
				}
			}
		} break;

		case NOTIFICATION_READY: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				if (auto_capture_mouse) {
					Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
				}
				if (active_camera && camera) {
					camera->make_current();
				}
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_physics_process_internal(false);
				set_process_input(false);
				set_process_unhandled_input(false);
			}
		} break;

		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				real_t delta = get_physics_process_delta_time();
				_handle_mouse_look(delta);
				_handle_movement_physics(delta);
				_update_telemetry_and_state();
			}
		} break;
	}
}

void CharacterController3D::input(const Ref<InputEvent> &p_event) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			mouse_delta_accum += mm->get_relative();
		}
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed()) {
		if (auto_capture_mouse && Input::get_singleton()->get_mouse_mode() != Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
		}
	}
}

void CharacterController3D::unhandled_input(const Ref<InputEvent> &p_event) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed()) {
		// Dynamic Wheel Zoom Switching
		if (view_mode == VIEW_DYNAMIC_TOGGLE || view_mode == VIEW_THIRD_PERSON) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP) {
				real_t new_dist = third_person_distance - 0.5;
				if (new_dist < 0.5 && view_mode == VIEW_DYNAMIC_TOGGLE) {
					set_view_mode(VIEW_FIRST_PERSON);
				} else {
					set_third_person_distance(MAX(0.5, new_dist));
				}
			} else if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				if (view_mode == VIEW_FIRST_PERSON && view_mode == VIEW_DYNAMIC_TOGGLE) {
					set_view_mode(VIEW_THIRD_PERSON);
					set_third_person_distance(1.5);
				} else {
					set_third_person_distance(MIN(12.0, third_person_distance + 0.5));
				}
			}
		}
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		// Escape releases mouse
		if (k->get_keycode() == Key::ESCAPE) {
			if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
				Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			}
			return;
		}

		// View Mode Toggle Shortcut (V key or Action)
		bool toggle_pressed = false;
		if (InputMap::get_singleton()->has_action(action_toggle_view)) {
			toggle_pressed = Input::get_singleton()->is_action_just_pressed(action_toggle_view);
		} else {
			toggle_pressed = (k->get_keycode() == Key::V && !k->is_echo());
		}

		if (toggle_pressed) {
			if (view_mode == VIEW_FIRST_PERSON) {
				set_view_mode(VIEW_THIRD_PERSON);
			} else {
				set_view_mode(VIEW_FIRST_PERSON);
			}
		}
	}
}

void CharacterController3D::_handle_mouse_look(real_t p_delta) {
	if (mouse_delta_accum.is_zero_approx()) {
		return;
	}

	Vector2 delta = mouse_delta_accum;
	mouse_delta_accum = Vector2();

	if (mouse_smoothing > 0.0) {
		delta = delta.lerp(Vector2(), mouse_smoothing);
	}

	real_t sens_x = mouse_sensitivity_x;
	real_t sens_y = mouse_sensitivity_y * (invert_mouse_y ? -1.0 : 1.0);

	real_t yaw_delta = -delta.x * sens_x;
	real_t pitch_delta = -delta.y * sens_y;

	// Update pitch with clamping
	camera_pitch = CLAMP(camera_pitch + pitch_delta, Math::deg_to_rad(min_pitch_angle), Math::deg_to_rad(max_pitch_angle));

	if (view_mode == VIEW_FIRST_PERSON || facing_mode == FACING_CAMERA_DIRECTION) {
		// Rotate character body for yaw in FPS / Strafe mode
		rotate_y(yaw_delta);
		if (head_pivot) {
			head_pivot->set_rotation(Vector3(camera_pitch, 0, 0));
		}
	} else {
		// In RPG/Third-Person mode, rotate the head/camera pivot around the character
		camera_yaw += yaw_delta;
		if (head_pivot) {
			head_pivot->set_rotation(Vector3(camera_pitch, camera_yaw, 0));
		}
	}
}

void CharacterController3D::_handle_movement_physics(real_t p_delta) {
	InputMap *im = InputMap::get_singleton();
	Input *in = Input::get_singleton();

	// 1. Gather Raw Input Vector
	Vector2 raw_in = Vector2();

	// Forward / Back
	if (im->has_action(action_move_forward) && in->is_action_pressed(action_move_forward)) {
		raw_in.y += 1.0;
	} else if (in->is_key_pressed(Key::W)) {
		raw_in.y += 1.0;
	}

	if (im->has_action(action_move_back) && in->is_action_pressed(action_move_back)) {
		raw_in.y -= 1.0;
	} else if (in->is_key_pressed(Key::S)) {
		raw_in.y -= 1.0;
	}

	// Left / Right
	if (im->has_action(action_move_left) && in->is_action_pressed(action_move_left)) {
		raw_in.x -= 1.0;
	} else if (in->is_key_pressed(Key::A)) {
		raw_in.x -= 1.0;
	}

	if (im->has_action(action_move_right) && in->is_action_pressed(action_move_right)) {
		raw_in.x += 1.0;
	} else if (in->is_key_pressed(Key::D)) {
		raw_in.x += 1.0;
	}

	// Gamepad Joypad Stick Input Fallback
	if (im->has_action(action_move_left) && im->has_action(action_move_right) &&
			im->has_action(action_move_back) && im->has_action(action_move_forward)) {
		Vector2 joy_in = in->get_vector(action_move_left, action_move_right, action_move_back, action_move_forward);
		if (joy_in.length_squared() > 0.05) {
			raw_in = joy_in;
		}
	} else {
		real_t joy_x = in->get_joy_axis(0, JoyAxis::LEFT_X);
		real_t joy_y = -in->get_joy_axis(0, JoyAxis::LEFT_Y);
		if (joy_x * joy_x + joy_y * joy_y > 0.04) {
			raw_in = Vector2(joy_x, joy_y);
		}
	}

	if (raw_in.length_squared() > 1.0) {
		raw_in.normalize();
	}
	input_vector_raw = raw_in;

	// 2. Sprint & Crouch Inputs
	bool sprint_pressed = false;
	if (im->has_action(action_sprint)) {
		sprint_pressed = in->is_action_pressed(action_sprint);
	}
	if (!sprint_pressed) {
		sprint_pressed = in->is_key_pressed(Key::SHIFT);
	}

	bool crouch_pressed = false;
	if (im->has_action(action_crouch)) {
		crouch_pressed = in->is_action_pressed(action_crouch);
	}
	if (!crouch_pressed) {
		crouch_pressed = in->is_key_pressed(Key::CTRL) || in->is_key_pressed(Key::C);
	}

	bool jump_just_pressed = false;
	if (im->has_action(action_jump)) {
		jump_just_pressed = in->is_action_just_pressed(action_jump);
	}
	if (!jump_just_pressed) {
		jump_just_pressed = in->is_key_pressed(Key::SPACE);
	}

	bool jump_released = false;
	if (im->has_action(action_jump)) {
		jump_released = in->is_action_just_released(action_jump);
	}

	// 3. Crouch Transition & Height Interpolation
	if (crouch_pressed) {
		is_crouching_active = true;
	} else if (is_crouching_active) {
		if (_can_uncrouch()) {
			is_crouching_active = false;
		}
	}

	target_capsule_height = is_crouching_active ? crouch_height : standing_height;
	target_eye_height = is_crouching_active ? crouch_eye_height : eye_height;

	current_capsule_height = Math::lerp(current_capsule_height, target_capsule_height, (real_t)CLAMP(crouch_transition_speed * p_delta, 0.0, 1.0));
	current_eye_height = Math::lerp(current_eye_height, target_eye_height, (real_t)CLAMP(crouch_transition_speed * p_delta, 0.0, 1.0));

	if (internal_capsule_shape.is_valid()) {
		internal_capsule_shape->set_height(current_capsule_height);
	}
	if (internal_collision_shape) {
		internal_collision_shape->set_position(Vector3(0, current_capsule_height * 0.5, 0));
	}
	if (default_capsule_mesh) {
		Ref<CapsuleMesh> cm = default_capsule_mesh->get_mesh();
		if (cm.is_valid()) {
			cm->set_height(current_capsule_height);
			default_capsule_mesh->set_position(Vector3(0, current_capsule_height * 0.5, 0));
		}
	}
	if (head_pivot) {
		head_pivot->set_position(Vector3(0, current_eye_height, 0));
	}

	// 4. Determine Active Speed
	is_sprinting_active = !is_crouching_active && sprint_pressed && (input_vector_raw.y > 0.1);
	real_t target_speed = walk_speed;
	if (is_crouching_active) {
		target_speed = crouch_speed;
	} else if (is_sprinting_active) {
		target_speed = sprint_speed;
	}

	// 5. Jump & Ground Physics
	bool on_floor = is_on_floor();
	if (on_floor) {
		coyote_timer = coyote_time;
		remaining_air_jumps = max_air_jumps;
		time_in_air = 0.0;

		if (!is_grounded_cached) {
			// Landed event
			emit_signal(SNAME("landed"), last_fall_velocity);
			landing_recovery_timer = 0.15;
		}
	} else {
		coyote_timer = MAX(0.0, coyote_timer - p_delta);
		time_in_air += p_delta;
		last_fall_velocity = -get_velocity().y;
	}
	is_grounded_cached = on_floor;

	if (jump_just_pressed) {
		jump_buffer_timer = jump_buffer_time;
	} else {
		jump_buffer_timer = MAX(0.0, jump_buffer_timer - p_delta);
	}

	Vector3 vel = get_velocity();

	// Jump Execution
	real_t grav = ProjectSettings::get_singleton()->get_setting("physics/3d/default_gravity", 9.8);
	grav *= gravity_scale;

	if (jump_buffer_timer > 0.0) {
		if (on_floor || coyote_timer > 0.0) {
			vel.y = Math::sqrt(2.0 * grav * jump_height);
			jump_buffer_timer = 0.0;
			coyote_timer = 0.0;
			emit_signal(SNAME("jumped"));
		} else if (remaining_air_jumps > 0) {
			vel.y = Math::sqrt(2.0 * grav * jump_height);
			remaining_air_jumps--;
			jump_buffer_timer = 0.0;
			emit_signal(SNAME("jumped"));
		}
	}

	// Variable Jump Cut
	if (variable_jump_height && jump_released && vel.y > 0.0) {
		vel.y *= 0.5;
	}

	// Apply Gravity
	if (!on_floor) {
		real_t g_mult = (vel.y < 0.0) ? fall_gravity_multiplier : 1.0;
		vel.y -= grav * g_mult * p_delta;
	}

	// 6. Direction Calculation (Camera-Relative)
	Vector3 forward;
	Vector3 right;

	if (camera) {
		Transform3D cam_gt = camera->get_global_transform();
		forward = -cam_gt.basis.get_column(Vector3::AXIS_Z);
		forward.y = 0.0;
		if (forward.length_squared() > 0.001) {
			forward.normalize();
		}

		right = cam_gt.basis.get_column(Vector3::AXIS_X);
		right.y = 0.0;
		if (right.length_squared() > 0.001) {
			right.normalize();
		}
	} else {
		forward = -get_global_transform().basis.get_column(Vector3::AXIS_Z);
		right = get_global_transform().basis.get_column(Vector3::AXIS_X);
	}

	Vector3 move_dir = (forward * input_vector_raw.y + right * input_vector_raw.x);
	if (move_dir.length_squared() > 1.0) {
		move_dir.normalize();
	}

	// 7. Horizontal Acceleration / Friction
	Vector3 h_vel = Vector3(vel.x, 0, vel.z);
	Vector3 target_h_vel = move_dir * target_speed;

	if (on_floor) {
		if (move_dir.length_squared() > 0.001) {
			h_vel = h_vel.move_toward(target_h_vel, ground_acceleration * p_delta);
		} else {
			h_vel = h_vel.move_toward(Vector3(), ground_deceleration * p_delta);
		}
	} else {
		// Air control
		if (move_dir.length_squared() > 0.001) {
			Vector3 air_target = target_h_vel * air_control_multiplier;
			h_vel = h_vel.move_toward(air_target, air_acceleration * p_delta);
		}
		h_vel = h_vel.move_toward(Vector3(), air_drag * p_delta);
	}

	vel.x = h_vel.x;
	vel.z = h_vel.z;
	set_velocity(vel);

	// 8. Stair Stepping
	if (stair_stepping_enabled && on_floor && move_dir.length_squared() > 0.05) {
		_handle_stair_stepping(p_delta, move_dir);
	}

	// 9. Move and Slide
	move_and_slide();

	// 10. Avatar Rotation in Third-Person Movement Mode
	if (facing_mode == FACING_MOVEMENT_DIRECTION && view_mode != VIEW_FIRST_PERSON) {
		if (move_dir.length_squared() > 0.05 && avatar_root) {
			real_t target_angle = Math::atan2(-move_dir.x, -move_dir.z);
			real_t current_angle = avatar_root->get_rotation().y;
			avatar_root->set_rotation(Vector3(0, Math::lerp_angle(current_angle, target_angle, (real_t)CLAMP(rotation_smooth_rate * p_delta, 0.0, 1.0)), 0));
		}
	} else if (avatar_root) {
		avatar_root->set_rotation(Vector3(0, 0, 0));
	}

	// 11. Camera Polish (Bob, FOV Kick, Roll)
	real_t speed_ratio = (target_speed > 0.0) ? (h_vel.length() / target_speed) : 0.0;
	_apply_camera_polish(p_delta, speed_ratio);
}

void CharacterController3D::_handle_stair_stepping(real_t p_delta, const Vector3 &p_move_dir) {
	PhysicsDirectSpaceState3D *space_state = get_world_3d()->get_direct_space_state();
	if (!space_state) {
		return;
	}

	Vector3 base_pos = get_global_position();
	Vector3 step_forward = p_move_dir.normalized() * step_forward_distance;

	// 1. Forward test at knee height (probe obstacle)
	PS3DT::RayParameters ray_forward;
	ray_forward.from = base_pos + Vector3(0, 0.1, 0);
	ray_forward.to = base_pos + step_forward + Vector3(0, 0.1, 0);
	ray_forward.exclude.insert(get_rid());

	PS3DT::RayResult res_forward;
	if (space_state->intersect_ray(ray_forward, res_forward)) {
		// 2. Downward probe from max step height to find step surface
		PS3DT::RayParameters ray_down;
		ray_down.from = base_pos + step_forward + Vector3(0, max_step_height + 0.05, 0);
		ray_down.to = base_pos + step_forward + Vector3(0, 0.02, 0);
		ray_down.exclude.insert(get_rid());

		PS3DT::RayResult res_down;
		if (space_state->intersect_ray(ray_down, res_down)) {
			real_t step_height = res_down.position.y - base_pos.y;
			if (step_height > 0.03 && step_height <= max_step_height) {
				// Check walkable surface angle
				real_t angle = res_down.normal.angle_to(Vector3(0, 1, 0));
				if (angle <= get_floor_max_angle()) {
					// Smoothly step up
					Vector3 pos = get_global_position();
					pos.y = res_down.position.y;
					set_global_position(pos);
				}
			}
		}
	}
}

bool CharacterController3D::_can_uncrouch() const {
	PhysicsDirectSpaceState3D *space_state = get_world_3d()->get_direct_space_state();
	if (!space_state) {
		return true;
	}

	PS3DT::RayParameters ray;
	ray.from = get_global_position() + Vector3(0, crouch_height * 0.5, 0);
	ray.to = get_global_position() + Vector3(0, standing_height + 0.05, 0);
	ray.exclude.insert(get_rid());

	PS3DT::RayResult res;
	return !space_state->intersect_ray(ray, res);
}

void CharacterController3D::_apply_camera_polish(real_t p_delta, real_t p_speed_ratio) {
	if (!camera) {
		return;
	}

	// 1. Sprint FOV Kick
	if (sprint_fov_kick_enabled) {
		real_t base_fov = (view_mode == VIEW_FIRST_PERSON) ? first_person_fov : third_person_fov;
		real_t target_fov = base_fov + (is_sprinting_active ? sprint_fov_kick_amount : 0.0);
		camera->set_fov(Math::lerp(camera->get_fov(), target_fov, (real_t)CLAMP(8.0 * p_delta, 0.0, 1.0)));
	}

	// 2. Strafe Roll Tilt
	if (strafe_roll_enabled && view_mode == VIEW_FIRST_PERSON) {
		real_t target_roll = -input_vector_raw.x * Math::deg_to_rad(strafe_roll_max_angle);
		Vector3 cam_rot = camera->get_rotation();
		cam_rot.z = Math::lerp(cam_rot.z, target_roll, (real_t)CLAMP(10.0 * p_delta, 0.0, 1.0));
		camera->set_rotation(cam_rot);
	}

	// 3. Head Bobbing & Footstep Signal
	if (is_on_floor() && p_speed_ratio > 0.05) {
		head_bob_cycle += p_delta * head_bob_frequency * (is_sprinting_active ? 1.4 : 1.0) * p_speed_ratio * Math::TAU;
		footstep_cycle_accum += p_delta * head_bob_frequency * (is_sprinting_active ? 1.4 : 1.0) * p_speed_ratio;

		if (footstep_cycle_accum >= 0.5) {
			footstep_cycle_accum -= 0.5;
			last_foot_index = (last_foot_index == 0) ? 1 : 0;
			emit_signal(SNAME("footstep"), last_foot_index, get_global_position());
		}

		if (head_bob_enabled && view_mode == VIEW_FIRST_PERSON) {
			real_t bob_h = Math::sin(head_bob_cycle * 0.5) * head_bob_amplitude_h;
			real_t bob_v = Math::abs(Math::sin(head_bob_cycle)) * head_bob_amplitude_v;
			camera->set_position(Vector3(bob_h, bob_v, 0));
		}
	} else {
		if (head_bob_enabled && view_mode == VIEW_FIRST_PERSON) {
			camera->set_position(camera->get_position().lerp(Vector3(), (real_t)CLAMP(10.0 * p_delta, 0.0, 1.0)));
		}
	}
}

void CharacterController3D::_update_telemetry_and_state() {
	Vector3 vel = get_velocity();
	real_t h_speed = Vector2(vel.x, vel.z).length();

	LocomotionState new_state = STATE_IDLE;
	if (!is_on_floor()) {
		if (vel.y > 0.5) {
			new_state = STATE_JUMP_RISE;
		} else {
			new_state = STATE_AIR_FALL;
		}
	} else if (landing_recovery_timer > 0.0) {
		new_state = STATE_LAND;
	} else if (is_crouching_active) {
		new_state = STATE_CROUCH;
	} else if (h_speed > 0.1) {
		new_state = is_sprinting_active ? STATE_SPRINT : STATE_WALK;
	}

	if (new_state != current_state) {
		current_state = new_state;
		emit_signal(SNAME("locomotion_state_changed"), (int)current_state);
	}
}

// ---------------- Getters & Setters ----------------

void CharacterController3D::set_view_mode(ViewMode p_mode) {
	if (view_mode == p_mode) {
		return;
	}
	view_mode = p_mode;
	_update_camera_mode();
}

CharacterController3D::ViewMode CharacterController3D::get_view_mode() const {
	return view_mode;
}

void CharacterController3D::set_facing_mode(FacingMode p_mode) {
	facing_mode = p_mode;
}

CharacterController3D::FacingMode CharacterController3D::get_facing_mode() const {
	return facing_mode;
}

void CharacterController3D::set_eye_height(real_t p_height) {
	eye_height = MAX(0.1, p_height);
	if (head_pivot && !is_crouching_active) {
		head_pivot->set_position(Vector3(0, eye_height, 0));
	}
}

real_t CharacterController3D::get_eye_height() const {
	return eye_height;
}

void CharacterController3D::set_crouch_eye_height(real_t p_height) {
	crouch_eye_height = MAX(0.1, p_height);
}

real_t CharacterController3D::get_crouch_eye_height() const {
	return crouch_eye_height;
}

void CharacterController3D::set_mouse_sensitivity_x(real_t p_val) {
	mouse_sensitivity_x = p_val;
}

real_t CharacterController3D::get_mouse_sensitivity_x() const {
	return mouse_sensitivity_x;
}

void CharacterController3D::set_mouse_sensitivity_y(real_t p_val) {
	mouse_sensitivity_y = p_val;
}

real_t CharacterController3D::get_mouse_sensitivity_y() const {
	return mouse_sensitivity_y;
}

void CharacterController3D::set_mouse_smoothing(real_t p_val) {
	mouse_smoothing = CLAMP(p_val, (real_t)0.0, (real_t)0.95);
}

real_t CharacterController3D::get_mouse_smoothing() const {
	return mouse_smoothing;
}

void CharacterController3D::set_invert_mouse_y(bool p_invert) {
	invert_mouse_y = p_invert;
}

bool CharacterController3D::is_mouse_y_inverted() const {
	return invert_mouse_y;
}

void CharacterController3D::set_min_pitch_angle(real_t p_angle) {
	min_pitch_angle = p_angle;
}

real_t CharacterController3D::get_min_pitch_angle() const {
	return min_pitch_angle;
}

void CharacterController3D::set_max_pitch_angle(real_t p_angle) {
	max_pitch_angle = p_angle;
}

real_t CharacterController3D::get_max_pitch_angle() const {
	return max_pitch_angle;
}

void CharacterController3D::set_third_person_distance(real_t p_distance) {
	third_person_distance = MAX(0.1, p_distance);
	if (spring_arm) {
		spring_arm->set_length(third_person_distance);
	}
}

real_t CharacterController3D::get_third_person_distance() const {
	return third_person_distance;
}

void CharacterController3D::set_third_person_offset(const Vector3 &p_offset) {
	third_person_offset = p_offset;
	if (spring_arm) {
		spring_arm->set_position(third_person_offset);
	}
}

Vector3 CharacterController3D::get_third_person_offset() const {
	return third_person_offset;
}

void CharacterController3D::set_first_person_fov(real_t p_fov) {
	first_person_fov = p_fov;
	if (camera && view_mode == VIEW_FIRST_PERSON) {
		camera->set_fov(first_person_fov);
	}
}

real_t CharacterController3D::get_first_person_fov() const {
	return first_person_fov;
}

void CharacterController3D::set_third_person_fov(real_t p_fov) {
	third_person_fov = p_fov;
	if (camera && view_mode != VIEW_FIRST_PERSON) {
		camera->set_fov(third_person_fov);
	}
}

real_t CharacterController3D::get_third_person_fov() const {
	return third_person_fov;
}

void CharacterController3D::set_auto_capture_mouse(bool p_capture) {
	auto_capture_mouse = p_capture;
}

bool CharacterController3D::is_auto_capture_mouse_enabled() const {
	return auto_capture_mouse;
}

void CharacterController3D::set_active_camera(bool p_active) {
	active_camera = p_active;
	if (camera && is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		if (active_camera) {
			camera->make_current();
		} else {
			camera->clear_current(true);
		}
	}
}

bool CharacterController3D::is_active_camera() const {
	return active_camera;
}

// Camera Polish
void CharacterController3D::set_head_bob_enabled(bool p_enabled) {
	head_bob_enabled = p_enabled;
}

bool CharacterController3D::is_head_bob_enabled() const {
	return head_bob_enabled;
}

void CharacterController3D::set_head_bob_frequency(real_t p_val) {
	head_bob_frequency = p_val;
}

real_t CharacterController3D::get_head_bob_frequency() const {
	return head_bob_frequency;
}

void CharacterController3D::set_head_bob_amplitude_h(real_t p_val) {
	head_bob_amplitude_h = p_val;
}

real_t CharacterController3D::get_head_bob_amplitude_h() const {
	return head_bob_amplitude_h;
}

void CharacterController3D::set_head_bob_amplitude_v(real_t p_val) {
	head_bob_amplitude_v = p_val;
}

real_t CharacterController3D::get_head_bob_amplitude_v() const {
	return head_bob_amplitude_v;
}

void CharacterController3D::set_sprint_fov_kick_enabled(bool p_enabled) {
	sprint_fov_kick_enabled = p_enabled;
}

bool CharacterController3D::is_sprint_fov_kick_enabled() const {
	return sprint_fov_kick_enabled;
}

void CharacterController3D::set_sprint_fov_kick_amount(real_t p_val) {
	sprint_fov_kick_amount = p_val;
}

real_t CharacterController3D::get_sprint_fov_kick_amount() const {
	return sprint_fov_kick_amount;
}

void CharacterController3D::set_strafe_roll_enabled(bool p_enabled) {
	strafe_roll_enabled = p_enabled;
}

bool CharacterController3D::is_strafe_roll_enabled() const {
	return strafe_roll_enabled;
}

void CharacterController3D::set_strafe_roll_max_angle(real_t p_val) {
	strafe_roll_max_angle = p_val;
}

real_t CharacterController3D::get_strafe_roll_max_angle() const {
	return strafe_roll_max_angle;
}

// Movement
void CharacterController3D::set_walk_speed(real_t p_speed) {
	walk_speed = MAX(0.1, p_speed);
}

real_t CharacterController3D::get_walk_speed() const {
	return walk_speed;
}

void CharacterController3D::set_sprint_speed(real_t p_speed) {
	sprint_speed = MAX(0.1, p_speed);
}

real_t CharacterController3D::get_sprint_speed() const {
	return sprint_speed;
}

void CharacterController3D::set_crouch_speed(real_t p_speed) {
	crouch_speed = MAX(0.1, p_speed);
}

real_t CharacterController3D::get_crouch_speed() const {
	return crouch_speed;
}

void CharacterController3D::set_ground_acceleration(real_t p_accel) {
	ground_acceleration = MAX(0.1, p_accel);
}

real_t CharacterController3D::get_ground_acceleration() const {
	return ground_acceleration;
}

void CharacterController3D::set_ground_deceleration(real_t p_decel) {
	ground_deceleration = MAX(0.1, p_decel);
}

real_t CharacterController3D::get_ground_deceleration() const {
	return ground_deceleration;
}

void CharacterController3D::set_air_acceleration(real_t p_accel) {
	air_acceleration = p_accel;
}

real_t CharacterController3D::get_air_acceleration() const {
	return air_acceleration;
}

void CharacterController3D::set_air_control_multiplier(real_t p_mult) {
	air_control_multiplier = CLAMP(p_mult, (real_t)0.0, (real_t)1.0);
}

real_t CharacterController3D::get_air_control_multiplier() const {
	return air_control_multiplier;
}

void CharacterController3D::set_air_drag(real_t p_drag) {
	air_drag = p_drag;
}

real_t CharacterController3D::get_air_drag() const {
	return air_drag;
}

void CharacterController3D::set_rotation_smooth_rate(real_t p_rate) {
	rotation_smooth_rate = p_rate;
}

real_t CharacterController3D::get_rotation_smooth_rate() const {
	return rotation_smooth_rate;
}

// Jump & Gravity
void CharacterController3D::set_jump_height(real_t p_height) {
	jump_height = MAX(0.0, p_height);
}

real_t CharacterController3D::get_jump_height() const {
	return jump_height;
}

void CharacterController3D::set_max_air_jumps(int p_count) {
	max_air_jumps = MAX(0, p_count);
}

int CharacterController3D::get_max_air_jumps() const {
	return max_air_jumps;
}

void CharacterController3D::set_coyote_time(real_t p_time) {
	coyote_time = MAX(0.0, p_time);
}

real_t CharacterController3D::get_coyote_time() const {
	return coyote_time;
}

void CharacterController3D::set_jump_buffer_time(real_t p_time) {
	jump_buffer_time = MAX(0.0, p_time);
}

real_t CharacterController3D::get_jump_buffer_time() const {
	return jump_buffer_time;
}

void CharacterController3D::set_variable_jump_height(bool p_enabled) {
	variable_jump_height = p_enabled;
}

bool CharacterController3D::is_variable_jump_height_enabled() const {
	return variable_jump_height;
}

void CharacterController3D::set_gravity_scale(real_t p_scale) {
	gravity_scale = p_scale;
}

real_t CharacterController3D::get_gravity_scale() const {
	return gravity_scale;
}

void CharacterController3D::set_fall_gravity_multiplier(real_t p_mult) {
	fall_gravity_multiplier = p_mult;
}

real_t CharacterController3D::get_fall_gravity_multiplier() const {
	return fall_gravity_multiplier;
}

// Crouch & Dimensions
void CharacterController3D::set_standing_height(real_t p_height) {
	standing_height = MAX(0.5, p_height);
	if (!is_crouching_active) {
		current_capsule_height = standing_height;
		_update_capsule_mesh();
	}
}

real_t CharacterController3D::get_standing_height() const {
	return standing_height;
}

void CharacterController3D::set_crouch_height(real_t p_height) {
	crouch_height = MAX(0.3, p_height);
}

real_t CharacterController3D::get_crouch_height() const {
	return crouch_height;
}

void CharacterController3D::set_capsule_radius(real_t p_radius) {
	capsule_radius = MAX(0.1, p_radius);
	if (internal_capsule_shape.is_valid()) {
		internal_capsule_shape->set_radius(capsule_radius);
	}
	_update_capsule_mesh();
}

real_t CharacterController3D::get_capsule_radius() const {
	return capsule_radius;
}

void CharacterController3D::set_crouch_transition_speed(real_t p_speed) {
	crouch_transition_speed = p_speed;
}

real_t CharacterController3D::get_crouch_transition_speed() const {
	return crouch_transition_speed;
}

// Stairs & Slopes
void CharacterController3D::set_stair_stepping_enabled(bool p_enabled) {
	stair_stepping_enabled = p_enabled;
}

bool CharacterController3D::is_stair_stepping_enabled() const {
	return stair_stepping_enabled;
}

void CharacterController3D::set_max_step_height(real_t p_height) {
	max_step_height = MAX(0.05, p_height);
}

real_t CharacterController3D::get_max_step_height() const {
	return max_step_height;
}

void CharacterController3D::set_step_forward_distance(real_t p_dist) {
	step_forward_distance = MAX(0.1, p_dist);
}

real_t CharacterController3D::get_step_forward_distance() const {
	return step_forward_distance;
}

void CharacterController3D::set_slide_on_steep_slopes(bool p_slide) {
	slide_on_steep_slopes = p_slide;
}

bool CharacterController3D::is_slide_on_steep_slopes_enabled() const {
	return slide_on_steep_slopes;
}

// Avatar Ingestion
void CharacterController3D::set_custom_avatar_scene(const Ref<PackedScene> &p_scene) {
	custom_avatar_scene = p_scene;
	_update_avatar_instance();
}

Ref<PackedScene> CharacterController3D::get_custom_avatar_scene() const {
	return custom_avatar_scene;
}

void CharacterController3D::set_avatar_offset(const Vector3 &p_offset) {
	avatar_offset = p_offset;
	if (custom_avatar_instance) {
		custom_avatar_instance->set_position(avatar_offset);
	}
}

Vector3 CharacterController3D::get_avatar_offset() const {
	return avatar_offset;
}

void CharacterController3D::set_avatar_rotation_offset(const Vector3 &p_rot_deg) {
	avatar_rotation_offset = p_rot_deg;
	if (custom_avatar_instance) {
		custom_avatar_instance->set_rotation_degrees(avatar_rotation_offset);
	}
}

Vector3 CharacterController3D::get_avatar_rotation_offset() const {
	return avatar_rotation_offset;
}

void CharacterController3D::set_first_person_avatar_visibility(AvatarVisibility p_visibility) {
	first_person_avatar_visibility = p_visibility;
	_update_avatar_visibility();
}

CharacterController3D::AvatarVisibility CharacterController3D::get_first_person_avatar_visibility() const {
	return first_person_avatar_visibility;
}

void CharacterController3D::set_show_capsule_in_editor(bool p_show) {
	show_capsule_in_editor = p_show;
	if (Engine::get_singleton()->is_editor_hint()) {
		if (default_capsule_mesh) {
			default_capsule_mesh->set_visible(show_capsule_in_editor);
		}
	}
}

bool CharacterController3D::is_show_capsule_in_editor_enabled() const {
	return show_capsule_in_editor;
}

// Actions
void CharacterController3D::set_action_move_forward(const StringName &p_name) {
	action_move_forward = p_name;
}

StringName CharacterController3D::get_action_move_forward() const {
	return action_move_forward;
}

void CharacterController3D::set_action_move_back(const StringName &p_name) {
	action_move_back = p_name;
}

StringName CharacterController3D::get_action_move_back() const {
	return action_move_back;
}

void CharacterController3D::set_action_move_left(const StringName &p_name) {
	action_move_left = p_name;
}

StringName CharacterController3D::get_action_move_left() const {
	return action_move_left;
}

void CharacterController3D::set_action_move_right(const StringName &p_name) {
	action_move_right = p_name;
}

StringName CharacterController3D::get_action_move_right() const {
	return action_move_right;
}

void CharacterController3D::set_action_jump(const StringName &p_name) {
	action_jump = p_name;
}

StringName CharacterController3D::get_action_jump() const {
	return action_jump;
}

void CharacterController3D::set_action_sprint(const StringName &p_name) {
	action_sprint = p_name;
}

StringName CharacterController3D::get_action_sprint() const {
	return action_sprint;
}

void CharacterController3D::set_action_crouch(const StringName &p_name) {
	action_crouch = p_name;
}

StringName CharacterController3D::get_action_crouch() const {
	return action_crouch;
}

void CharacterController3D::set_action_toggle_view(const StringName &p_name) {
	action_toggle_view = p_name;
}

StringName CharacterController3D::get_action_toggle_view() const {
	return action_toggle_view;
}

// Telemetry API
CharacterController3D::LocomotionState CharacterController3D::get_locomotion_state() const {
	return current_state;
}

Vector2 CharacterController3D::get_movement_vector_2d() const {
	return input_vector_raw;
}

real_t CharacterController3D::get_horizontal_speed() const {
	Vector3 vel = get_velocity();
	return Vector2(vel.x, vel.z).length();
}

real_t CharacterController3D::get_turn_rate() const {
	return Math::rad_to_deg(avatar_yaw);
}

bool CharacterController3D::is_sprinting() const {
	return is_sprinting_active;
}

bool CharacterController3D::is_crouching() const {
	return is_crouching_active;
}

real_t CharacterController3D::get_time_in_air() const {
	return time_in_air;
}

Node3D *CharacterController3D::get_avatar_root() const {
	return avatar_root;
}

Node3D *CharacterController3D::get_head_pivot() const {
	return head_pivot;
}

SpringArm3D *CharacterController3D::get_spring_arm() const {
	return spring_arm;
}

Camera3D *CharacterController3D::get_camera() const {
	return camera;
}

void CharacterController3D::_bind_methods() {
	// View & Camera
	ClassDB::bind_method(D_METHOD("set_view_mode", "mode"), &CharacterController3D::set_view_mode);
	ClassDB::bind_method(D_METHOD("get_view_mode"), &CharacterController3D::get_view_mode);
	ClassDB::bind_method(D_METHOD("set_facing_mode", "mode"), &CharacterController3D::set_facing_mode);
	ClassDB::bind_method(D_METHOD("get_facing_mode"), &CharacterController3D::get_facing_mode);
	ClassDB::bind_method(D_METHOD("set_eye_height", "height"), &CharacterController3D::set_eye_height);
	ClassDB::bind_method(D_METHOD("get_eye_height"), &CharacterController3D::get_eye_height);
	ClassDB::bind_method(D_METHOD("set_crouch_eye_height", "height"), &CharacterController3D::set_crouch_eye_height);
	ClassDB::bind_method(D_METHOD("get_crouch_eye_height"), &CharacterController3D::get_crouch_eye_height);
	ClassDB::bind_method(D_METHOD("set_mouse_sensitivity_x", "sensitivity"), &CharacterController3D::set_mouse_sensitivity_x);
	ClassDB::bind_method(D_METHOD("get_mouse_sensitivity_x"), &CharacterController3D::get_mouse_sensitivity_x);
	ClassDB::bind_method(D_METHOD("set_mouse_sensitivity_y", "sensitivity"), &CharacterController3D::set_mouse_sensitivity_y);
	ClassDB::bind_method(D_METHOD("get_mouse_sensitivity_y"), &CharacterController3D::get_mouse_sensitivity_y);
	ClassDB::bind_method(D_METHOD("set_mouse_smoothing", "smoothing"), &CharacterController3D::set_mouse_smoothing);
	ClassDB::bind_method(D_METHOD("get_mouse_smoothing"), &CharacterController3D::get_mouse_smoothing);
	ClassDB::bind_method(D_METHOD("set_invert_mouse_y", "invert"), &CharacterController3D::set_invert_mouse_y);
	ClassDB::bind_method(D_METHOD("is_mouse_y_inverted"), &CharacterController3D::is_mouse_y_inverted);
	ClassDB::bind_method(D_METHOD("set_min_pitch_angle", "angle"), &CharacterController3D::set_min_pitch_angle);
	ClassDB::bind_method(D_METHOD("get_min_pitch_angle"), &CharacterController3D::get_min_pitch_angle);
	ClassDB::bind_method(D_METHOD("set_max_pitch_angle", "angle"), &CharacterController3D::set_max_pitch_angle);
	ClassDB::bind_method(D_METHOD("get_max_pitch_angle"), &CharacterController3D::get_max_pitch_angle);
	ClassDB::bind_method(D_METHOD("set_third_person_distance", "distance"), &CharacterController3D::set_third_person_distance);
	ClassDB::bind_method(D_METHOD("get_third_person_distance"), &CharacterController3D::get_third_person_distance);
	ClassDB::bind_method(D_METHOD("set_third_person_offset", "offset"), &CharacterController3D::set_third_person_offset);
	ClassDB::bind_method(D_METHOD("get_third_person_offset"), &CharacterController3D::get_third_person_offset);
	ClassDB::bind_method(D_METHOD("set_first_person_fov", "fov"), &CharacterController3D::set_first_person_fov);
	ClassDB::bind_method(D_METHOD("get_first_person_fov"), &CharacterController3D::get_first_person_fov);
	ClassDB::bind_method(D_METHOD("set_third_person_fov", "fov"), &CharacterController3D::set_third_person_fov);
	ClassDB::bind_method(D_METHOD("get_third_person_fov"), &CharacterController3D::get_third_person_fov);
	ClassDB::bind_method(D_METHOD("set_auto_capture_mouse", "capture"), &CharacterController3D::set_auto_capture_mouse);
	ClassDB::bind_method(D_METHOD("is_auto_capture_mouse_enabled"), &CharacterController3D::is_auto_capture_mouse_enabled);
	ClassDB::bind_method(D_METHOD("set_active_camera", "active"), &CharacterController3D::set_active_camera);
	ClassDB::bind_method(D_METHOD("is_active_camera"), &CharacterController3D::is_active_camera);

	// Camera Polish
	ClassDB::bind_method(D_METHOD("set_head_bob_enabled", "enabled"), &CharacterController3D::set_head_bob_enabled);
	ClassDB::bind_method(D_METHOD("is_head_bob_enabled"), &CharacterController3D::is_head_bob_enabled);
	ClassDB::bind_method(D_METHOD("set_head_bob_frequency", "freq"), &CharacterController3D::set_head_bob_frequency);
	ClassDB::bind_method(D_METHOD("get_head_bob_frequency"), &CharacterController3D::get_head_bob_frequency);
	ClassDB::bind_method(D_METHOD("set_head_bob_amplitude_h", "amp"), &CharacterController3D::set_head_bob_amplitude_h);
	ClassDB::bind_method(D_METHOD("get_head_bob_amplitude_h"), &CharacterController3D::get_head_bob_amplitude_h);
	ClassDB::bind_method(D_METHOD("set_head_bob_amplitude_v", "amp"), &CharacterController3D::set_head_bob_amplitude_v);
	ClassDB::bind_method(D_METHOD("get_head_bob_amplitude_v"), &CharacterController3D::get_head_bob_amplitude_v);
	ClassDB::bind_method(D_METHOD("set_sprint_fov_kick_enabled", "enabled"), &CharacterController3D::set_sprint_fov_kick_enabled);
	ClassDB::bind_method(D_METHOD("is_sprint_fov_kick_enabled"), &CharacterController3D::is_sprint_fov_kick_enabled);
	ClassDB::bind_method(D_METHOD("set_sprint_fov_kick_amount", "amount"), &CharacterController3D::set_sprint_fov_kick_amount);
	ClassDB::bind_method(D_METHOD("get_sprint_fov_kick_amount"), &CharacterController3D::get_sprint_fov_kick_amount);
	ClassDB::bind_method(D_METHOD("set_strafe_roll_enabled", "enabled"), &CharacterController3D::set_strafe_roll_enabled);
	ClassDB::bind_method(D_METHOD("is_strafe_roll_enabled"), &CharacterController3D::is_strafe_roll_enabled);
	ClassDB::bind_method(D_METHOD("set_strafe_roll_max_angle", "angle"), &CharacterController3D::set_strafe_roll_max_angle);
	ClassDB::bind_method(D_METHOD("get_strafe_roll_max_angle"), &CharacterController3D::get_strafe_roll_max_angle);

	// Movement
	ClassDB::bind_method(D_METHOD("set_walk_speed", "speed"), &CharacterController3D::set_walk_speed);
	ClassDB::bind_method(D_METHOD("get_walk_speed"), &CharacterController3D::get_walk_speed);
	ClassDB::bind_method(D_METHOD("set_sprint_speed", "speed"), &CharacterController3D::set_sprint_speed);
	ClassDB::bind_method(D_METHOD("get_sprint_speed"), &CharacterController3D::get_sprint_speed);
	ClassDB::bind_method(D_METHOD("set_crouch_speed", "speed"), &CharacterController3D::set_crouch_speed);
	ClassDB::bind_method(D_METHOD("get_crouch_speed"), &CharacterController3D::get_crouch_speed);
	ClassDB::bind_method(D_METHOD("set_ground_acceleration", "accel"), &CharacterController3D::set_ground_acceleration);
	ClassDB::bind_method(D_METHOD("get_ground_acceleration"), &CharacterController3D::get_ground_acceleration);
	ClassDB::bind_method(D_METHOD("set_ground_deceleration", "decel"), &CharacterController3D::set_ground_deceleration);
	ClassDB::bind_method(D_METHOD("get_ground_deceleration"), &CharacterController3D::get_ground_deceleration);
	ClassDB::bind_method(D_METHOD("set_air_acceleration", "accel"), &CharacterController3D::set_air_acceleration);
	ClassDB::bind_method(D_METHOD("get_air_acceleration"), &CharacterController3D::get_air_acceleration);
	ClassDB::bind_method(D_METHOD("set_air_control_multiplier", "mult"), &CharacterController3D::set_air_control_multiplier);
	ClassDB::bind_method(D_METHOD("get_air_control_multiplier"), &CharacterController3D::get_air_control_multiplier);
	ClassDB::bind_method(D_METHOD("set_air_drag", "drag"), &CharacterController3D::set_air_drag);
	ClassDB::bind_method(D_METHOD("get_air_drag"), &CharacterController3D::get_air_drag);
	ClassDB::bind_method(D_METHOD("set_rotation_smooth_rate", "rate"), &CharacterController3D::set_rotation_smooth_rate);
	ClassDB::bind_method(D_METHOD("get_rotation_smooth_rate"), &CharacterController3D::get_rotation_smooth_rate);

	// Jump & Gravity
	ClassDB::bind_method(D_METHOD("set_jump_height", "height"), &CharacterController3D::set_jump_height);
	ClassDB::bind_method(D_METHOD("get_jump_height"), &CharacterController3D::get_jump_height);
	ClassDB::bind_method(D_METHOD("set_max_air_jumps", "count"), &CharacterController3D::set_max_air_jumps);
	ClassDB::bind_method(D_METHOD("get_max_air_jumps"), &CharacterController3D::get_max_air_jumps);
	ClassDB::bind_method(D_METHOD("set_coyote_time", "time"), &CharacterController3D::set_coyote_time);
	ClassDB::bind_method(D_METHOD("get_coyote_time"), &CharacterController3D::get_coyote_time);
	ClassDB::bind_method(D_METHOD("set_jump_buffer_time", "time"), &CharacterController3D::set_jump_buffer_time);
	ClassDB::bind_method(D_METHOD("get_jump_buffer_time"), &CharacterController3D::get_jump_buffer_time);
	ClassDB::bind_method(D_METHOD("set_variable_jump_height", "enabled"), &CharacterController3D::set_variable_jump_height);
	ClassDB::bind_method(D_METHOD("is_variable_jump_height_enabled"), &CharacterController3D::is_variable_jump_height_enabled);
	ClassDB::bind_method(D_METHOD("set_gravity_scale", "scale"), &CharacterController3D::set_gravity_scale);
	ClassDB::bind_method(D_METHOD("get_gravity_scale"), &CharacterController3D::get_gravity_scale);
	ClassDB::bind_method(D_METHOD("set_fall_gravity_multiplier", "mult"), &CharacterController3D::set_fall_gravity_multiplier);
	ClassDB::bind_method(D_METHOD("get_fall_gravity_multiplier"), &CharacterController3D::get_fall_gravity_multiplier);

	// Crouch & Dimensions
	ClassDB::bind_method(D_METHOD("set_standing_height", "height"), &CharacterController3D::set_standing_height);
	ClassDB::bind_method(D_METHOD("get_standing_height"), &CharacterController3D::get_standing_height);
	ClassDB::bind_method(D_METHOD("set_crouch_height", "height"), &CharacterController3D::set_crouch_height);
	ClassDB::bind_method(D_METHOD("get_crouch_height"), &CharacterController3D::get_crouch_height);
	ClassDB::bind_method(D_METHOD("set_capsule_radius", "radius"), &CharacterController3D::set_capsule_radius);
	ClassDB::bind_method(D_METHOD("get_capsule_radius"), &CharacterController3D::get_capsule_radius);
	ClassDB::bind_method(D_METHOD("set_crouch_transition_speed", "speed"), &CharacterController3D::set_crouch_transition_speed);
	ClassDB::bind_method(D_METHOD("get_crouch_transition_speed"), &CharacterController3D::get_crouch_transition_speed);

	// Stairs & Slopes
	ClassDB::bind_method(D_METHOD("set_stair_stepping_enabled", "enabled"), &CharacterController3D::set_stair_stepping_enabled);
	ClassDB::bind_method(D_METHOD("is_stair_stepping_enabled"), &CharacterController3D::is_stair_stepping_enabled);
	ClassDB::bind_method(D_METHOD("set_max_step_height", "height"), &CharacterController3D::set_max_step_height);
	ClassDB::bind_method(D_METHOD("get_max_step_height"), &CharacterController3D::get_max_step_height);
	ClassDB::bind_method(D_METHOD("set_step_forward_distance", "distance"), &CharacterController3D::set_step_forward_distance);
	ClassDB::bind_method(D_METHOD("get_step_forward_distance"), &CharacterController3D::get_step_forward_distance);
	ClassDB::bind_method(D_METHOD("set_slide_on_steep_slopes", "slide"), &CharacterController3D::set_slide_on_steep_slopes);
	ClassDB::bind_method(D_METHOD("is_slide_on_steep_slopes_enabled"), &CharacterController3D::is_slide_on_steep_slopes_enabled);

	// Avatar Ingestion
	ClassDB::bind_method(D_METHOD("set_custom_avatar_scene", "scene"), &CharacterController3D::set_custom_avatar_scene);
	ClassDB::bind_method(D_METHOD("get_custom_avatar_scene"), &CharacterController3D::get_custom_avatar_scene);
	ClassDB::bind_method(D_METHOD("set_avatar_offset", "offset"), &CharacterController3D::set_avatar_offset);
	ClassDB::bind_method(D_METHOD("get_avatar_offset"), &CharacterController3D::get_avatar_offset);
	ClassDB::bind_method(D_METHOD("set_avatar_rotation_offset", "rot_deg"), &CharacterController3D::set_avatar_rotation_offset);
	ClassDB::bind_method(D_METHOD("get_avatar_rotation_offset"), &CharacterController3D::get_avatar_rotation_offset);
	ClassDB::bind_method(D_METHOD("set_first_person_avatar_visibility", "visibility"), &CharacterController3D::set_first_person_avatar_visibility);
	ClassDB::bind_method(D_METHOD("get_first_person_avatar_visibility"), &CharacterController3D::get_first_person_avatar_visibility);
	ClassDB::bind_method(D_METHOD("set_show_capsule_in_editor", "show"), &CharacterController3D::set_show_capsule_in_editor);
	ClassDB::bind_method(D_METHOD("is_show_capsule_in_editor_enabled"), &CharacterController3D::is_show_capsule_in_editor_enabled);

	// Telemetry API
	ClassDB::bind_method(D_METHOD("get_locomotion_state"), &CharacterController3D::get_locomotion_state);
	ClassDB::bind_method(D_METHOD("get_movement_vector_2d"), &CharacterController3D::get_movement_vector_2d);
	ClassDB::bind_method(D_METHOD("get_horizontal_speed"), &CharacterController3D::get_horizontal_speed);
	ClassDB::bind_method(D_METHOD("get_turn_rate"), &CharacterController3D::get_turn_rate);
	ClassDB::bind_method(D_METHOD("is_sprinting"), &CharacterController3D::is_sprinting);
	ClassDB::bind_method(D_METHOD("is_crouching"), &CharacterController3D::is_crouching);
	ClassDB::bind_method(D_METHOD("get_time_in_air"), &CharacterController3D::get_time_in_air);

	// Sub-Node Accessors
	ClassDB::bind_method(D_METHOD("get_avatar_root"), &CharacterController3D::get_avatar_root);
	ClassDB::bind_method(D_METHOD("get_head_pivot"), &CharacterController3D::get_head_pivot);
	ClassDB::bind_method(D_METHOD("get_spring_arm"), &CharacterController3D::get_spring_arm);
	ClassDB::bind_method(D_METHOD("get_camera"), &CharacterController3D::get_camera);

	// Property Groups in Inspector
	ADD_GROUP("View & Camera", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_mode", PROPERTY_HINT_ENUM, "First Person,Third Person,Dynamic Toggle / Zoom"), "set_view_mode", "get_view_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "facing_mode", PROPERTY_HINT_ENUM, "Camera Direction (Strafe),Movement Direction (Orient)"), "set_facing_mode", "get_facing_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "eye_height", PROPERTY_HINT_RANGE, "0.5,3.0,0.05,suffix:m"), "set_eye_height", "get_eye_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crouch_eye_height", PROPERTY_HINT_RANGE, "0.3,2.0,0.05,suffix:m"), "set_crouch_eye_height", "get_crouch_eye_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_sensitivity_x", PROPERTY_HINT_RANGE, "0.0005,0.02,0.0005"), "set_mouse_sensitivity_x", "get_mouse_sensitivity_x");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_sensitivity_y", PROPERTY_HINT_RANGE, "0.0005,0.02,0.0005"), "set_mouse_sensitivity_y", "get_mouse_sensitivity_y");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_smoothing", PROPERTY_HINT_RANGE, "0.0,0.95,0.05"), "set_mouse_smoothing", "get_mouse_smoothing");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invert_mouse_y"), "set_invert_mouse_y", "is_mouse_y_inverted");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_pitch_angle", PROPERTY_HINT_RANGE, "-90.0,0.0,1.0,suffix:deg"), "set_min_pitch_angle", "get_min_pitch_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_pitch_angle", PROPERTY_HINT_RANGE, "0.0,90.0,1.0,suffix:deg"), "set_max_pitch_angle", "get_max_pitch_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "third_person_distance", PROPERTY_HINT_RANGE, "0.5,15.0,0.1,suffix:m"), "set_third_person_distance", "get_third_person_distance");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "third_person_offset"), "set_third_person_offset", "get_third_person_offset");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "first_person_fov", PROPERTY_HINT_RANGE, "45.0,120.0,1.0,suffix:deg"), "set_first_person_fov", "get_first_person_fov");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "third_person_fov", PROPERTY_HINT_RANGE, "45.0,120.0,1.0,suffix:deg"), "set_third_person_fov", "get_third_person_fov");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_capture_mouse"), "set_auto_capture_mouse", "is_auto_capture_mouse_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active_camera"), "set_active_camera", "is_active_camera");

	ADD_GROUP("Camera Polish", "camera_polish_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "head_bob_enabled"), "set_head_bob_enabled", "is_head_bob_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "head_bob_frequency", PROPERTY_HINT_RANGE, "0.5,5.0,0.1"), "set_head_bob_frequency", "get_head_bob_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "head_bob_amplitude_h", PROPERTY_HINT_RANGE, "0.0,0.2,0.005"), "set_head_bob_amplitude_h", "get_head_bob_amplitude_h");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "head_bob_amplitude_v", PROPERTY_HINT_RANGE, "0.0,0.2,0.005"), "set_head_bob_amplitude_v", "get_head_bob_amplitude_v");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sprint_fov_kick_enabled"), "set_sprint_fov_kick_enabled", "is_sprint_fov_kick_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sprint_fov_kick_amount", PROPERTY_HINT_RANGE, "0.0,30.0,1.0,suffix:deg"), "set_sprint_fov_kick_amount", "get_sprint_fov_kick_amount");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "strafe_roll_enabled"), "set_strafe_roll_enabled", "is_strafe_roll_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "strafe_roll_max_angle", PROPERTY_HINT_RANGE, "0.0,10.0,0.5,suffix:deg"), "set_strafe_roll_max_angle", "get_strafe_roll_max_angle");

	ADD_GROUP("Movement Dynamics", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "walk_speed", PROPERTY_HINT_RANGE, "0.5,30.0,0.1,suffix:m/s"), "set_walk_speed", "get_walk_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sprint_speed", PROPERTY_HINT_RANGE, "0.5,50.0,0.1,suffix:m/s"), "set_sprint_speed", "get_sprint_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crouch_speed", PROPERTY_HINT_RANGE, "0.5,15.0,0.1,suffix:m/s"), "set_crouch_speed", "get_crouch_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ground_acceleration", PROPERTY_HINT_RANGE, "1.0,60.0,0.5"), "set_ground_acceleration", "get_ground_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ground_deceleration", PROPERTY_HINT_RANGE, "1.0,60.0,0.5"), "set_ground_deceleration", "get_ground_deceleration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_acceleration", PROPERTY_HINT_RANGE, "0.0,30.0,0.5"), "set_air_acceleration", "get_air_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_control_multiplier", PROPERTY_HINT_RANGE, "0.0,1.0,0.05"), "set_air_control_multiplier", "get_air_control_multiplier");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_drag", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_air_drag", "get_air_drag");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation_smooth_rate", PROPERTY_HINT_RANGE, "1.0,30.0,0.5"), "set_rotation_smooth_rate", "get_rotation_smooth_rate");

	ADD_GROUP("Jump & Gravity", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "jump_height", PROPERTY_HINT_RANGE, "0.1,10.0,0.05,suffix:m"), "set_jump_height", "get_jump_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_air_jumps", PROPERTY_HINT_RANGE, "0,5,1"), "set_max_air_jumps", "get_max_air_jumps");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coyote_time", PROPERTY_HINT_RANGE, "0.0,0.5,0.01,suffix:s"), "set_coyote_time", "get_coyote_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "jump_buffer_time", PROPERTY_HINT_RANGE, "0.0,0.5,0.01,suffix:s"), "set_jump_buffer_time", "get_jump_buffer_time");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "variable_jump_height"), "set_variable_jump_height", "is_variable_jump_height_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.1"), "set_gravity_scale", "get_gravity_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fall_gravity_multiplier", PROPERTY_HINT_RANGE, "0.5,4.0,0.1"), "set_fall_gravity_multiplier", "get_fall_gravity_multiplier");

	ADD_GROUP("Crouch & Collision", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "standing_height", PROPERTY_HINT_RANGE, "0.5,3.0,0.05,suffix:m"), "set_standing_height", "get_standing_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crouch_height", PROPERTY_HINT_RANGE, "0.3,2.0,0.05,suffix:m"), "set_crouch_height", "get_crouch_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "capsule_radius", PROPERTY_HINT_RANGE, "0.1,1.5,0.05,suffix:m"), "set_capsule_radius", "get_capsule_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crouch_transition_speed", PROPERTY_HINT_RANGE, "1.0,30.0,0.5"), "set_crouch_transition_speed", "get_crouch_transition_speed");

	ADD_GROUP("Stairs & Slopes", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stair_stepping_enabled"), "set_stair_stepping_enabled", "is_stair_stepping_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_step_height", PROPERTY_HINT_RANGE, "0.05,1.0,0.02,suffix:m"), "set_max_step_height", "get_max_step_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step_forward_distance", PROPERTY_HINT_RANGE, "0.1,1.0,0.05,suffix:m"), "set_step_forward_distance", "get_step_forward_distance");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "slide_on_steep_slopes"), "set_slide_on_steep_slopes", "is_slide_on_steep_slopes_enabled");

	ADD_GROUP("Avatar Ingestion", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_avatar_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_custom_avatar_scene", "get_custom_avatar_scene");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "avatar_offset"), "set_avatar_offset", "get_avatar_offset");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "avatar_rotation_offset"), "set_avatar_rotation_offset", "get_avatar_rotation_offset");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "first_person_avatar_visibility", PROPERTY_HINT_ENUM, "Visible,Shadows Only (Recommended),Hidden"), "set_first_person_avatar_visibility", "get_first_person_avatar_visibility");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_capsule_in_editor"), "set_show_capsule_in_editor", "is_show_capsule_in_editor_enabled");

	// Signals
	ADD_SIGNAL(MethodInfo("footstep", PropertyInfo(Variant::INT, "foot_index"), PropertyInfo(Variant::VECTOR3, "position")));
	ADD_SIGNAL(MethodInfo("jumped"));
	ADD_SIGNAL(MethodInfo("landed", PropertyInfo(Variant::FLOAT, "impact_velocity")));
	ADD_SIGNAL(MethodInfo("camera_mode_changed", PropertyInfo(Variant::INT, "new_mode")));
	ADD_SIGNAL(MethodInfo("locomotion_state_changed", PropertyInfo(Variant::INT, "new_state")));

	// Enums
	BIND_ENUM_CONSTANT(VIEW_FIRST_PERSON);
	BIND_ENUM_CONSTANT(VIEW_THIRD_PERSON);
	BIND_ENUM_CONSTANT(VIEW_DYNAMIC_TOGGLE);

	BIND_ENUM_CONSTANT(FACING_CAMERA_DIRECTION);
	BIND_ENUM_CONSTANT(FACING_MOVEMENT_DIRECTION);

	BIND_ENUM_CONSTANT(AVATAR_VISIBLE);
	BIND_ENUM_CONSTANT(AVATAR_SHADOWS_ONLY);
	BIND_ENUM_CONSTANT(AVATAR_HIDDEN);

	BIND_ENUM_CONSTANT(STATE_IDLE);
	BIND_ENUM_CONSTANT(STATE_WALK);
	BIND_ENUM_CONSTANT(STATE_SPRINT);
	BIND_ENUM_CONSTANT(STATE_CROUCH);
	BIND_ENUM_CONSTANT(STATE_JUMP_RISE);
	BIND_ENUM_CONSTANT(STATE_AIR_FALL);
	BIND_ENUM_CONSTANT(STATE_LAND);
	BIND_ENUM_CONSTANT(STATE_SLIDE);
}
