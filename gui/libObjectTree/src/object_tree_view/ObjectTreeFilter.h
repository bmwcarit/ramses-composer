/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * PARSING USING BOOST.SPIRIT LIBRARY
 *
 * Ramses Composer supports complex filter expressions inside of the Scene Graph as well as the Resources and Prefabs views.
 * This section explains some parts of the code which handles such filter expressions. Because the filtering expressions can be nested and in general
 * arbitrarily complex we cannot use regex expressions to parse them. For this reason the more complex tool called Boost.Spirit - a parsing library is used.
 * Some of its main concepts are explained here together with its syntax.
 *
 * The first step in dealing with the complex expressions is to define a grammar - a set of rules which define what we are looking inside the text
 * and what we are doing after we have found it.
 *
 *
 * 1. Literal parser
 *
 * Let's start with the simple example:
 * qi::rule<std::string::const_iterator, std::string(), qi::space_type> name = qi::lit("name");
 *
 * Now let's take a closer look at the line above:
 * qi::rule<> is a class which tells as that this is a grammar rule which will be used by the parser to find something in the text which we call "name".
 * This rule will iterate through any string (std::string::const_iterator) omitting white spaces (qi::space_type) and will put the result into a string (std::string()).
 * What exactly we are looking for is stated on the right hand side: qi::lit("name"). This part tells that we will be looking for literal constant which
 * has the exact sequence of letters "name".
 * Because we want to distinguish between several keywords in our text we declare 4 of such constants:
 * name = qi::lit("name");
 * type = qi::lit("type");
 * id = qi::lit("id");
 * tag = qi::lit("tag");
 *
 *
 * 2. Alternates
 *
 * Alternates: e.g. a | b. Try a first. If it succeeds, good. If not, try the next alternative, b.
 *
 * In our example the alternate is our keyword which can be either name OR type OR id OR tag:
 * keyword = name | type | id | tag;
 *
 * Analogously to the keywords we want to extract operations like equal OR not equal:
 * equal = qi::lit("=");
 * not_equal = qi::lit("!=");
 * operation = equal | not_equal;
 *
 *
 * 3. Lexeme and Optionals
 *
 * Sometimes it is important to omit some characters or strings inside the text. There are some modifiers in Boost.Spirit library which can be used for these purposes.
 * Let's assume we don't want the rounded brackets to be the part of the parsed value. We could write the grammar rule as follows:
 * value = qi::lexeme[+(qi::char_ - '(' - ')')];
 *
 * qi::lit() is for strict literal matching, useful for fixed keywords or character sequences
 * qi::lexeme[] is used to treat a sequence of characters as a single lexeme, ignoring any intervening whitespace or other irrelevant characters.
 * - operator tells that '(' and ')' should exclude the rounded brackets from the parsing result
 * + operator tells us that there must be 1 or more characters in a sequence
 * * operator tells us that there must be 0 or more characters in a sequence
 *
 *
 * 4. Sequences
 *
 * The next important part is to detect some specific sequences of the keywords, operations and values. For example we want to detect cases when a keyword is followed by
 * an operation which itself is followed by some value. In Boost.Spirit this can be expressed by the following expression, where ">>" can be read as "followed by":
 * expression = keyword >> operation >> value;
 * As soon as such a sequence is found it is classified as an expression (for example: name = myName)
 *
 *
 * 5. Semantic actions
 *
 * The examples in the previous section were very simplistic. They only recognized data, but did nothing with it. They answered the question: "Did the input match?".
 * Now, we want to extract information from what was parsed and perform some manipulations with it, in other words we will do semantic actions.
 *
 * Semantic actions may be attached to any point in the grammar specification. These actions are C++ functions or function objects that are called whenever a part
 * of the parser successfully recognizes a portion of the input. Say you have a parser P, and a C++ function F.
 * You can make the parser call F whenever it matches an input by attaching F:
 * P[F]
 *
 * Boost.Phoenix, a companion library bundled with Spirit, is specifically suited for binding semantic actions. It is like Boost.Lambda with special custom features
 * that make it easy to integrate semantic actions with Spirit. Let's look at a concrete example from the code:
 * expression = (keyword >> operation >> value)
 * 				[qi::_val = phx::bind(&ObjectTreeFilter::filterExpression, this, qi::_1, qi::_2, qi::_3, false)];
 *
 * The expression in the square brackets is the semantic action. Here we see that the results of parsing are stored in the placeholders
 * qi::_1(keyword),  qi::_2(operation) and qi::_3(value).
 * All of them including "false" are used as the arguments for the "filterExpression" function. The result of this function is stored in qi::_val.
 * This is done to keep the result for the later usage. That's the only way to use the previous parsed results later,
 * otherwise the parsing process keeps iterating through the next symbols and previous results are lost.
 *
 *
 * 6. Nested expressions
 *
 * In Boost.Spirit, parsing of nested expressions is achieved through recursive grammar rules. This involves defining parsing rules in terms of themselves
 * or in terms of other rules that refer back to the original rule. This recursive structure allows for the parsing of nested constructs.
 * Here is a simple example to parse the arithmetic expression like this "(1 + (2 + 3))":
 *
 * qi::rule<Iterator, int(), qi::space_type> expression;
 * qi::rule<Iterator, int(), qi::space_type> factor;
 *
 * expression = factor >> *(qi::char_('+') >> factor);
 * factor = qi::int_ | ('(' >> expression >> ')');
 *
 *
 * 7. qi::phrase_parse function
 *
 * The parse function returns true or false depending on the result of the parse.
 * One overload of this function accepts five arguments:
 *
 * 1. An iterator pointing to the start of the input
 * 2. An iterator pointing to one past the end of the input
 * 3. The parser object
 * 4. Another parser called the skip parser. In our example, we wish to skip spaces and tabs. Another parser named space is included in Spirit's repertoire
 *    of predefined parsers. It is a very simple parser that simply recognizes whitespace. We will use space as our skip parser.
 * 5. Variable which stores the parsing result
 *
 * Example:
 * bool r = phrase_parse(
 *			first,			1
 *			last,			2
 *			*this,			3
 *			space,			4
 *			output			5
 * );
 *
 *
 * 8. Parser class definition
 *
 * The types of the local variables like qi::_1 should be also declared.This is done using the template arguments qi::locals.
 * See the example :
 * class ObjectTreeFilter : public qi::grammar<std::string::const_iterator,
 * 								std::function<bool(const model::ObjectTreeNode&)>(),
 * 								qi::space_type,
 * 								qi::locals<std::function<bool(const model::ObjectTreeNode&)>>> {
 * };
 *
 * The result of the parsing as well as the type of the arguments are also defined here using the template arguments :
 * std::function<bool(const model::ObjectTreeNode&)>()
 * The separator between the words here is default and defined to be a white space : qi::space_type
 *
 */

#pragma once

#include "object_tree_view/FilterResult.h"

#include "core/EditorObject.h"

#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator/self.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>

#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewSortProxyModels.h"

namespace raco::object_tree::view {

using EditorObject = core::EditorObject;
namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

class ObjectTreeFilter : public qi::grammar<std::string::const_iterator, std::function<bool(const model::ObjectTreeNode&)>(),
							 qi::space_type, qi::locals<std::function<bool(const model::ObjectTreeNode&)>>> {
public:
	ObjectTreeFilter();

	void setDefaultFilterKeyColumn(const int column);
	std::function<bool(const model::ObjectTreeNode&)> parse(const std::string& input, FilterResult& filterResult);

private:
	qi::rule<std::string::const_iterator, std::string(), qi::space_type> name, type, id, tag, keyword;
	qi::rule<std::string::const_iterator, std::string(), qi::space_type> equal, not_equal, operation;
	qi::rule<std::string::const_iterator, std::string(), qi::space_type> value, value_single_quoted, value_double_quoted;
	qi::rule<std::string::const_iterator, std::function<bool(const model::ObjectTreeNode&)>(), qi::space_type, qi::locals<std::function<bool(const model::ObjectTreeNode&)>>> expression, expression_no_key, expression_no_key_no_op;
	qi::rule<std::string::const_iterator, std::function<bool(const model::ObjectTreeNode&)>(), qi::space_type, qi::locals<std::function<bool(const model::ObjectTreeNode&)>>> expression_exact, expression_exact_no_key, expression_exact_no_key_no_op;
	qi::rule<std::string::const_iterator, std::function<bool(const model::ObjectTreeNode&)>(), qi::space_type, qi::locals<std::function<bool(const model::ObjectTreeNode&)>>> term, factor, start;

	inline static int defaultFilterKeyColumn_ = 0;

	std::string toLower(const std::string& input) const;
	void removeQuotes(std::string& s) const;
	static void removeTrailingSpaces(std::string& s);

	std::function<bool(const model::ObjectTreeNode&)> filterOR(
		const std::function<bool(const model::ObjectTreeNode&)>& lhs,
		const std::function<bool(const model::ObjectTreeNode&)>& rhs) const;
	std::function<bool(const model::ObjectTreeNode&)> filterAND(
		const std::function<bool(const model::ObjectTreeNode&)>& lhs,
		const std::function<bool(const model::ObjectTreeNode&)>& rhs) const;
	std::function<bool(const model::ObjectTreeNode&)> filterExpressionByDefaultKey(std::string op, std::string value, bool exact) const;
	std::function<bool(const model::ObjectTreeNode&)> filterExpression(std::string keyword, std::string op, std::string value, bool exact) const;
};

}  // namespace raco::object_tree::view
