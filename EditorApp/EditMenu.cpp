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

#include <QApplication>
#include <QShortcut>
#include <QMessageBox>

EditMenu::EditMenu(raco::application::RaCoApplication* racoApplication, QMenu* menu) : QObject{menu} {
	QObject::connect(menu, &QMenu::aboutToShow, this, [this, racoApplication, menu]() {
		auto* undoAction = menu->addAction("Undo");
		undoAction->setShortcut(QKeySequence::Undo);
		undoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canUndo());
		QObject::connect(undoAction, &QAction::triggered, this, [menu, racoApplication]() {
			try {
				racoApplication->activeRaCoProject().undoStack()->undo();
			} catch (raco::core::ExtrefError& error) {
				QMessageBox::warning(menu, "Undo Error", fmt::format("External reference update failed.\n\n{}", error.what()).c_str(), QMessageBox::Close);
			}
		});

		auto* redoAction = menu->addAction("Redo");
		redoAction->setShortcut(QKeySequence::Redo);
		redoAction->setEnabled(racoApplication->activeRaCoProject().undoStack()->canRedo());
		QObject::connect(redoAction, &QAction::triggered, this, [menu, racoApplication]() {
			try {
				racoApplication->activeRaCoProject().undoStack()->redo();
			} catch (raco::core::ExtrefError& error) {
				QMessageBox::warning(menu, "Undo Error", fmt::format("External reference update failed.\n\n{}", error.what()).c_str(), QMessageBox::Close);
			}  
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

		if (!QObject::connect(copyAction, SIGNAL(triggered()), QApplication::focusWidget(), SLOT(copy()))) {
			copyAction->setDisabled(true);
		}
		if (!QObject::connect(pasteAction, SIGNAL(triggered()), QApplication::focusWidget(), SLOT(paste()))) {
			pasteAction->setDisabled(true);
		}
	});
	QObject::connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
		while (menu->actions().size() > 0) {
			menu->removeAction(menu->actions().at(0));
		}
		sub_ = raco::components::Subscription{};
	});
}

