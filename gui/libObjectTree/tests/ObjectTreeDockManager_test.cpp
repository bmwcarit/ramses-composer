/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeDockManager_test.h"

#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"

using namespace raco::object_tree::view;
TEST_F(ObjectDefaultTreeDockManagerTest, DockCachingEmpty) {
	ASSERT_EQ(manager_.getTreeDockAmount(), 0);
}

TEST_F(ObjectDefaultTreeDockManagerTest, DockCachingOneDock) {
	auto dockName = "Simple Dock";
	auto dockPtr = std::make_unique<ObjectTreeDock>("Simple Dock");
	manager_.addTreeDock(dockPtr.get());

	ASSERT_EQ(manager_.getTreeDockAmount(), 1);
}

TEST_F(ObjectDefaultTreeDockManagerTest, DockCachingTwoDocks) {
	auto firstDock = generateDockInManager();
	auto secondDock = generateDockInManager();

	ASSERT_EQ(manager_.getTreeDockAmount(), 2);
}

TEST_F(ObjectDefaultTreeDockManagerTest, DockRemovalTwoDocksOneRemovedBySimulatedClosing) {
	auto firstDock = generateDockInManager();
	auto secondDock = generateDockInManager();

	firstDock->Q_EMIT dockClosed(firstDock.get());

	ASSERT_EQ(manager_.getTreeDockAmount(), 1);
}

TEST_F(ObjectDefaultTreeDockManagerTest, DockRemovalTwoDocksOneRemovedByDeallocation) {
	auto firstDock = generateDockInManager();
	{
		auto secondDock = generateDockInManager();
	}

	ASSERT_EQ(manager_.getTreeDockAmount(), 1);
}

TEST_F(ObjectDefaultTreeDockManagerTest, DockWithUnselectedItemIsNotConsideredSelectedAnymore) {
	auto firstDock = generateDockInManager();
	auto model = new raco::object_tree::model::ObjectTreeViewDefaultModel(&commandInterface, dispatcher_, nullptr, {raco::user_types::Node::typeDescription.typeName});
	firstDock->addTreeView(new raco::object_tree::view::ObjectTreeView("TreeView", model));
	auto obj = model->createNewObject(raco::user_types::Node::typeDescription, "name");
	dispatcher_->dispatch(recorder);
	firstDock->getCurrentlyActiveTreeView()->selectObject(QString::fromStdString(obj->objectID()));
	ASSERT_TRUE(manager_.getActiveDockWithSelection() == firstDock.get());

	commandInterface.deleteObjects({obj});
	dispatcher_->dispatch(recorder);
	model->buildObjectTree();
	ASSERT_TRUE(manager_.getActiveDockWithSelection() == nullptr);
}