/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeDockManager_test.h"

#include "object_tree_view/ObjectTreeDock.h"

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