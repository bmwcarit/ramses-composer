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

#include "property_browser/ObjectSearchView.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace raco::property_browser {
class PropertyBrowserItem;


class PropertyBrowserEditorPopup : public QDialog {
public:
	PropertyBrowserEditorPopup(PropertyBrowserItem* item, QWidget* anchor, ObjectSearchView* list);

protected:
	bool eventFilter(QObject* obj, QEvent* event);

	void keyPressEvent(QKeyEvent* event) override;

	virtual void establishObjectRelation() = 0;
	virtual void removeObjectRelation() = 0;

	PropertyBrowserItem* item_;
	QGridLayout outerLayout_{this};
	QFrame frame_{this};
	QGridLayout layout_{&frame_};
	QLineEdit currentRelation_{this};
	QPushButton deleteButton_{this};
	QLineEdit search_{this};
	ObjectSearchView *list_;
	QPushButton acceptButton_{this};
	QPushButton closeButton_{this};
	QLabel dataTypeLabel_{this};
};

}  // namespace raco::property_browser
