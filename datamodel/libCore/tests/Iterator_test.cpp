/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"
#include "core/Handles.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/MeshCacheInterface.h"
#include "user_types/Node.h"

#include "user_types/UserObjectFactory.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;

TEST(IteratorTest, ObjectTree) {
	SNode node{new Node("node")};
	SNode child_a(new Node("child a"));

	std::vector<SEditorObject> children;
	for (auto child : *node) {
		children.push_back(child);
	}
	EXPECT_EQ(children.size(), 0);

	children.clear();
	std::copy(node->begin(), node->end(), std::back_inserter(children));
	EXPECT_EQ(children.size(), 0);

	children.clear();
	for (auto child : TreeIteratorAdaptor(node)) {
		children.push_back(child);
	}
	EXPECT_EQ(children, std::vector<SEditorObject>({node}));

	ValueBase* prop = node->children_->addProperty();
	*prop = child_a;

	children.clear();
	for (auto child : TreeIteratorAdaptor(node)) {
		children.push_back(child);
	}
	EXPECT_EQ(children, std::vector<SEditorObject>({node, child_a}));
}

TEST(TreeIteratorAdaptor, copy_noChildren_shouldNotFail) {
	SEditorObject foo{new Node("foo")};
	std::vector<SEditorObject> result{};
	std::copy(TreeIteratorAdaptor(foo).begin(), TreeIteratorAdaptor(foo).end(), std::back_inserter(result));
	EXPECT_TRUE(true);
}

TEST(TreeIteratorAdaptor, forEach_noChildren_shouldNotFail) {
	SEditorObject foo{new Node("foo")};
	std::vector<SEditorObject> result{};
	for (const SEditorObject &obj : TreeIteratorAdaptor(foo)) {
	}
	EXPECT_TRUE(true);
}

TEST(IteratorTest, Property) {
	std::shared_ptr<Node> node{new Node()};

	std::vector<ValueHandle> handles;
	for (auto prop : ValueTreeIteratorAdaptor(ValueHandle(node))) {
		handles.emplace_back(prop);
	}
	std::vector<ValueHandle> refHandles{
		{node, {"objectID"}},
		{node, {"objectName"}},
		{node, {"children"}},
		{node, {"userTags"}},
		{node, {"metaData"}},
		{node, {"tags"}},
		{node, {"visibility"}},
		{node, {"enabled"}},
		{node, {"translation"}},
		{node, {"translation", "x"}},
		{node, {"translation", "y"}},
		{node, {"translation", "z"}},
		{node, {"rotation"}},
		{node, {"rotation", "x"}},
		{node, {"rotation", "y"}},
		{node, {"rotation", "z"}},
		{node, {"scaling"}},
		{node, {"scaling", "x"}},
		{node, {"scaling", "y"}},
		{node, {"scaling", "z"}},
		{node, {"editorVisibility"}}};
	EXPECT_EQ(handles, refHandles);

	handles.clear();
	ValueHandle translation{node, {"translation"}};
	std::copy(ValueTreeIteratorAdaptor(translation).begin(), ValueTreeIteratorAdaptor(translation).end(), std::back_inserter(handles));

	std::vector<ValueHandle> refTranslationHandles{
		{node, {"translation", "x"}},
		{node, {"translation", "y"}},
		{node, {"translation", "z"}}};
	EXPECT_EQ(handles, refTranslationHandles);

	handles.clear();
	ValueHandle visible{node, {"visibility"}};
	std::copy(ValueTreeIteratorAdaptor(visible).begin(), ValueTreeIteratorAdaptor(visible).end(), std::back_inserter(handles));
	EXPECT_EQ(handles.size(), 0);

	handles.clear();
	ValueHandle children{node, {"children"}};
	std::copy(ValueTreeIteratorAdaptor(children).begin(), ValueTreeIteratorAdaptor(children).end(), std::back_inserter(handles));
	EXPECT_EQ(handles.size(), 0);
}
