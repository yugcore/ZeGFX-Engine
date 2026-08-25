/**************************************************************************/
/*  gameplay_tag_query.h                                                  */
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

#pragma once

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/variant/typed_array.h"

class GameplayTagContainer;

class GameplayTagQuery : public Resource {
	GDCLASS(GameplayTagQuery, Resource);

public:
	enum QueryType {
		QUERY_MATCH_ANY,
		QUERY_MATCH_ALL,
		QUERY_MATCH_NONE,
		QUERY_MATCH_EXPRESSION,
	};

	enum ExpressionOp {
		OP_AND,
		OP_OR,
		OP_NOT,
	};

private:
	QueryType query_type = QUERY_MATCH_ANY;
	ExpressionOp expression_op = OP_AND;
	PackedStringArray tags;
	Array sub_queries;

protected:
	static void _bind_methods();

public:
	void set_query_type(QueryType p_type);
	QueryType get_query_type() const { return query_type; }

	void set_expression_op(ExpressionOp p_op);
	ExpressionOp get_expression_op() const { return expression_op; }

	void set_tags(const PackedStringArray &p_tags);
	PackedStringArray get_tags() const { return tags; }

	void set_sub_queries(const TypedArray<GameplayTagQuery> &p_queries);
	TypedArray<GameplayTagQuery> get_sub_queries() const;

	bool evaluate(const Ref<GameplayTagContainer> &p_container) const;
	bool evaluate_tags(const PackedStringArray &p_tags) const;

	static Ref<GameplayTagQuery> create_match_any(const PackedStringArray &p_tags);
	static Ref<GameplayTagQuery> create_match_all(const PackedStringArray &p_tags);
	static Ref<GameplayTagQuery> create_match_none(const PackedStringArray &p_tags);

	GameplayTagQuery() {}
	~GameplayTagQuery() {}
};

VARIANT_ENUM_CAST(GameplayTagQuery::QueryType);
VARIANT_ENUM_CAST(GameplayTagQuery::ExpressionOp);
