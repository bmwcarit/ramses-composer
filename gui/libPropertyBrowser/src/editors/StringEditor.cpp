/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/StringEditor.h"

#include <QBoxLayout>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QObject>

#include "common_widgets/NoContentMarginsLayout.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include <QFileInfo>
#include <QMimeData>

namespace raco::property_browser {

StringEditor::StringEditor(PropertyBrowserItem* item, QWidget* parent)
	: PropertyEditor(item, parent) {
	this->setLayout(new common_widgets::NoContentMarginsLayout<QHBoxLayout>(this));
	lineEdit_ = new StringEditorLineEdit(this);
	layout()->addWidget(lineEdit_);
	
	// connect item to QLineEdit
	QObject::connect(lineEdit_, &QLineEdit::editingFinished, item, [this]() { 
		// Qt seems to emit editingFinished twice if Return is pressed but not if focus is lost by Tab of mouse click.
		// As a solution we set isModified and check it before attempting the set operation.
		if (lineEdit_->isModified()) {
			if (!lineEdit_->hasMultipleValues()) {
				item_->set(lineEdit_->text().toStdString());
				lineEdit_->setModified(false);
			}
		}
	});

	QObject::connect(lineEdit_, &StringEditorLineEdit::saveFocusInValues, item, [this]() {
		focusInValues_.clear();
		for (const auto& handle : item_->valueHandles()) {
			focusInValues_[handle] = handle.asString();
		}
	});
	QObject::connect(lineEdit_, &StringEditorLineEdit::restoreFocusInValues, item, [this]() {
		std::string desc = fmt::format("Restore value of property '{}'", item_->getPropertyPath());
		item_->commandInterface()->executeCompositeCommand(
			[this]() {
				for (const auto& handle : item_->valueHandles()) {
					item_->commandInterface()->set(handle, focusInValues_[handle]);
				}
			},
			desc);
		updateLineEdit();
	});

	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, item]() {
		this->updateLineEdit();
		this->updateErrorState();
		lineEdit_->update();
	});
	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, [this]() {
		this->updateErrorState();
	});
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		lineEdit_->setFocus();
	});
	QObject::connect(lineEdit_, &StringEditorLineEdit::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });

	updateLineEdit();
	updateErrorState();

	// simple reset of outdate color on editingFinished
	QObject::connect(lineEdit_, &QLineEdit::editingFinished, this, [this]() {
		updatedInBackground_ = false;
	});

	// eventFilter to capture right click for showing copy dialog.
	installEventFilter(this);
	canDisplayCopyDialog = true;
	setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

void StringEditor::updateLineEdit() {
	auto value = item_->as<std::string>();
	lineEdit_->set(value);
}

void StringEditor::updateErrorState() {
	if (item_->hasError()) {
		lineEdit_->setProperty("errorLevel", static_cast<int>(item_->maxErrorLevel()));
		lineEdit_->setToolTip(item_->errorMessage().c_str());
	} else {
		lineEdit_->setProperty("errorLevel", static_cast<int>(core::ErrorLevel::NONE));
		lineEdit_->setToolTip({});
	}
}

void StringEditor::setText(const QString& t) {
	if (lineEdit_->hasFocus() && editingStartedByUser() && t != lineEdit_->text()) {
		updatedInBackground_ = true;
	} else {
		lineEdit_->setText(t);
	}
}

bool StringEditor::editingStartedByUser() {
	return lineEdit_->isModified() || lineEdit_->cursorPosition() != lineEdit_->text().size();
}

bool StringEditor::updatedInBackground() const {
	return updatedInBackground_;
}

void StringEditorLineEdit::focusInEvent(QFocusEvent* event) {
	this->selectAll();
	Q_EMIT saveFocusInValues();
	QLineEdit::focusInEvent(event);
}

int StringEditorLineEdit::errorLevel() const noexcept {
	return static_cast<int>(errorLevel_);
}

void StringEditorLineEdit::setErrorLevel(int level) {
	errorLevel_ = static_cast<core::ErrorLevel>(level);
}

bool StringEditorLineEdit::hasMultipleValues() const {
	return multiValue_;
}

void StringEditorLineEdit::set(std::optional<std::string> value) {
	if (value.has_value()) {
		setText(value.value().c_str());
		setPlaceholderText({});
		multiValue_ = false;
	} else {
		setText({});
		setPlaceholderText(PropertyBrowserItem::MultipleValueText);
		multiValue_ = true;
	}
}

void StringEditorLineEdit::keyPressEvent(QKeyEvent* event) {
	QLineEdit::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape) {
		Q_EMIT restoreFocusInValues();
		clearFocus();
	} else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
		clearFocus();
		Q_EMIT focusNextRequested();
	} else {
		setPlaceholderText({});
		multiValue_ = false;
	}
}

void StringEditorLineEdit::dragEnterEvent(QDragEnterEvent* event) {
	const auto filePath = getAcceptableFilePath(event->mimeData());
	if (!filePath.isEmpty()) {
		event->acceptProposedAction();
	}
}

void StringEditorLineEdit::dropEvent(QDropEvent* event) {
	const QString filePath = getAcceptableFilePath(event->mimeData());
	if (!filePath.isEmpty()) {
		Q_EMIT fileDropped(filePath);
	}
}

QString StringEditorLineEdit::getAcceptableFilePath(const QMimeData* mimeData) const {
	if (mimeData->urls().empty()) {
		return {};
	}

	const QString filePath = mimeData->urls().first().toLocalFile();
	const QFileInfo fileInfo{QFile{filePath}};
	const QString fileExtension = fileInfo.suffix().toLower();
	if (dragAndDropFilter_.toLower().contains("*." + fileExtension)) {
		return filePath;
	}

	return {};
}

}  // namespace raco::property_browser
