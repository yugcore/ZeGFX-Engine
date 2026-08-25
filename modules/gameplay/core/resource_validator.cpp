/**************************************************************************/
/*  resource_validator.cpp                                                */
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

#include "resource_validator.h"

ResourceValidator *ResourceValidator::singleton = nullptr;

void ResourceValidator::register_type_validator(const StringName &p_class_name, const Callable &p_validator_callable) {
	if (!p_validator_callable.is_valid()) {
		return;
	}
	Vector<Callable> &validators = type_validators[p_class_name];
	if (!validators.has(p_validator_callable)) {
		validators.push_back(p_validator_callable);
	}
}

void ResourceValidator::unregister_type_validator(const StringName &p_class_name, const Callable &p_validator_callable) {
	if (type_validators.has(p_class_name)) {
		type_validators[p_class_name].erase(p_validator_callable);
		if (type_validators[p_class_name].is_empty()) {
			type_validators.erase(p_class_name);
		}
	}
}

PackedStringArray ResourceValidator::validate_resource(const Ref<Resource> &p_resource) const {
	PackedStringArray errors;
	if (p_resource.is_null()) {
		errors.push_back("Resource is null.");
		return errors;
	}

	StringName class_name = p_resource->get_class_name();

	// Check validators for this class and any parent classes in ClassDB
	StringName current_class = class_name;
	while (current_class != StringName()) {
		if (type_validators.has(current_class)) {
			const Vector<Callable> &validators = type_validators[current_class];
			const Variant arg = p_resource;
			const Variant *args[1] = { &arg };

			for (int i = 0; i < validators.size(); i++) {
				Callable c = validators[i];
				if (c.is_valid()) {
					Callable::CallError ce;
					Variant result;
					c.callp(args, 1, result, ce);

					if (ce.error == Callable::CallError::CALL_OK) {
						if (result.get_type() == Variant::PACKED_STRING_ARRAY) {
							PackedStringArray res_errors = result;
							for (int j = 0; j < res_errors.size(); j++) {
								errors.push_back(res_errors[j]);
							}
						} else if (result.get_type() == Variant::STRING && !String(result).is_empty()) {
							errors.push_back(String(result));
						} else if (result.get_type() == Variant::ARRAY) {
							Array arr = result;
							for (int j = 0; j < arr.size(); j++) {
								errors.push_back(arr[j]);
							}
						}
					}
				}
			}
		}
		current_class = ClassDB::get_parent_class(current_class);
	}

	return errors;
}

bool ResourceValidator::is_resource_valid(const Ref<Resource> &p_resource) const {
	return validate_resource(p_resource).is_empty();
}

void ResourceValidator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_type_validator", "class_name", "validator_callable"), &ResourceValidator::register_type_validator);
	ClassDB::bind_method(D_METHOD("unregister_type_validator", "class_name", "validator_callable"), &ResourceValidator::unregister_type_validator);

	ClassDB::bind_method(D_METHOD("validate_resource", "resource"), &ResourceValidator::validate_resource);
	ClassDB::bind_method(D_METHOD("is_resource_valid", "resource"), &ResourceValidator::is_resource_valid);
}

ResourceValidator::ResourceValidator() {
	singleton = this;
}

ResourceValidator::~ResourceValidator() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
