/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "application/RaCoApplication.h"
#include <QMenu>
#include <QObject>

namespace raco::object_tree::view {
class ObjectTreeDockManager;
}

/**
 * Class Handeling the state of the edit menu in the menuBar.
 * TODO: if we at some point get rid of the ui files this should be made into an actual menu.
 */ 
class EditMenu final : public QObject {
	Q_OBJECT
public:
	explicit EditMenu(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager, QMenu* menu);

    static void globalUndoCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager);
    static void globalRedoCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager);
	static void globalCopyCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager);
	static void globalPasteCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager);

private:
	raco::components::Subscription sub_;
};
