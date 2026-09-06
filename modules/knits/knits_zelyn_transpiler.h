/**************************************************************************/
/*  knits_zelyn_transpiler.h                                              */
/**************************************************************************/

#pragma once

#include "knits_node.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

class KnitsZelynTranspiler {
private:
	static String _export_exec_chain(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, int p_indent, HashSet<KnitNodeID> &p_visited);
	static String _export_pure_expr(const KnitsGraph &p_graph, const KnitPin &p_pin, HashSet<KnitNodeID> &p_visited);

public:
	static bool zelyn_to_knit_graph(const String &p_code, Ref<KnitsGraph> &r_graph, String &r_error);
	static bool knit_graph_to_zelyn(const Ref<KnitsGraph> &p_graph, String &r_code, String &r_error);
};
