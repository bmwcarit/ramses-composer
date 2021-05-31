/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "SavedLayoutsDialog.h"
#include "ui_SavedLayoutsDialog.h"

#include "RaCoDockManager.h"

#include <QStringListModel>

SavedLayoutsDialog::SavedLayoutsDialog(RaCoDockManager *dockManager, QWidget *parent) : QDialog(parent), ui(new Ui::SavedLayoutsDialog), dockManager_(dockManager), model_(new QStringListModel(this)) {
	ui->setupUi(this);

	auto layoutNames = dockManager_->perspectiveNames();
	model_->setStringList(layoutNames);
	ui->layoutView->setModel(model_);
	ui->layoutView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	updateButtons();

	connect(ui->deleteButton, &QPushButton::clicked, [this]() {
		deleteSelectedLayout();
	});
}

SavedLayoutsDialog::~SavedLayoutsDialog() {
	delete ui;
}

void SavedLayoutsDialog::deleteSelectedLayout() {
	auto currentIndex = ui->layoutView->currentIndex();
	dockManager_->removeCustomLayout(currentIndex.data().toString());
	model_->removeRows(currentIndex.row(), 1);
	updateButtons();
}

void SavedLayoutsDialog::updateButtons() {
	ui->deleteButton->setDisabled(model_->rowCount() == 0);
}

void SavedLayoutsDialog::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Delete && ui->layoutView->currentIndex().isValid()) {
		deleteSelectedLayout();
		return;
	}
	QDialog::keyPressEvent(event);
}
