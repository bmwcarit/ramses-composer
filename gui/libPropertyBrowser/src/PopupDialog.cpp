/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PopupDialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QWindow>

namespace raco::property_browser {

	PopupDialog::PopupDialog(QWidget* anchor, QString title) : QDialog{anchor}, anchor_(anchor) {
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowFlag(Qt::CustomizeWindowHint, true);
		setWindowFlag(Qt::WindowContextHelpButtonHint, false);
		setWindowFlag(Qt::WindowSystemMenuHint, false);
		setWindowFlag(Qt::WindowCloseButtonHint, false);		
		setWindowTitle(title);
		setSizeGripEnabled(true);
		qGuiApp->installEventFilter(this);
	}

	void PopupDialog::showCenteredOnAnchor() {
		// center horizontally on edit button and keep on screen
		updateGeometry();
		const QPoint parentLocation = anchor_->mapToGlobal(anchor_->geometry().topLeft());
		const QSize size = sizeHint();
		const QRect screen = QApplication::desktop()->screenGeometry(anchor_);
		int x = std::min(parentLocation.x() - size.width() / 2, screen.x() + screen.width() - size.width());
		int y = std::min(parentLocation.y(), screen.height() - size.height());
		move(x, y);

		show();
	}

	bool PopupDialog::eventFilter(QObject* watched, QEvent* event) {
		if (event->type() == QEvent::Type::MouseButtonPress) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			if (anchor_->window()->frameGeometry().contains(mouseEvent->globalPos()) && !frameGeometry().contains(mouseEvent->globalPos())) {
				close();
				return true;
			}			
		}
		return QDialog::eventFilter(watched, event);
		
	}

}