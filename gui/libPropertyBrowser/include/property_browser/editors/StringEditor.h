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
	Q_PROPERTY(int errorLevel READ errorLevel WRITE setErrorLevel)

public:
	StringEditorLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {  }

	int errorLevel() const noexcept;
	void setDragAndDropFilter(const QString& filter) {
		dragAndDropFilter_ = filter;
	}
	bool hasMultipleValues() const;
	void set(std::optional<std::string> value);

protected:
	core::ErrorLevel errorLevel_{core::ErrorLevel::NONE};

	void setErrorLevel(int level);
	void focusInEvent(QFocusEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

Q_SIGNALS:
	void focusNextRequested();
	void saveFocusInValues();
	void restoreFocusInValues();
	void fileDropped(QString filePath);

private:
	bool multiValue_ = false;
	QString dragAndDropFilter_;
	QString getAcceptableFilePath(const QMimeData* mimeData) const;
};

class StringEditor : public PropertyEditor {
	Q_OBJECT
	Q_PROPERTY(bool updatedInBackground READ updatedInBackground);

public:
	explicit StringEditor(PropertyBrowserItem* item, QWidget* parent = nullptr);
	bool updatedInBackground() const;

public Q_SLOTS:
	void setText(const QString&);

protected:
	bool editingStartedByUser();
	void updateLineEdit();
	void updateErrorState();

	bool updatedInBackground_ = false;
	StringEditorLineEdit* lineEdit_;

	std::map<core::ValueHandle, std::string> focusInValues_;
};

}  // namespace raco::property_browser
