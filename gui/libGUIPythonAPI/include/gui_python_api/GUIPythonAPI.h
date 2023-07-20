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

#include "object_tree_view/ObjectTreeDockManager.h"
#include "ramses_widgets/PreviewMainWindow.h"

namespace raco::gui_python_api {
	void setupObjectTree(object_tree::view::ObjectTreeDockManager* objTreeDockManager);
	void setupPreviewWindow(ramses_widgets::PreviewMainWindow* preview);
}
