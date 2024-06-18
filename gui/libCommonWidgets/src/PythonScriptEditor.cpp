/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/PythonScriptEditor.h"

#include "python_api/PythonAPI.h"

#include <QShortcut>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QStringListModel>
#include <QTextCursor>
#include <QTextBlock>

namespace raco::common_widgets {

PythonScriptEditor::PythonScriptEditor(QWidget* parent) : QPlainTextEdit(parent) {
	setPlaceholderText("Type your Python script here...");

	// Set tabs alignment equal to 4 spaces
	QTextOption textOption = document()->defaultTextOption();
	textOption.setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
	document()->setDefaultTextOption(textOption);

	// Set up the completer_
	QStringListModel* model = new QStringListModel(this);
	completer_ = new QCompleter(this);
	completer_->setModel(model);
	completer_->setWidget(this);
	completer_->setCompletionMode(QCompleter::PopupCompletion);
	completer_->setCaseSensitivity(Qt::CaseInsensitive);
	connect(completer_, QOverload<const QString&>::of(&QCompleter::activated), this, &PythonScriptEditor::insertCompletion);

	// Add shortcut for duplicating the lines (Ctrl+D)
	QShortcut* duplicateLineShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_D), this);
	connect(duplicateLineShortcut, &QShortcut::activated, this, &PythonScriptEditor::duplicateCurrentLine);
}

QString PythonScriptEditor::getExpression() const {
	return toPlainText().replace(indentationSymbol_, ' ');
}

void PythonScriptEditor::updateText(QString text) {
	clear();
	insertPlainText(text.replace(' ', indentationSymbol_));
}

void PythonScriptEditor::insertFromMimeData(const QMimeData* source) {
	QString textToInsert = source->text();
	textToInsert.replace(' ', indentationSymbol_);

	QTextCursor cursor = textCursor();
	cursor.insertText(textToInsert);
	setTextCursor(cursor);
}

QMimeData* PythonScriptEditor::createMimeDataFromSelection() const {
	QMimeData* mimeData = QPlainTextEdit::createMimeDataFromSelection();
	if (!mimeData)
		return nullptr;

	QString originalText = mimeData->text();
	QString modifiedText = originalText.replace(indentationSymbol_, QChar(' '));
	mimeData->setText(modifiedText);

	return mimeData;
}

void PythonScriptEditor::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Space) {
		QTextCursor cursor = textCursor();
		cursor.insertText(QString(indentationSymbol_));
		setTextCursor(cursor);
	} else {
		// Code completion
		if (completer_ && completer_->popup()->isVisible()) {
			// The following keys are forwarded by the completer_ to the widget
			switch (event->key()) {
				case Qt::Key_Enter:
				case Qt::Key_Return:
				case Qt::Key_Escape:
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
					event->ignore();
					return;	 // let the completer_ handle the event
				default:
					break;
			}
		}

		if (event->key() == Qt::Key_D && event->modifiers() == Qt::ControlModifier) {
			duplicateCurrentLine();
		} else {
			switch (event->key()) {
				case Qt::Key_Tab: {
					addSpaces(4);
					break;
				}
				case Qt::Key_Enter:
				case Qt::Key_Return: {
					const int spaces = getSpacesAtBeginningOfLine();
					QPlainTextEdit::keyPressEvent(event);
					addSpaces(spaces);
					break;
				}
				default:
					QPlainTextEdit::keyPressEvent(event);
					break;
			}

			// Set up the completion box
			const QString completionPrefix = getCompletionPrefix();
			if (completionPrefix.isEmpty()) {
				completer_->popup()->hide();
				return;
			}
			if (completionPrefix != completer_->completionPrefix()) {
				updateCompleterModel(completionPrefix);
			}

			QRect cr = cursorRect();
			cr.setWidth(completer_->popup()->sizeHintForColumn(0) + completer_->popup()->verticalScrollBar()->sizeHint().width());
			completer_->complete(cr);
		}
	}
}

QString PythonScriptEditor::getCompletionPrefix() const {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
	QString prefix = cursor.selectedText();

	// Move the cursor to the start of the word
	cursor.movePosition(QTextCursor::StartOfWord);

	// Check if the symbol before is a dot and adjust the position accordingly
	if (cursor.position() > 0 && document()->characterAt(cursor.position() - 1) == '.') {
		// Move the cursor to include the word before the dot
		cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
		cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);

		prefix = prefix.prepend(cursor.selectedText());
	}

	return prefix;
}

void PythonScriptEditor::insertCompletion(const QString& completion) {
	if (completer_->widget() != this)
		return;

	QTextCursor cursor = textCursor();
	const auto trimmedCompletion = completion.mid(completer_->completionPrefix().length());
	cursor.insertText(trimmedCompletion);
	setTextCursor(cursor);
}

void PythonScriptEditor::updateCompleterModel(const QString& prefix) const {
	QStringList completions;
	for (const auto& match : python_api::getCompletions(prefix.toStdString())) {
		completions << QString::fromStdString(match);
	}

	static_cast<QStringListModel*>(completer_->model())->setStringList(completions);
	completer_->setCompletionPrefix(prefix);
	completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
}

void PythonScriptEditor::duplicateCurrentLine() const {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfBlock);
	QString currentLine = cursor.block().text();

	cursor.movePosition(QTextCursor::EndOfBlock);
	cursor.insertText("\n" + currentLine);
}

void PythonScriptEditor::addSpaces(int spaceCount) {
	if (spaceCount > 0) {
		QString bulletString = QString(spaceCount, indentationSymbol_);
		insertPlainText(bulletString);
	}
}

int PythonScriptEditor::getSpacesAtBeginningOfLine() const {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfLine);

	QString line = cursor.block().text();
	int spaceCount = 0;

	for (const QChar& character : line) {
		if (character == indentationSymbol_) {
			spaceCount++;
		} else {
			// Stop collecting spaces when a non-space character is encountered
			break;
		}
	}

	return spaceCount;
}

}  // namespace raco::common_widgets
