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

#include "gui_python_api/GUIPythonAPI.h"
#include "object_tree_view/ObjectTreeDock.h"

#include "raco_pybind11_embed.h"

namespace {
	raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager;
	raco::ramses_widgets::PreviewMainWindow* previewWindow = nullptr;
	}

PYBIND11_EMBEDDED_MODULE(raco_gui, m) {
	m.def("getSelection", []() {
		return objectTreeDockManager->getSelection();
	});

	m.def("saveScreenshot", [](const std::string& fullPath) {
		if (previewWindow) {
			previewWindow->saveScreenshot(fullPath);
		} else {
			throw std::runtime_error("Could not save screenshot to \"" + fullPath + "\": no Preview window exists.");
		}
	});
}

namespace raco::gui_python_api {
	void setupObjectTree(object_tree::view::ObjectTreeDockManager* objTreeDockManager) {
		::objectTreeDockManager = objTreeDockManager;
	}

	void setupPreviewWindow(ramses_widgets::PreviewMainWindow* preview) {
		::previewWindow = preview;
	}
}
