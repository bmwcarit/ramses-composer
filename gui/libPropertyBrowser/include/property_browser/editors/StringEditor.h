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

#include "PropertyEditor.h"

#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QKeyEvent>
#include "core/ErrorItem.h"

namespace raco::property_browser {
	
class PropertyBrowserItem;

class StringEditorLineEdit : public QLineEdit {
	Q_OBJECT

public:
	StringEditorLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {  }

protected:
	QString focusInOldText_;

	void focusInEvent(QFocusEvent* event);

	void keyPressEvent(QKeyEvent* event);

Q_SIGNALS:
	void focusNextRequested();
};

class StringEditor : public PropertyEditor {
	Q_OBJECT
	Q_PROPERTY(bool updatedInBackground READ updatedInBackground);
	Q_PROPERTY(int errorLevel READ errorLevel);

public:
	explicit StringEditor(PropertyBrowserItem* item, QWidget* parent = nullptr);
	bool updatedInBackground() const;
	int errorLevel() const noexcept;

public Q_SLOTS:
	void setText(const QString&);

protected:
	bool editingStartedByUser();
	void updateErrorState();

	bool updatedInBackground_ = false;
	core::ErrorLevel errorLevel_{core::ErrorLevel::NONE};
	StringEditorLineEdit* lineEdit_;
};

}  // namespace raco::property_browser
