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

#include <QKeyEvent>
#include <QTreeView>

namespace raco::property_browser {

class TreeViewWithDel : public QTreeView {
	Q_OBJECT
public:
	TreeViewWithDel(QWidget* parent) : QTreeView(parent) {}

Q_SIGNALS:
	void deletePressed(QModelIndex const& index);

protected:
	void keyReleaseEvent(QKeyEvent* event) override {
		if (event->key() == Qt::Key::Key_Delete && currentIndex().isValid()) {
			Q_EMIT deletePressed(currentIndex());
			event->accept();
		}
		QTreeView::keyReleaseEvent(event);
	}
};

}
