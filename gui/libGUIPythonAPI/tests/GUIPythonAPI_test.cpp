/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <raco_pybind11_embed.h>

#include <gtest/gtest.h>

#include "testing/RaCoApplicationTest.h"
#include <QApplication>

#include "application/RaCoApplication.h"
#include "gui_python_api/GUIPythonAPI.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "python_api/PythonAPI.h"
#include "ramses_adaptor/SceneBackend.h"

namespace py = pybind11;

class GUIPythonTest : public RaCoApplicationTest {
	public:
		GUIPythonTest() {
			objectTreeDockManager = new object_tree::view::ObjectTreeDockManager();
			objectTreeDock = std::make_unique<object_tree::view::ObjectTreeDock>("Dock");
			objectTreeDockManager->addTreeDock(objectTreeDock.get());
			objectTreeViewModel = new object_tree::model::ObjectTreeViewDefaultModel(&commandInterface(), application.dataChangeDispatcher(), nullptr, {user_types::Node::typeDescription.typeName});
			objectTreeDock->setTreeView(new object_tree::view::ObjectTreeView("TreeView", objectTreeViewModel));

			python_api::preparePythonEnvironment(QCoreApplication::applicationFilePath().toStdWString(), {}, true);
			pyGuard = std::make_unique<py::scoped_interpreter>();
			python_api::setApp(&application);
			python_api::importRaCoModule();
			gui_python_api::setupObjectTree(objectTreeDockManager);
		}

		object_tree::model::ObjectTreeViewDefaultModel* objectTreeViewModel;
		std::unique_ptr<object_tree::view::ObjectTreeDock> objectTreeDock;
		object_tree::view::ObjectTreeDockManager* objectTreeDockManager;

		std::unique_ptr<py::scoped_interpreter> pyGuard;

		int argc = 0;
		QApplication fakeApp_{argc, nullptr};

		void dispatch() {
			application.dataChangeDispatcher()->dispatch(recorder().release());
		}
	
		std::string createNode(std::string name) {
			auto obj = objectTreeViewModel->createNewObject(user_types::Node::typeDescription.typeName, name);
			dispatch();
			return obj->objectID();
		}
	};

TEST_F(GUIPythonTest, import_raco_gui) {
	py::exec(R"(
import raco_gui
print("Hello world from Python!")
)");
}

TEST_F(GUIPythonTest, get_selection_empty_selection) {
	py::exec(R"(
import unittest
import raco
import raco_gui

class test_class(unittest.TestCase):
    def test(self):
        selection = raco_gui.getSelection()
        self.assertEqual(0, len(selection))

instance = test_class()
instance.test()
)");
}

TEST_F(GUIPythonTest, get_selection_single_selection) {
	createNode("node1");
	auto node2Id = createNode("node2");
	createNode("node3");
	objectTreeDock->getActiveTreeView()->selectObject(QString::fromStdString(node2Id), false);

	py::exec(R"(
import unittest
import raco
import raco_gui

class test_class(unittest.TestCase):
    def test(self):
        selection = raco_gui.getSelection()
        self.assertEqual(1, len(selection))
        self.assertEqual("node2", selection[0].objectName.value())

instance = test_class()
instance.test()
)");
}
