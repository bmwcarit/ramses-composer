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

#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "core/Handles.h"
#include "gui_python_api/GUIPythonAPI.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "raco_pybind11_embed.h"

namespace {
	raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager;
}

PYBIND11_EMBEDDED_MODULE(raco_gui, m) {
	m.def("getSelection", []() {
		auto activeDockWhichHasSelection = objectTreeDockManager->getActiveDockWithSelection();
		if (activeDockWhichHasSelection == nullptr || activeDockWhichHasSelection->windowTitle().toStdString() == "Project Browser") {
			return std::vector<raco::core::SEditorObject>();
		}

		return activeDockWhichHasSelection->getActiveTreeView()->getSortedSelectedEditorObjects();
	});
}

namespace raco::gui_python_api {
	void setup(raco::object_tree::view::ObjectTreeDockManager* objTreeDockManager) {
		::objectTreeDockManager = objTreeDockManager;
	}
}
