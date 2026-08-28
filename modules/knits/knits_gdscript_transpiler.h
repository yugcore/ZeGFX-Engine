/**************************************************************************/
/*  knits_gdscript_transpiler.h                                           */
/**************************************************************************/

#pragma once

#include "knits_node.h"
#include "modules/gdscript/gdscript_parser.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

class KnitsGDScriptTranspiler {
private:
	static String _expression_to_string(const GDScriptParser::ExpressionNode *p_expr);
	static void _import_suite(const GDScriptParser::SuiteNode *p_suite, const Ref<KnitsGraph> &p_graph, KnitNodeID &r_last_node, KnitPinID &r_last_exec_pin, Vector2 &r_pos);
	static void _import_statement(const GDScriptParser::Node *p_stmt, const Ref<KnitsGraph> &p_graph, KnitNodeID &r_last_node, KnitPinID &r_last_exec_pin, Vector2 &r_pos);

	static String _export_exec_chain(const KnitsGraph &p_graph, const Ref<KnitNode> &p_node, int p_indent, HashSet<KnitNodeID> &p_visited);
	static String _export_pure_expr(const KnitsGraph &p_graph, const KnitPin &p_pin, HashSet<KnitNodeID> &p_visited);

public:
	static bool gdscript_to_knit_graph(const String &p_code, Ref<KnitsGraph> &r_graph, String &r_error);
	static bool knit_graph_to_gdscript(const Ref<KnitsGraph> &p_graph, String &r_code, String &r_error);
};
