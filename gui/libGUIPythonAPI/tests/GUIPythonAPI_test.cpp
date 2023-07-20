/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined(WIN32) && defined(_DEBUG) && defined(PYTHON_DEBUG_LIBRARY_AVAILABLE)
// Needed to avoid pybind11/embed.h to cause linking to the non-debug DLL if the debug DLL is available.
// See https://github.com/pybind/pybind11/issues/3403#issuecomment-962878324
#define Py_DEBUG
#endif

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
			objectTreeDockManager = new raco::object_tree::view::ObjectTreeDockManager();
			objectTreeDock = std::make_unique<raco::object_tree::view::ObjectTreeDock>("Dock");
			objectTreeDockManager->addTreeDock(objectTreeDock.get());
			objectTreeViewModel = new raco::object_tree::model::ObjectTreeViewDefaultModel(&commandInterface(), application.dataChangeDispatcher(), nullptr, {raco::user_types::Node::typeDescription.typeName});
			objectTreeDock->setTreeView(new raco::object_tree::view::ObjectTreeView("TreeView", objectTreeViewModel));

			raco::python_api::preparePythonEnvironment(QCoreApplication::applicationFilePath().toStdWString(), {}, true);
			pyGuard = std::make_unique<py::scoped_interpreter>();
			raco::python_api::setup(&application);
			raco::gui_python_api::setupObjectTree(objectTreeDockManager);
		}

		raco::object_tree::model::ObjectTreeViewDefaultModel* objectTreeViewModel;
		std::unique_ptr<raco::object_tree::view::ObjectTreeDock> objectTreeDock;
		raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager;

		std::unique_ptr<py::scoped_interpreter> pyGuard;

		int argc = 0;
		QApplication fakeApp_{argc, nullptr};

		void dispatch() {
			application.dataChangeDispatcher()->dispatch(recorder().release());
		}
	
		std::string createNode(std::string name) {
			auto obj = objectTreeViewModel->createNewObject(raco::user_types::Node::typeDescription.typeName, name);
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
	objectTreeDock->getActiveTreeView()->selectObject(QString::fromStdString(node2Id));

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
