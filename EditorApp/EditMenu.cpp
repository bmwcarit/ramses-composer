/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditMenu.h"

#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeDockManager.h"

#include <QShortcut>

EditMenu::EditMenu(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager, QMenu* menu) : QObject{menu} {
	QObject::connect(menu, &QMenu::aboutToShow, this, [this, racoApplication, objectTreeDockManager, menu]() {
		auto* undoAction = menu->addAction("Undo");
		undoAction->setShortcut(QKeySequence::Undo);
		undoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canUndo());
		QObject::connect(undoAction, &QAction::triggered, this, [menu, racoApplication]() {
			globalUndoCallback(racoApplication);
		});

		auto* redoAction = menu->addAction("Redo");
		redoAction->setShortcut(QKeySequence::Redo);
		redoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canRedo());
		QObject::connect(redoAction, &QAction::triggered, this, [menu, racoApplication]() {
			globalRedoCallback(racoApplication);
		});

		sub_ = racoApplication->dataChangeDispatcher()->registerOnUndoChanged([racoApplication, undoAction, redoAction]() {
			redoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canRedo());
			undoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canUndo());
		});
	});
	QObject::connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
		while (menu->actions().size() > 0) {
			menu->removeAction(menu->actions().at(0));
		}
		sub_ = raco::components::Subscription{};
	});
}

void EditMenu::globalUndoCallback(raco::application::RaCoApplication* racoApplication) {
	if (racoApplication->activeRaCoProject().undoStack()->canUndo()) {
		racoApplication->activeRaCoProject().undoStack()->undo();
	}
}

void EditMenu::globalRedoCallback(raco::application::RaCoApplication* racoApplication) {
	if (racoApplication->activeRaCoProject().undoStack()->canRedo()) {
		racoApplication->activeRaCoProject().undoStack()->redo();
	}
}
