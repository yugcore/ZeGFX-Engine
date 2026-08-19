/**************************************************************************/
/*  knits_serializer.h                                                    */
/**************************************************************************/

#pragma once

#include "knits_node.h"
#include "core/io/file_access.h"
#include "core/string/ustring.h"

class KnitsSerializer {
public:
	static String serialize(const Ref<KnitsGraph> &p_graph);
	static bool deserialize(const String &p_text, Ref<KnitsGraph> &r_graph, String &r_error);

	static Error save_to_file(const Ref<KnitsGraph> &p_graph, const String &p_path);
	static Ref<KnitsGraph> load_from_file(const String &p_path, Error *r_error = nullptr);
};
