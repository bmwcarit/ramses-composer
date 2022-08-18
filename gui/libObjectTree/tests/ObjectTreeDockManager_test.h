/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include "gtest/gtest.h"
#include "components/DataChangeDispatcher.h"

#include "object_tree_view/ObjectTreeDockManager.h"
#include <QApplication>
#include <testing/TestEnvironmentCore.h>

class ObjectDefaultTreeDockManagerTest : public TestEnvironmentCore {
protected:
	raco::object_tree::view::ObjectTreeDockManager manager_;
	int argc = 0;
	QApplication fakeApp_{argc, nullptr};	
	raco::components::SDataChangeDispatcher dispatcher_{std::make_shared<raco::components::DataChangeDispatcher>()};

	std::unique_ptr<raco::object_tree::view::ObjectTreeDock> generateDockInManager() {
		auto dockName = std::string("Dock").append(std::to_string(manager_.getTreeDockAmount()));
		auto newDock = std::make_unique<raco::object_tree::view::ObjectTreeDock>(dockName.c_str());

		manager_.addTreeDock(newDock.get());

		return newDock;
	}

	
};