/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditMenu.h"

#include "common_widgets/RaCoClipboard.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "object_tree_view/ObjectTreeView.h"

#include <QApplication>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QMessageBox>

EditMenu::EditMenu(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager, QMenu* menu) : QObject{menu} {
	QObject::connect(menu, &QMenu::aboutToShow, this, [this, racoApplication, objectTreeDockManager, menu]() {
		auto* undoAction = menu->addAction("Undo");
		undoAction->setShortcut(QKeySequence::Undo);
		undoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canUndo());
        QObject::connect(undoAction, &QAction::triggered, this, [menu, racoApplication, objectTreeDockManager]() {
            globalUndoCallback(racoApplication, objectTreeDockManager);
		});

		auto* redoAction = menu->addAction("Redo");
		redoAction->setShortcut(QKeySequence::Redo);
		redoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canRedo());
        QObject::connect(redoAction, &QAction::triggered, this, [menu, racoApplication, objectTreeDockManager]() {
            globalRedoCallback(racoApplication, objectTreeDockManager);
		});

		sub_ = racoApplication->dataChangeDispatcher()->registerOnUndoChanged([racoApplication, undoAction, redoAction]() {
			redoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canRedo());
			undoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canUndo());
		});

		menu->addSeparator();

		auto* copyAction = menu->addAction("Copy");
		copyAction->setShortcut(QKeySequence::Copy);
		auto* pasteAction = menu->addAction("Paste");
		pasteAction->setShortcut(QKeySequence::Paste);
		auto activeObjectTreeDockWithSelection = objectTreeDockManager->getActiveDockWithSelection();
		if (!activeObjectTreeDockWithSelection) {
			copyAction->setEnabled(false);
			pasteAction->setEnabled(raco::RaCoClipboard::hasEditorObject());
		} else {
			auto focusedTreeView = activeObjectTreeDockWithSelection->getCurrentlyActiveTreeView();
			auto selectedIndices = focusedTreeView->getSelectedIndices();
			auto pasteIndex = focusedTreeView->getSelectedInsertionTargetIndex();
			copyAction->setEnabled(focusedTreeView->canCopyAtIndices(selectedIndices));
			pasteAction->setEnabled(focusedTreeView->canPasteIntoIndex(pasteIndex, false) || focusedTreeView->canPasteIntoIndex({}, false));
		}

		QObject::connect(copyAction, &QAction::triggered, [racoApplication, objectTreeDockManager]() {
			globalCopyCallback(racoApplication, objectTreeDockManager);
		});

		QObject::connect(pasteAction, &QAction::triggered, [racoApplication, objectTreeDockManager]() {
			globalPasteCallback(racoApplication, objectTreeDockManager);
		});
	});
	QObject::connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
		while (menu->actions().size() > 0) {
			menu->removeAction(menu->actions().at(0));
		}
		sub_ = raco::components::Subscription{};
	});
}

void EditMenu::globalUndoCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager *objectTreeDockManager) {
    if (racoApplication->activeRaCoProject().undoStack()->canUndo()) {
        if (racoApplication->activeRaCoProject().undoStack()->curIndexIsOpreatObject()) {
            Q_EMIT signalProxy::GetInstance().sigReLoadNodeData();
        }
        racoApplication->activeRaCoProject().undoStack()->undo();
    }
}

void EditMenu::globalRedoCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager *objectTreeDockManager) {
    if (racoApplication->activeRaCoProject().undoStack()->canRedo()) {
        racoApplication->activeRaCoProject().undoStack()->redo();
        if (racoApplication->activeRaCoProject().undoStack()->curIndexIsOpreatObject()) {
            Q_EMIT signalProxy::GetInstance().sigReLoadNodeData();
        }
    }
}

void EditMenu::globalCopyCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager) {
	if (auto activeObjectTreeDockWithSelection = objectTreeDockManager->getActiveDockWithSelection()) {
		auto focusedTreeView = activeObjectTreeDockWithSelection->getCurrentlyActiveTreeView();
        focusedTreeView->globalCopyCallback();
	}
}

void EditMenu::globalPasteCallback(raco::application::RaCoApplication* racoApplication, raco::object_tree::view::ObjectTreeDockManager* objectTreeDockManager) {
	if (auto activeObjectTreeDockWithSelection = objectTreeDockManager->getActiveDockWithSelection()) {
		auto focusedTreeView = activeObjectTreeDockWithSelection->getCurrentlyActiveTreeView();

		focusedTreeView->globalPasteCallback(focusedTreeView->getSelectedInsertionTargetIndex());
	} else {
		auto copiedObjs = raco::RaCoClipboard::get();
		racoApplication->activeRaCoProject().commandInterface()->pasteObjects(copiedObjs);
	}
}
