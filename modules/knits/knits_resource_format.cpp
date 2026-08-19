/**************************************************************************/
/*  knits_resource_format.cpp                                             */
/**************************************************************************/

#include "knits_resource_format.h"
#include "knits_script.h"
#include "knits_serializer.h"

Ref<Resource> ResourceFormatLoaderKnits::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Error err = OK;
	Ref<KnitsGraph> graph = KnitsSerializer::load_from_file(p_path, &err);

	if (err != OK || graph.is_null()) {
		if (r_error) *r_error = err;
		return Ref<Resource>();
	}

	String ext = p_path.get_extension().to_lower();
	if (ext == "knit_macro") {
		if (r_error) *r_error = OK;
		return graph;
	}

	Ref<KnitsScript> script;
	script.instantiate();
	script->set_graph(graph);
	script->set_path(p_original_path.is_empty() ? p_path : p_original_path);

	if (r_error) *r_error = OK;
	return script;
}

void ResourceFormatLoaderKnits::get_recognized_extensions(List<String> *p_extensions) const {
	if (p_extensions) {
		p_extensions->push_back("knit");
		p_extensions->push_back("knit_macro");
	}
}

bool ResourceFormatLoaderKnits::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "KnitsScript" || p_type == "KnitsGraph" || p_type == "Resource";
}

String ResourceFormatLoaderKnits::get_resource_type(const String &p_path) const {
	String ext = p_path.get_extension().to_lower();
	if (ext == "knit") return "KnitsScript";
	if (ext == "knit_macro") return "KnitsGraph";
	return "";
}

Error ResourceFormatSaverKnits::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<KnitsScript> script = p_resource;
	if (script.is_valid()) {
		Ref<KnitsGraph> graph = script->get_graph();
		if (graph.is_null()) {
			graph.instantiate();
			graph->graph_name = p_path.get_file().get_basename();
			script->set_graph(graph);
		}
		return KnitsSerializer::save_to_file(graph, p_path);
	}

	Ref<KnitsGraph> graph = p_resource;
	if (graph.is_valid()) {
		return KnitsSerializer::save_to_file(graph, p_path);
	}

	return ERR_UNAVAILABLE;
}

void ResourceFormatSaverKnits::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (p_extensions) {
		if (Object::cast_to<KnitsScript>(p_resource.ptr()) || Object::cast_to<KnitsGraph>(p_resource.ptr())) {
			p_extensions->push_back("knit");
			p_extensions->push_back("knit_macro");
		}
	}
}

bool ResourceFormatSaverKnits::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<KnitsScript>(p_resource.ptr()) != nullptr || Object::cast_to<KnitsGraph>(p_resource.ptr()) != nullptr;
}
