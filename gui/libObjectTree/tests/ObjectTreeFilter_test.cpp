/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <qstandarditemmodel.h>
#include "gtest/gtest.h"
#include "ObjectTreeFilter_test.h"
#include "object_tree_view/ObjectTreeFilter.h"


using namespace object_tree::model;
using namespace raco::core;

TEST_P(ObjectTreeFilterTestFixture, ParametricParsingTest) {
	using FilterResult = object_tree::view::FilterResult;
	const auto valid = std::get<0>(GetParam());
	const auto input = std::get<1>(GetParam());
	const auto ids = std::get<2>(GetParam());

	FilterResult filterResult = input.isEmpty() ? FilterResult::Success : FilterResult::Failed;
	object_tree::view::ObjectTreeFilter filter;
	const auto filterFunction = filter.parse(input.toStdString(), filterResult);

	ASSERT_EQ(filterFunction != nullptr, valid);
	proxyModel_.setCustomFilter(filterFunction);

	QStringList parsedIds{};
	for (int row = 0; row < proxyModel_.rowCount(); ++row) {
		const auto idIndex = proxyModel_.index(row, ObjectTreeViewDefaultModel::ColumnIndex::COLUMNINDEX_ID);
		const auto id = proxyModel_.getDataAtIndex(idIndex);
		
		parsedIds.append(id);
	}
	EXPECT_EQ(parsedIds, ids);
}

// Define the parameter values for the test cases
INSTANTIATE_TEST_SUITE_P(ParametricTests, ObjectTreeFilterTestFixture,
	::testing::Values(
		// Pair: Parsing string / IDs list
		std::make_tuple(true, QString("name = \"name1\" | type != \"Node\""), QStringList({"id_01", "id_02", "id_03", "id_04", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name = \"name1\" | type != \"Node\" & name != \"name2\""), QStringList({"id_01", "id_02", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = \"name1\" | type != \"Node\") & name != \"name2\""), QStringList({"id_01", "id_02", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = \"name1\" & type != \"Node\") & name != \"name4\""), QStringList({"id_02"})),
		std::make_tuple(true, QString("(name = \"name1\" & (type != \"Node\")) & name != \"name4\""), QStringList({"id_02"})),

		std::make_tuple(true, QString("(name = \"name1\" & (type != \"Node\")) & tag != tag1"), QStringList({"id_02"})),
		std::make_tuple(true, QString("(name = \"name1\" & (type != \"Node\")) | tag != tag1"), QStringList({"id_02", "id_03", "id_04", "id_06", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = \"name1\" | (tag != tag1))"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_06", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name != \"name1\" & tag != tag3)"), QStringList({"id_04", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name != \"name1\" & id != \"id_04\""), QStringList({"id_03", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"})),

		std::make_tuple(true, QString("name != \"name1\" & id != \"id_04\""), QStringList({"id_03", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name != \"name3\" & (type = \"Node\") & tag != tag1"), QStringList({})),
		std::make_tuple(true, QString("(name != \"name3\" & (type = \"Node\")) | tag != tag1"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_06", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name != \"name3\" | type = \"Node\") & tag != tag1"), QStringList({"id_02", "id_03", "id_04", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name != \"name3\" | (type = \"Node\")) | (tag != tag1)"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"})),

		std::make_tuple(true, QString("name != \"name3\" | type = \"Node\" | (tag != tag1)"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name = name1 | type != \"Node\""), QStringList({"id_01", "id_02", "id_03", "id_04", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name = name1 | type != Node"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name = name1 | type != \"Node\" & name != \"name2\""), QStringList({"id_01", "id_02", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("name = name1 | type != Node & name != \"name2\""), QStringList({"id_01", "id_02", "id_07", "id_08", "id_09", "id_10"})),

		std::make_tuple(true, QString("name = name1 | type != Node & name != name2"), QStringList({"id_01", "id_02", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = name1 | type != \"Node\") & name != \"name2\""), QStringList({"id_01", "id_02", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = name1 & type != \"Node\") & name != name4"), QStringList({"id_02"})),
		std::make_tuple(true, QString("(name = name1 & (type != Node)) & name != name4"), QStringList({})),
		std::make_tuple(true, QString("(name = name1 & (type != \"Node\")) & tag != \"tag1\""), QStringList({"id_02"})),

		std::make_tuple(true, QString("(name = name1 & (type != Node)) | tag != \"tag1\""), QStringList({"id_02", "id_03", "id_04", "id_06", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = name1 & (type != Node)) | tag = \"tag1\""), QStringList({"id_01", "id_05", "id_07", "id_08"})),
		std::make_tuple(false, QString("(name = \"name5 (1)\" & (type ! Node)) | tag = 3"), QStringList({"id_01", "id_02", "id_03", "id_04", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"})),
		std::make_tuple(true, QString("(name = '(1)' & (type != Node)) | tag = 'tag7 (3)'"), QStringList({"id_09", "id_10"})))
);
