/**************************************************************************/
/*  zelyn_script_instance.cpp                                             */
/**************************************************************************/

#include "zelyn_script_instance.h"
#include "zelyn_script_language.h"
#include "bridge/zelyn_variant_bridge.h"

#include "scene/main/node.h"
#include "core/config/engine.h"

void ZelynScriptInstance::_wire_declarative_signals() {
	if (!owner || !script.is_valid()) return;

	Node *owner_node = Object::cast_to<Node>(owner);
	if (!owner_node) return;

	for (const ZelynDeclarativeHandler &h : script->get_declarative_handlers()) {
		Node *target = owner_node->get_node_or_null(NodePath(h.node_name));
		if (!target) {
			target = owner_node->find_child(h.node_name, true, false);
		}
		if (target && target->has_signal(StringName(h.signal_name))) {
			Callable target_callable(owner, StringName(h.function_name));
			if (!target->is_connected(StringName(h.signal_name), target_callable)) {
				target->connect(StringName(h.signal_name), target_callable);
			}
		}
	}
}

bool ZelynScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	member_properties[p_name] = p_value;
	return true;
}

bool ZelynScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	// 1. Check member variables
	const Variant *val = member_properties.getptr(p_name);
	if (val) {
		r_ret = *val;
		return true;
	}

	// 2. Zero-Boilerplate Child Node Auto-Resolution
	if (owner) {
		const Node *owner_node = Object::cast_to<Node>(owner);
		if (owner_node) {
			Node *child = owner_node->get_node_or_null(NodePath(p_name));
			if (child) {
				r_ret = child;
				return true;
			}
		}
	}

	return false;
}

void ZelynScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (script.is_valid()) {
		script->get_script_property_list(p_properties);
	}
}

Variant::Type ZelynScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	const Variant *val = member_properties.getptr(p_name);
	if (val) {
		if (r_is_valid) *r_is_valid = true;
		return val->get_type();
	}
	if (r_is_valid) *r_is_valid = false;
	return Variant::NIL;
}

void ZelynScriptInstance::validate_property(PropertyInfo &p_property) const {}

bool ZelynScriptInstance::property_can_revert(const StringName &p_name) const {
	return false;
}

bool ZelynScriptInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
	return false;
}

void ZelynScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool ZelynScriptInstance::has_method(const StringName &p_method) const {
	if (p_method == "transition_to") return true;
	return script.is_valid() && script->has_method(p_method);
}

bool ZelynScriptInstance::transition_to(const StringName &p_state) {
	return state_machine.transition_to(this, p_state);
}

Variant ZelynScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) {
	if (p_method == "transition_to") {
		if (p_arg_count > 0 && p_args[0]) {
			StringName target_st = *p_args[0];
			r_error.error = Callable::CallError::CALL_OK;
			return transition_to(target_st);
		}
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		return false;
	}

	if (!script.is_valid() || !script->is_script_valid()) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;

	// Bridge call into Zelyn VM
	BytecodeVM &vm = ZelynScriptLanguage::get_singleton()->get_shared_vm();
	const Chunk &chunk = script->get_compiled_chunk();

	try {
		String s_method = p_method;
		CharString method_cs = s_method.utf8();
		uint32_t method_sym = SymbolTable::get().getOrCreate(std::string(method_cs.get_data(), method_cs.length()));
		const ZChunk *target_func = nullptr;
		for (const ZChunk *child : chunk.children) {
			if (child && child->functionNameId == method_sym) {
				target_func = child;
				break;
			}
		}

		if (target_func) {
			size_t total_needed = (size_t)target_func->maxRegisters + (size_t)p_arg_count + 4;
			vm.ensureRegisterCapacity(total_needed);
			if (target_func->paramCount > (uint16_t)p_arg_count) {
				vm.registers[0] = ZelynVariantBridge::make_object_handle(owner);
				for (int i = 0; i < p_arg_count; i++) {
					if (p_args[i]) {
						vm.registers[1 + i] = ZelynVariantBridge::variant_to_zelyn(*p_args[i]);
					} else {
						vm.registers[1 + i] = Value();
					}
				}
			} else {
				for (int i = 0; i < p_arg_count; i++) {
					if (p_args[i]) {
						vm.registers[i] = ZelynVariantBridge::variant_to_zelyn(*p_args[i]);
					} else {
						vm.registers[i] = Value();
					}
				}
			}
			Value result = vm.run(*target_func, 0);
			return ZelynVariantBridge::zelyn_to_variant(result);
		} else {
			r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
			return Variant();
		}
	} catch (...) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}
}

void ZelynScriptInstance::notification(int p_notification, bool p_reversed) {
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		if (!script.is_valid() || !script->is_tool()) {
			if (p_notification == Object::NOTIFICATION_PREDELETE) {
				member_properties.clear();
			}
			return;
		}
	}

	switch (p_notification) {
		case Node::NOTIFICATION_READY: {
			_wire_declarative_signals();
			if (!state_machine.is_empty()) {
				transition_to(state_machine.get_current_state());
			}
			if (script.is_valid() && script->is_script_valid()) {
				BytecodeVM &vm = ZelynScriptLanguage::get_singleton()->get_shared_vm();
				const Chunk &chunk = script->get_compiled_chunk();
				vm.run(chunk, 0);
			}
			if (has_method("on_ready")) {
				Callable::CallError err;
				callp("on_ready", nullptr, 0, err);
			}
		} break;
		case Node::NOTIFICATION_PROCESS: {
			Node *n = Object::cast_to<Node>(owner);
			double delta = n ? n->get_process_delta_time() : 0.0;
			state_machine.update(this, delta);
			if (has_method("on_process")) {
				Variant v_delta = delta;
				const Variant *args[1] = { &v_delta };
				Callable::CallError err;
				callp("on_process", args, 1, err);
			}
		} break;
		case Node::NOTIFICATION_PHYSICS_PROCESS: {
			if (has_method("on_physics_process")) {
				Node *n = Object::cast_to<Node>(owner);
				double delta = n ? n->get_physics_process_delta_time() : 0.0;
				Variant v_delta = delta;
				const Variant *args[1] = { &v_delta };
				Callable::CallError err;
				callp("on_physics_process", args, 1, err);
			}
		} break;
		case Object::NOTIFICATION_PREDELETE: {
			// Clear cached references
			member_properties.clear();
		} break;
	}
}

String ZelynScriptInstance::to_string(bool *r_valid) {
	if (r_valid) *r_valid = true;
	return "[ZelynScriptInstance: " + String::num_int64(owner ? (uint64_t)owner->get_instance_id() : 0) + "]";
}

Ref<Script> ZelynScriptInstance::get_script() const {
	return script;
}

ScriptLanguage *ZelynScriptInstance::get_language() {
	return ZelynScriptLanguage::get_singleton();
}

ZelynScriptInstance::ZelynScriptInstance(Object *p_owner, const Ref<ZelynScript> &p_script) :
		owner(p_owner), script(p_script) {
	if (script.is_valid()) {
		for (const ZelynState &st : script->get_declared_states()) {
			state_machine.register_state(st);
		}
	}
}

ZelynScriptInstance::~ZelynScriptInstance() {
	if (owner) {
		member_properties.clear();
		owner = nullptr;
	}
}
