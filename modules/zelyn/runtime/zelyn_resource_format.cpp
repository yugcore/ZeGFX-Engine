/**************************************************************************/
/*  zelyn_resource_format.cpp                                             */
/**************************************************************************/

#include "zelyn_resource_format.h"
#include "zelyn_script.h"
#include "core/io/file_access.h"

Ref<Resource> ResourceFormatLoaderZelyn::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Error err = OK;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (err != OK) {
		if (r_error) *r_error = err;
		return Ref<Resource>();
	}

	Ref<ZelynScript> script;
	script.instantiate();
	script->set_path(p_path);
	script->set_source_code(f->get_as_utf8_string());

	if (r_error) *r_error = OK;
	return script;
}

void ResourceFormatLoaderZelyn::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("zl");
}

bool ResourceFormatLoaderZelyn::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "ZelynScript";
}

String ResourceFormatLoaderZelyn::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "zl") {
		return "ZelynScript";
	}
	return "";
}

Error ResourceFormatSaverZelyn::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<ZelynScript> script = p_resource;
	if (script.is_null()) {
		return ERR_INVALID_PARAMETER;
	}

	Error err = OK;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);
	if (err != OK) {
		return err;
	}

	f->store_string(script->get_source_code());
	return OK;
}

void ResourceFormatSaverZelyn::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<ZelynScript>(p_resource.ptr())) {
		p_extensions->push_back("zl");
	}
}

bool ResourceFormatSaverZelyn::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<ZelynScript>(p_resource.ptr()) != nullptr;
}
