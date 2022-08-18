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

#include <QDialog>

namespace Ui {
class SavedLayoutsDialog;
}


class RaCoDockManager;

class QStringListModel;

class SavedLayoutsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SavedLayoutsDialog(RaCoDockManager *dockManager, QWidget *parent = nullptr);
	~SavedLayoutsDialog();

protected:
	void deleteSelectedLayout();
	void updateButtons();

	void keyPressEvent(QKeyEvent *event) override;

	Ui::SavedLayoutsDialog *ui;
	RaCoDockManager *dockManager_;
	QStringListModel *model_;
};
