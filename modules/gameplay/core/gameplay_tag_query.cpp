/**************************************************************************/
/*  gameplay_tag_query.cpp                                                */
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

#include "gameplay_tag_query.h"
#include "gameplay_tags.h"

void GameplayTagQuery::set_query_type(QueryType p_type) {
	query_type = p_type;
	emit_changed();
}

void GameplayTagQuery::set_tags(const PackedStringArray &p_tags) {
	tags = p_tags;
	emit_changed();
}

void GameplayTagQuery::set_expression_op(ExpressionOp p_op) {
	expression_op = p_op;
	emit_changed();
}

void GameplayTagQuery::set_sub_queries(const TypedArray<GameplayTagQuery> &p_queries) {
	sub_queries = p_queries;
	emit_changed();
}

TypedArray<GameplayTagQuery> GameplayTagQuery::get_sub_queries() const {
	return sub_queries;
}

bool GameplayTagQuery::evaluate(const Ref<GameplayTagContainer> &p_container) const {
	if (p_container.is_null()) {
		return (query_type == QUERY_MATCH_NONE);
	}

	switch (query_type) {
		case QUERY_MATCH_ANY: {
			if (tags.is_empty()) {
				return true;
			}
			return p_container->has_any(tags);
		}
		case QUERY_MATCH_ALL: {
			if (tags.is_empty()) {
				return true;
			}
			return p_container->has_all(tags);
		}
		case QUERY_MATCH_NONE: {
			if (tags.is_empty()) {
				return true;
			}
			return p_container->has_none(tags);
		}
		case QUERY_MATCH_EXPRESSION: {
			if (sub_queries.is_empty()) {
				return true;
			}
			if (expression_op == OP_NOT) {
				if (sub_queries.size() > 0) {
					Ref<GameplayTagQuery> q = sub_queries[0];
					if (q.is_valid()) {
						return !q->evaluate(p_container);
					}
				}
				return true;
			} else if (expression_op == OP_OR) {
				for (int i = 0; i < sub_queries.size(); i++) {
					Ref<GameplayTagQuery> q = sub_queries[i];
					if (q.is_valid() && q->evaluate(p_container)) {
						return true;
					}
				}
				return false;
			} else { // OP_AND
				for (int i = 0; i < sub_queries.size(); i++) {
					Ref<GameplayTagQuery> q = sub_queries[i];
					if (q.is_valid() && !q->evaluate(p_container)) {
						return false;
					}
				}
				return true;
			}
		}
	}
	return false;
}

bool GameplayTagQuery::evaluate_tags(const PackedStringArray &p_tags) const {
	Ref<GameplayTagContainer> temp;
	temp.instantiate();
	temp->set_tags(p_tags);
	return evaluate(temp);
}

Ref<GameplayTagQuery> GameplayTagQuery::create_match_any(const PackedStringArray &p_tags) {
	Ref<GameplayTagQuery> q;
	q.instantiate();
	q->set_query_type(QUERY_MATCH_ANY);
	q->set_tags(p_tags);
	return q;
}

Ref<GameplayTagQuery> GameplayTagQuery::create_match_all(const PackedStringArray &p_tags) {
	Ref<GameplayTagQuery> q;
	q.instantiate();
	q->set_query_type(QUERY_MATCH_ALL);
	q->set_tags(p_tags);
	return q;
}

Ref<GameplayTagQuery> GameplayTagQuery::create_match_none(const PackedStringArray &p_tags) {
	Ref<GameplayTagQuery> q;
	q.instantiate();
	q->set_query_type(QUERY_MATCH_NONE);
	q->set_tags(p_tags);
	return q;
}

void GameplayTagQuery::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_query_type", "type"), &GameplayTagQuery::set_query_type);
	ClassDB::bind_method(D_METHOD("get_query_type"), &GameplayTagQuery::get_query_type);

	ClassDB::bind_method(D_METHOD("set_tags", "tags"), &GameplayTagQuery::set_tags);
	ClassDB::bind_method(D_METHOD("get_tags"), &GameplayTagQuery::get_tags);

	ClassDB::bind_method(D_METHOD("set_expression_op", "op"), &GameplayTagQuery::set_expression_op);
	ClassDB::bind_method(D_METHOD("get_expression_op"), &GameplayTagQuery::get_expression_op);

	ClassDB::bind_method(D_METHOD("set_sub_queries", "queries"), &GameplayTagQuery::set_sub_queries);
	ClassDB::bind_method(D_METHOD("get_sub_queries"), &GameplayTagQuery::get_sub_queries);

	ClassDB::bind_method(D_METHOD("evaluate", "container"), &GameplayTagQuery::evaluate);
	ClassDB::bind_method(D_METHOD("evaluate_tags", "tags"), &GameplayTagQuery::evaluate_tags);

	ClassDB::bind_static_method("GameplayTagQuery", D_METHOD("create_match_any", "tags"), &GameplayTagQuery::create_match_any);
	ClassDB::bind_static_method("GameplayTagQuery", D_METHOD("create_match_all", "tags"), &GameplayTagQuery::create_match_all);
	ClassDB::bind_static_method("GameplayTagQuery", D_METHOD("create_match_none", "tags"), &GameplayTagQuery::create_match_none);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "query_type", PROPERTY_HINT_ENUM, "Match Any,Match All,Match None,Match Expression"), "set_query_type", "get_query_type");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "tags"), "set_tags", "get_tags");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "expression_op", PROPERTY_HINT_ENUM, "And,Or,Not"), "set_expression_op", "get_expression_op");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "sub_queries", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagQuery"), "set_sub_queries", "get_sub_queries");

	BIND_ENUM_CONSTANT(QUERY_MATCH_ANY);
	BIND_ENUM_CONSTANT(QUERY_MATCH_ALL);
	BIND_ENUM_CONSTANT(QUERY_MATCH_NONE);
	BIND_ENUM_CONSTANT(QUERY_MATCH_EXPRESSION);

	BIND_ENUM_CONSTANT(OP_AND);
	BIND_ENUM_CONSTANT(OP_OR);
	BIND_ENUM_CONSTANT(OP_NOT);
}
