/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeFilter.h"

using namespace raco::object_tree::view;
using namespace raco;

ObjectTreeFilter::ObjectTreeFilter() : ObjectTreeFilter::base_type(start) {
	// keywords
	name = qi::lit("name");
	type = qi::lit("type");
	id = qi::lit("id");
	tag = qi::lit("tag");
	keyword = name | type | id | tag;

	// operations
	equal = qi::lit("=");
	not_equal = qi::lit("!=");
	operation = equal | not_equal;

	// values
	value_single_quoted = qi::lexeme['\'' >> +(qi::char_ - '\'') >> '\''];
	value_double_quoted = qi::lexeme['\"' >> +(qi::char_ - '\"') >> '\"'];
	value = qi::lexeme[+(qi::char_ - '(' - ')' - '|' - '&' - '!' - '=' - '\'' - '\"' - ' ')] | value_single_quoted;
	
	// expressions
	expression = (keyword >> operation >> value)
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpression, this, qi::_1, qi::_2, qi::_3, false)];
	expression_exact = (keyword >> operation >> value_double_quoted)
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpression, this, qi::_1, qi::_2, qi::_3, true)];

	expression_no_key = (operation >> value)
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpressionByDefaultKey, this, qi::_1, qi::_2, false)];
	expression_exact_no_key = (operation >> value_double_quoted)
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpressionByDefaultKey, this, qi::_1, qi::_2, true)];

	expression_no_key_no_op = value
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpressionByDefaultKey, this, "=", qi::_1, false)];
	expression_exact_no_key_no_op = value_double_quoted
		[qi::_val = phx::bind(&ObjectTreeFilter::filterExpressionByDefaultKey, this, "=", qi::_1, true)];

	// factor / term / start
	factor = (expression_exact | expression_exact_no_key | expression_exact_no_key_no_op) |
			  (expression | expression_no_key | expression_no_key_no_op) | ('(' >> start >> ')');

	term = factor[qi::_val = qi::_1] >> *('&' >> factor[qi::_val = phx::bind(&ObjectTreeFilter::filterAND, this, qi::_val, qi::_1)]);
	
	start = term[qi::_val = qi::_1] >> *(-qi::lit('|') >> term[qi::_val = phx::bind(&ObjectTreeFilter::filterOR, this, qi::_val, qi::_1)]);
}

std::string ObjectTreeFilter::toLower(const std::string& input) const {
	std::string lowered = input;
	std::transform(input.begin(), input.end(), lowered.begin(), [](unsigned char c) { return std::tolower(c); });
	return lowered;
}

void ObjectTreeFilter::removeQuotes(std::string& s) const {
	s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
}
	
	

void ObjectTreeFilter::removeTrailingSpaces(std::string& s) {
	std::size_t pos = s.find_last_not_of(' ');
	if (pos != std::string::npos) {
		s.erase(pos + 1);
	} else {
		// The string only contains spaces or is empty
		s.clear();
	}
}

std::function<bool(const object_tree::model::ObjectTreeNode&)> ObjectTreeFilter::filterOR(
	const std::function<bool(const model::ObjectTreeNode&)>& lhs,
	const std::function<bool(const model::ObjectTreeNode&)>& rhs) const {
	// Define a lambda function for the filtering logic
	auto filterFunc = [lhs, rhs](const model::ObjectTreeNode& objectTreeNode) {
		return lhs(objectTreeNode) || rhs(objectTreeNode);
	};

	return filterFunc;
}

std::function<bool(const object_tree::model::ObjectTreeNode&)> ObjectTreeFilter::filterAND(
	const std::function<bool(const model::ObjectTreeNode&)>& lhs,
	const std::function<bool(const model::ObjectTreeNode&)>& rhs) const {
	// Define a lambda function for the filtering logic
	auto filterFunc = [lhs, rhs](const model::ObjectTreeNode& objectTreeNode) {
		return lhs(objectTreeNode) && rhs(objectTreeNode);
	};

	return filterFunc;
}

std::function<bool(const object_tree::model::ObjectTreeNode&)> ObjectTreeFilter::filterExpressionByDefaultKey(std::string op, std::string value, bool exact) const {
	switch (defaultFilterKeyColumn_) {
		case model::ObjectTreeViewDefaultModel::ColumnIndex::COLUMNINDEX_NAME: {
			return filterExpression("name", op, value, exact);
		}
		case model::ObjectTreeViewDefaultModel::ColumnIndex::COLUMNINDEX_TYPE: {
			return filterExpression("type", op, value, exact);
		}
		case model::ObjectTreeViewDefaultModel::ColumnIndex::COLUMNINDEX_ID: {
			return filterExpression("id", op, value, exact);
		}
		case model::ObjectTreeViewDefaultModel::ColumnIndex::COLUMNINDEX_USERTAGS: {
			return filterExpression("tag", op, value, exact);
		}
		default: {
			return {};
		}
	}
}

std::function<bool(const object_tree::model::ObjectTreeNode&)> ObjectTreeFilter::filterExpression(std::string keyword, std::string op, std::string value, bool exact) const {
	// Remove the single quotes from the parsed value if present
	removeQuotes(value);
	removeTrailingSpaces(value);

	// Define a lambda function for the filtering logic
	auto filterFunc = [this, keyword, op, value, exact](const model::ObjectTreeNode& objectTreeNode) {
		if (keyword == "tag") {
			const auto tags = objectTreeNode.getUserTags();
			if (op == "=" && exact == true) {
				return std::any_of(tags.begin(), tags.end(), [this, &value](const std::string& tag) {
					return toLower(tag) == toLower(value);
				});
			}
			if (op == "!=" && exact == true) {
				return std::all_of(tags.begin(), tags.end(), [this, &value](const std::string& tag) {
					return toLower(tag) != toLower(value);
				});
			}
			if (op == "=") {
				return std::any_of(tags.begin(), tags.end(), [this, &value](const std::string& tag) {
					return toLower(tag).find(toLower(value)) != std::string::npos;
				});
			}
			if (op == "!=") {
				return std::all_of(tags.begin(), tags.end(), [this, &value](const std::string& tag) {
					return toLower(tag).find(toLower(value)) == std::string::npos;
				});
			}
		} else {
			std::string objValue;
			if (keyword == "name") objValue = objectTreeNode.getDisplayName();
			if (keyword == "type") objValue = objectTreeNode.getDisplayType();
			if (keyword == "id") objValue = objectTreeNode.getID();

			if (!objValue.empty()) {
				if (op == "=" && exact == true) {
					return toLower(objValue) == toLower(value);
				}
				if (op == "!=" && exact == true) {
					return toLower(objValue) != toLower(value);
				}
				if (op == "=") {
					return toLower(objValue).find(toLower(value)) != std::string::npos;
				}
				if (op == "!=") {
					return toLower(objValue).find(toLower(value)) == std::string::npos;
				}
			}
		}
		
		return false;
	};

	return filterFunc;
}

void ObjectTreeFilter::setDefaultFilterKeyColumn(const int column) {
	defaultFilterKeyColumn_ = column;
}

std::function<bool(const object_tree::model::ObjectTreeNode&)> ObjectTreeFilter::parse(const std::string& input, FilterResult& filterResult) {
	std::function<bool(const model::ObjectTreeNode&)> output{};
	filterResult = FilterResult::Failed;

	try {
		std::string::const_iterator iterator = input.begin();
		std::string::const_iterator end = input.end();

		if (phrase_parse(iterator, end, *this, qi::space, output)) {
			if (iterator == end) {
				filterResult = FilterResult::Success;
			} else {
				filterResult = FilterResult::PartialSuccess;
			}
		}
	} catch (...) {
	}

	return output;
}



