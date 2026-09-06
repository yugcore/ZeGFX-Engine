/**************************************************************************/
/*  zelyn_language_features.cpp                                           */
/**************************************************************************/

#include "zelyn_language_features.h"
#include "zelyn_variant_bridge.h"

#include "core/variant/variant_utility.h"
#include "core/config/engine.h"
#include "core/os/os.h"
#include "core/math/math_funcs.h"
#include "core/input/input.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "zelyn/api_bridge.h"
#include <cmath>

std::vector<Token> ZelynLanguageFeatures::normalize_tokens(const std::vector<Token> &p_tokens) {
	std::vector<Token> out;
	out.reserve(p_tokens.size());

	size_t i = 0;
	while (i < p_tokens.size()) {
		const Token &tok = p_tokens[i];

		// 1. Convert 'let' identifier to 'var'
		if (tok.type == TokenType::IDENTIFIER && tok.getValue() == "let") {
			out.push_back(Token(TokenType::VAR, "var", tok.line, tok.column, tok.filename));
			i++;
			// Handle optional type annotation on let: let speed: float = 300.0;
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
				out.push_back(p_tokens[i]);
				i++;
				if (i < p_tokens.size() && p_tokens[i].type == TokenType::COLON) {
					i++; // Skip ':'
					if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
						i++; // Skip type name
						if (i < p_tokens.size() && p_tokens[i].type == TokenType::LBRACKET) {
							int depth = 1;
							i++;
							while (i < p_tokens.size() && depth > 0) {
								if (p_tokens[i].type == TokenType::LBRACKET) depth++;
								else if (p_tokens[i].type == TokenType::RBRACKET) depth--;
								i++;
							}
						}
					}
				}
			}
			continue;
		}

		// 2. Type annotations on 'var': var speed: float = 300.0;
		if (tok.type == TokenType::VAR) {
			out.push_back(tok);
			i++;
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
				out.push_back(p_tokens[i]);
				i++;
				if (i < p_tokens.size() && p_tokens[i].type == TokenType::COLON) {
					i++; // Skip ':'
					if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
						i++; // Skip type name
						if (i < p_tokens.size() && p_tokens[i].type == TokenType::LBRACKET) {
							int depth = 1;
							i++;
							while (i < p_tokens.size() && depth > 0) {
								if (p_tokens[i].type == TokenType::LBRACKET) depth++;
								else if (p_tokens[i].type == TokenType::RBRACKET) depth--;
								i++;
							}
						}
					}
				}
			}
			continue;
		}

		// 3. Function parameter and return type annotations: func foo(a: int, b: float) -> void {
		if (tok.type == TokenType::FUNC) {
			out.push_back(tok);
			i++;
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
				out.push_back(p_tokens[i]); // func name
				i++;
			}
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::DOT) {
				out.push_back(p_tokens[i]);
				i++;
				if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
					out.push_back(p_tokens[i]);
					i++;
				}
			}
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::LPAREN) {
				out.push_back(p_tokens[i]);
				i++;
				while (i < p_tokens.size() && p_tokens[i].type != TokenType::RPAREN) {
					if (p_tokens[i].type == TokenType::IDENTIFIER) {
						out.push_back(p_tokens[i]);
						i++;
						if (i < p_tokens.size() && p_tokens[i].type == TokenType::COLON) {
							i++; // Skip ':'
							if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
								i++; // Skip type name
								if (i < p_tokens.size() && p_tokens[i].type == TokenType::LBRACKET) {
									int depth = 1;
									i++;
									while (i < p_tokens.size() && depth > 0) {
										if (p_tokens[i].type == TokenType::LBRACKET) depth++;
										else if (p_tokens[i].type == TokenType::RBRACKET) depth--;
										i++;
									}
								}
							}
						}
					} else {
						out.push_back(p_tokens[i]);
						i++;
					}
				}
				if (i < p_tokens.size() && p_tokens[i].type == TokenType::RPAREN) {
					out.push_back(p_tokens[i]);
					i++;
				}

				// Check for optional return type: '-> Type' or ': Type' before '{'
				if (i < p_tokens.size() && p_tokens[i].type == TokenType::MINUS && i + 1 < p_tokens.size() && p_tokens[i + 1].type == TokenType::GREATER) {
					i += 2; // Skip '->'
					if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
						i++; // Skip return type name
					}
				} else if (i < p_tokens.size() && p_tokens[i].type == TokenType::COLON) {
					i++; // Skip ':'
					if (i < p_tokens.size() && p_tokens[i].type == TokenType::IDENTIFIER) {
						i++; // Skip return type name
					}
				}
			}
			continue;
		}

		// 4. 'null' literal -> 'nil'
		if (tok.type == TokenType::IDENTIFIER && tok.getValue() == "null") {
			out.push_back(Token(TokenType::NIL_LITERAL, "nil", tok.line, tok.column, tok.filename));
			i++;
			continue;
		}

		// 5. 'elif' -> 'else' 'if'
		if (tok.type == TokenType::IDENTIFIER && tok.getValue() == "elif") {
			out.push_back(Token(TokenType::ELSE, "else", tok.line, tok.column, tok.filename));
			out.push_back(Token(TokenType::IF, "if", tok.line, tok.column, tok.filename));
			i++;
			continue;
		}

		// 6. 'pass' statement -> skip
		if (tok.type == TokenType::IDENTIFIER && tok.getValue() == "pass") {
			i++;
			if (i < p_tokens.size() && p_tokens[i].type == TokenType::SEMICOLON) {
				i++;
			}
			continue;
		}

		out.push_back(tok);
		i++;
	}

	return out;
}

void ZelynLanguageFeatures::register_godot_builtins(NativeRegistry &reg) {
	// 1. Register upstream core and gameplay modules
	bindCoreAPI(reg);
	bindGameplayAPI(reg);

	// 2. Register Godot Engine & GDScript GlobalScope utility functions
	reg.bindFast("print", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		String s;
		for (int i = 0; i < argCount; i++) {
			if (i > 0) s += " ";
			s += ZelynVariantBridge::zelyn_to_variant(args[i]).operator String();
		}
		print_line(s);
		return Value();
	}, 0, 255);

	reg.bindFast("printerr", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		String s;
		for (int i = 0; i < argCount; i++) {
			if (i > 0) s += " ";
			s += ZelynVariantBridge::zelyn_to_variant(args[i]).operator String();
		}
		print_error(s);
		return Value();
	}, 0, 255);

	reg.bindFast("push_error", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			ERR_PRINT(ZelynVariantBridge::zelyn_to_variant(args[0]).operator String());
		}
		return Value();
	}, 1, 1);

	reg.bindFast("push_warning", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			WARN_PRINT(ZelynVariantBridge::zelyn_to_variant(args[0]).operator String());
		}
		return Value();
	}, 1, 1);

	reg.bindFast("str", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		String s;
		for (int i = 0; i < argCount; i++) {
			s += ZelynVariantBridge::zelyn_to_variant(args[i]).operator String();
		}
		return ZelynVariantBridge::variant_to_zelyn(s);
	}, 0, 255);

	// Math
	reg.bindFast("lerp", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 3) {
			Variant from = ZelynVariantBridge::zelyn_to_variant(args[0]);
			Variant to = ZelynVariantBridge::zelyn_to_variant(args[1]);
			double weight = args[2].number;
			Callable::CallError err;
			Variant res = VariantUtilityFunctions::lerp(from, to, weight, err);
			return ZelynVariantBridge::variant_to_zelyn(res);
		}
		return Value();
	}, 3, 3);

	reg.bindFast("clamp", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 3) {
			double val = args[0].number;
			double min_val = args[1].number;
			double max_val = args[2].number;
			return Value(Value::FLOAT, CLAMP(val, min_val, max_val));
		}
		return Value();
	}, 3, 3);

	reg.bindFast("remap", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 5) {
			double res = VariantUtilityFunctions::remap(args[0].number, args[1].number, args[2].number, args[3].number, args[4].number);
			return Value(Value::FLOAT, res);
		}
		return Value();
	}, 5, 5);

	reg.bindFast("smoothstep", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 3) {
			double res = VariantUtilityFunctions::smoothstep(args[0].number, args[1].number, args[2].number);
			return Value(Value::FLOAT, res);
		}
		return Value();
	}, 3, 3);

	reg.bindFast("move_toward", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 3) {
			double res = VariantUtilityFunctions::move_toward(args[0].number, args[1].number, args[2].number);
			return Value(Value::FLOAT, res);
		}
		return Value();
	}, 3, 3);

	reg.bindFast("rotate_toward", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 3) {
			double res = VariantUtilityFunctions::rotate_toward(args[0].number, args[1].number, args[2].number);
			return Value(Value::FLOAT, res);
		}
		return Value();
	}, 3, 3);

	reg.bindFast("deg_to_rad", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, Math::deg_to_rad(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("rad_to_deg", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, Math::rad_to_deg(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("atan2", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) return Value(Value::FLOAT, std::atan2(args[0].number, args[1].number));
		return Value();
	}, 2, 2);

	reg.bindFast("asin", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, std::asin(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("acos", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, std::acos(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("atan", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, std::atan(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("round", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) return Value(Value::FLOAT, std::round(args[0].number));
		return Value();
	}, 1, 1);

	reg.bindFast("sign", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			double n = args[0].number;
			return Value(Value::FLOAT, (n > 0.0) ? 1.0 : ((n < 0.0) ? -1.0 : 0.0));
		}
		return Value();
	}, 1, 1);

	// Random
	reg.bindFast("randf", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		return Value(Value::FLOAT, (double)Math::randf());
	}, 0, 0);

	reg.bindFast("randi", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		return Value(Value::FLOAT, (double)Math::rand());
	}, 0, 0);

	reg.bindFast("randf_range", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			return Value(Value::FLOAT, (double)Math::random(args[0].number, args[1].number));
		}
		return Value();
	}, 2, 2);

	reg.bindFast("randi_range", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			return Value(Value::FLOAT, (double)Math::random((int)args[0].number, (int)args[1].number));
		}
		return Value();
	}, 2, 2);

	reg.bindFast("randomize", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		Math::randomize();
		return Value();
	}, 0, 0);

	reg.bindFast("is_instance_valid", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Object *obj = ZelynVariantBridge::get_object_from_handle(args[0]);
			return Value(Value::BOOL, obj != nullptr ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	// Engine yielding hooks
	reg.bindFast("wait", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		(void)ctx;
		(void)args;
		(void)argCount;
		return Value();
	}, 1, 1);

	reg.bindFast("wait_seconds", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		(void)ctx;
		(void)args;
		(void)argCount;
		return Value();
	}, 1, 1);

	// --- 3D Scene, CharacterBody3D & Input System ---
	reg.bindFast("Input.set_mouse_mode", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			Input::get_singleton()->set_mouse_mode((Input::MouseMode)(int)args[0].number);
		}
		return Value();
	}, 1, 1);

	reg.bindFast("Input.get_mouse_mode", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (Input::get_singleton()) {
			return Value(Value::FLOAT, (double)Input::get_singleton()->get_mouse_mode());
		}
		return Value(Value::FLOAT, 0.0);
	}, 0, 0);

	reg.bindFast("Input.is_key_pressed", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			Key k = (Key)(int64_t)args[0].number;
			return Value(Value::BOOL, Input::get_singleton()->is_key_pressed(k) ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("is_key_down", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			if (args[0].type == ValueType::Number) {
				Key k = (Key)(int64_t)args[0].number;
				return Value(Value::BOOL, Input::get_singleton()->is_key_pressed(k) ? 1.0 : 0.0);
			} else if (args[0].type == ValueType::Object && args[0].object && args[0].object->kind == HeapObject::Kind::String) {
				std::string s = static_cast<StringObject *>(args[0].object)->data;
				Key k = Key::NONE;
				if (s == "W" || s == "w") k = Key::W;
				else if (s == "S" || s == "s") k = Key::S;
				else if (s == "A" || s == "a") k = Key::A;
				else if (s == "D" || s == "d") k = Key::D;
				else if (s == "SPACE" || s == "space" || s == " ") k = Key::SPACE;
				else if (s == "ESCAPE" || s == "escape" || s == "esc") k = Key::ESCAPE;
				else if (s == "SHIFT" || s == "shift") k = Key::SHIFT;
				else if (s == "E" || s == "e") k = Key::E;
				if (k != Key::NONE) {
					return Value(Value::BOOL, Input::get_singleton()->is_key_pressed(k) ? 1.0 : 0.0);
				}
			}
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("is_key_just_pressed", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			Key k = Key::NONE;
			if (args[0].type == ValueType::Number) {
				k = (Key)(int64_t)args[0].number;
			} else if (args[0].type == ValueType::Object && args[0].object && args[0].object->kind == HeapObject::Kind::String) {
				std::string s = static_cast<StringObject *>(args[0].object)->data;
				if (s == "W" || s == "w") k = Key::W;
				else if (s == "S" || s == "s") k = Key::S;
				else if (s == "A" || s == "a") k = Key::A;
				else if (s == "D" || s == "d") k = Key::D;
				else if (s == "SPACE" || s == "space" || s == " ") k = Key::SPACE;
				else if (s == "ESCAPE" || s == "escape" || s == "esc") k = Key::ESCAPE;
				else if (s == "SHIFT" || s == "shift") k = Key::SHIFT;
				else if (s == "E" || s == "e") k = Key::E;
			}
			if (k != Key::NONE) {
				static HashMap<Key, bool> s_key_was_down;
				static HashMap<Key, uint64_t> s_just_pressed_frame;
				bool is_down = Input::get_singleton()->is_key_pressed(k);
				uint64_t cur_frame = Engine::get_singleton() ? Engine::get_singleton()->get_physics_frames() : 0;
				bool was_down = s_key_was_down.has(k) ? s_key_was_down[k] : false;
				if (is_down) {
					if (!was_down) {
						s_key_was_down[k] = true;
						s_just_pressed_frame[k] = cur_frame;
						return Value(Value::BOOL, 1.0);
					}
					return Value(Value::BOOL, (s_just_pressed_frame.has(k) && s_just_pressed_frame[k] == cur_frame) ? 1.0 : 0.0);
				} else {
					s_key_was_down[k] = false;
				}
			}
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("Input.is_mouse_button_pressed", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			MouseButton b = (MouseButton)(int)args[0].number;
			return Value(Value::BOOL, Input::get_singleton()->is_mouse_button_pressed(b) ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("Input.is_action_pressed", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			StringName sn = ZelynVariantBridge::zelyn_to_variant(args[0]);
			return Value(Value::BOOL, Input::get_singleton()->is_action_pressed(sn) ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("Input.is_action_just_pressed", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1 && Input::get_singleton()) {
			StringName sn = ZelynVariantBridge::zelyn_to_variant(args[0]);
			return Value(Value::BOOL, Input::get_singleton()->is_action_just_pressed(sn) ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("Input.get_mouse_velocity_x", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (Input::get_singleton()) {
			return Value(Value::FLOAT, (double)Input::get_singleton()->get_last_mouse_velocity().x);
		}
		return Value(Value::FLOAT, 0.0);
	}, 0, 0);

	reg.bindFast("Input.get_mouse_velocity_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (Input::get_singleton()) {
			return Value(Value::FLOAT, (double)Input::get_singleton()->get_last_mouse_velocity().y);
		}
		return Value(Value::FLOAT, 0.0);
	}, 0, 0);

	reg.bindFast("move_body", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 4) {
			CharacterBody3D *cb = Object::cast_to<CharacterBody3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (cb) {
				cb->set_velocity(Vector3(args[1].number, args[2].number, args[3].number));
				cb->move_and_slide();
			}
		}
		return Value();
	}, 4, 4);

	reg.bindFast("is_on_floor", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			CharacterBody3D *cb = Object::cast_to<CharacterBody3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			return Value(Value::BOOL, (cb && cb->is_on_floor()) ? 1.0 : 0.0);
		}
		return Value(Value::BOOL, 0.0);
	}, 1, 1);

	reg.bindFast("get_position_x", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) return Value(Value::FLOAT, (double)n->get_position().x);
		}
		return Value(Value::FLOAT, 0.0);
	}, 1, 1);

	reg.bindFast("get_position_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) return Value(Value::FLOAT, (double)n->get_position().y);
		}
		return Value(Value::FLOAT, 0.0);
	}, 1, 1);

	reg.bindFast("get_position_z", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) return Value(Value::FLOAT, (double)n->get_position().z);
		}
		return Value(Value::FLOAT, 0.0);
	}, 1, 1);

	reg.bindFast("set_position", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 4) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) n->set_position(Vector3(args[1].number, args[2].number, args[3].number));
		}
		return Value();
	}, 4, 4);

	reg.bindFast("set_position_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) {
				Vector3 p = n->get_position();
				p.y = args[1].number;
				n->set_position(p);
			}
		}
		return Value();
	}, 2, 2);

	reg.bindFast("get_rotation_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) return Value(Value::FLOAT, (double)n->get_rotation().y);
		}
		return Value(Value::FLOAT, 0.0);
	}, 1, 1);

	reg.bindFast("set_rotation_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) {
				Vector3 r = n->get_rotation();
				r.y = args[1].number;
				n->set_rotation(r);
			}
		}
		return Value();
	}, 2, 2);

	reg.bindFast("set_rotation_x", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) {
				Vector3 r = n->get_rotation();
				r.x = args[1].number;
				n->set_rotation(r);
			}
		}
		return Value();
	}, 2, 2);

	reg.bindFast("rotate_y", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node3D *n = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) n->rotate_y(args[1].number);
		}
		return Value();
	}, 2, 2);

	reg.bindFast("get_node", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node *n = Object::cast_to<Node>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) {
				String path = ZelynVariantBridge::zelyn_to_variant(args[1]);
				Node *child = n->get_node_or_null(NodePath(path));
				if (child) return ZelynVariantBridge::make_object_handle(child);
			}
		}
		return Value();
	}, 2, 2);

	reg.bindFast("get_distance", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 2) {
			Node3D *na = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[0]));
			Node3D *nb = Object::cast_to<Node3D>(ZelynVariantBridge::get_object_from_handle(args[1]));
			if (na && nb) {
				return Value(Value::FLOAT, (double)na->get_global_position().distance_to(nb->get_global_position()));
			}
		}
		return Value(Value::FLOAT, 999999.0);
	}, 2, 2);

	reg.bindFast("queue_free", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			Node *n = Object::cast_to<Node>(ZelynVariantBridge::get_object_from_handle(args[0]));
			if (n) n->queue_free();
		}
		return Value();
	}, 1, 1);

	reg.bindFast("set_hud_text", [](RuntimeContext *ctx, Value *args, int argCount) -> Value {
		if (argCount >= 1) {
			String text = ZelynVariantBridge::zelyn_to_variant(args[0]);
			SceneTree *st = SceneTree::get_singleton();
			if (st && st->get_root()) {
				Node *hud = st->get_root()->find_child("HUDLabel", true, false);
				if (hud) {
					hud->set("text", text);
				}
			}
		}
		return Value();
	}, 1, 1);
}

Vector<String> ZelynLanguageFeatures::get_all_keywords() {
	Vector<String> words;
	static const char *kws[] = {
		"func", "fn", "let", "var", "return", "if", "elif", "else", "while", "for", "in",
		"wait", "wait_seconds", "on", "state", "out", "print", "self", "super", "extends",
		"class", "class_name", "signal", "const", "enum", "match", "pass", "is", "as",
		"break", "continue", "await", "assert", "true", "false", "nil", "null"
	};
	for (const char *kw : kws) {
		words.push_back(kw);
	}
	return words;
}

Vector<String> ZelynLanguageFeatures::get_control_flow_keywords() {
	Vector<String> words;
	static const char *cfs[] = {
		"if", "elif", "else", "while", "for", "in", "return", "wait", "wait_seconds", "break", "continue", "match", "pass"
	};
	for (const char *cf : cfs) {
		words.push_back(cf);
	}
	return words;
}
