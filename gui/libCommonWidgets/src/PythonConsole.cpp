/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/PythonConsole.h"

#include "python_api/PythonAPI.h"

namespace raco::common_widgets {

PythonConsole::PythonConsole(QWidget* parent) : QPlainTextEdit(parent) {
	pythonVersion_ = QString::fromStdString(python_api::getPythonVersion());

	setUndoRedoEnabled(false);

	appendInputArrows();
	insertPythonVersion();
	appendInputArrows();

	// Setup the completer
	QStringListModel* model = new QStringListModel(this);
	completer_ = new QCompleter(this);
	completer_->setModel(model);
	completer_->setWidget(this);
	completer_->setCompletionMode(QCompleter::PopupCompletion);
	completer_->setCaseSensitivity(Qt::CaseInsensitive);
	connect(completer_, QOverload<const QString&>::of(&QCompleter::activated), this, &PythonConsole::insertCompletion);

	setWordWrapMode(QTextOption::WrapMode::WrapAnywhere);
	show();
}

void PythonConsole::keyPressEvent(QKeyEvent* event) {
	// Handle only key presses
	if (event->type() != QEvent::KeyPress) {
		return;
	}

	// Ignore undo/redo shortcuts
	if (event->matches(QKeySequence::Undo) || event->matches(QKeySequence::Redo)) {
		return;
	}

	// Code completer events
	if (completer_ && completer_->popup()->isVisible()) {
		// The following keys are forwarded by the completer to the widget
		switch (event->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				event->ignore();
				return;	 // let the completer handle the event
			default:
				break;
		}
	}

	// Handle control buttons (Page Up/Down, Home/End, Arrow Up/Down, Enter...)
	bool needsComleterUpdate = false;
	switch (event->key()) {
		case Qt::Key_Up:
			recallPreviousCommand();
			break;
		case Qt::Key_Down:
			recallNextCommand();
			break;
		case Qt::Key_Left:
			if (isMovingLeftAllowed()) {
				QPlainTextEdit::keyPressEvent(event);
			}
			break;
		case Qt::Key_Right:
			if (isMovingRightAllowed()) {
				QPlainTextEdit::keyPressEvent(event);
			}
			break;
		case Qt::Key_Home:
			moveCursorToLineBeginning();
			break;
		case Qt::Key_Backspace:
			if (isDeletingAllowed()) {
				QPlainTextEdit::keyPressEvent(event);
				needsComleterUpdate = true;
			}
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			accumulateCommand();
			break;
		default: // Handle the rest of the buttons
			if (isTypingAllowed()) {
				QPlainTextEdit::keyPressEvent(event);
				needsComleterUpdate = true;
			}
			break;
	}

	if (needsComleterUpdate) {
		const QString completionPrefix = getCompletionPrefix();
		if (completionPrefix.isEmpty()) {
			completer_->popup()->close();
			return;
		}

		if (completer_->completionPrefix() != completionPrefix) {
			updateCompleterModel(completionPrefix);

			QRect cr = cursorRect();
			cr.setWidth(completer_->popup()->sizeHintForColumn(0) + completer_->popup()->verticalScrollBar()->sizeHint().width());
			completer_->complete(cr);
		}
	}
}

void PythonConsole::insertFromMimeData(const QMimeData* source) {
	QString textToInsert = source->text();
	textToInsert.replace("\n", "\n.  .  . ");

	QTextCursor cursor = textCursor();
	cursor.insertText(textToInsert);
	setTextCursor(cursor);
}

void PythonConsole::insertCompletion(const QString& completion) {
	if (completer_->widget() != this)
		return;

	QTextCursor cursor = textCursor();
	const auto trimmedCompletion = completion.mid(completer_->completionPrefix().length());
	cursor.insertText(trimmedCompletion);
	setTextCursor(cursor);
}

QString PythonConsole::getCompletionPrefix() const {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
	while (cursor.position() > 0 && document()->characterAt(cursor.position() - 1) == '.') {
		cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
		cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
	}
	return cursor.selectedText();
}

void PythonConsole::updateCompleterModel(const QString& prefix) const {
	QStringList completions;
	for (const auto& match : python_api::getCompletions(prefix.toStdString())) {
		completions << QString::fromStdString(match);
	}

	static_cast<QStringListModel*>(completer_->model())->setStringList(completions);
	completer_->setCompletionPrefix(prefix);
	completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
}

void PythonConsole::appendNewLine() {
	appendPlainText("\n");
}

void PythonConsole::appendInputArrows() {
	appendPlainText(">>> ");
}

void PythonConsole::insertInputArrows() {
	insertPlainText(">>> ");
}

void PythonConsole::appendInputDots() {
	appendPlainText(".  .  . ");
}

void PythonConsole::insertPythonVersion() {
	insertPlainText("Python " + pythonVersion_);
}

void PythonConsole::moveCursorToLineBeginning() {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfBlock);
	cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 4);
	setTextCursor(cursor);
}

void PythonConsole::moveCursorToLastPosition() {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::End);
	setTextCursor(cursor);
}

bool PythonConsole::isMovingLeftAllowed() const {
	return isActionAllowed(0);
}

bool PythonConsole::isMovingRightAllowed() const {
	return isActionAllowed(-1);
}

bool PythonConsole::isDeletingAllowed() const {
	return isActionAllowed(0, true);
}

bool PythonConsole::isTypingAllowed() const {
	return isActionAllowed(-1, true);
}

bool PythonConsole::isActionAllowed(int positionOffset, bool checkLine) const {
	const auto cursor = textCursor();
	const auto currentPositionInBlock = cursor.positionInBlock();
	const auto startsWithArrows = cursor.block().text().startsWith(">>> ");
	// Position in line must be outside of arrows (4 position) or dots (8 positions) minus one
	const auto minAllowedPosition = startsWithArrows ? (4 + positionOffset) : (8 + positionOffset);
	
	if (checkLine) {
		const auto currentBlockNumber = cursor.blockNumber();
		const auto totalBlocks = document()->blockCount();
		return currentBlockNumber == totalBlocks - 1 && currentPositionInBlock > minAllowedPosition;
	} else {
		return currentPositionInBlock > minAllowedPosition;
	}
}

void PythonConsole::cleanCurrentLine() {
	QTextCursor cursor = textCursor();

	const QString lineContent = cursor.block().text();
	QString lineBeginning;
	if (lineContent.startsWith(">>> ")) {
		lineBeginning = ">>> ";
	} else if (lineContent.startsWith(".  .  . ")) {
		lineBeginning = ".  .  . ";
	}
	
	cursor.movePosition(QTextCursor::End);
	cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
	cursor.removeSelectedText();

	insertPlainText(lineBeginning);
}

void PythonConsole::clearConsole() {
	clear();
	appendInputArrows();
	insertPythonVersion();
	appendInputArrows();
}

void PythonConsole::addOutput(const QString& output, const QString& error) {
	if (!output.trimmed().isEmpty()) {
		appendPlainText(output.trimmed());
	}

	if (!error.trimmed().isEmpty()) {
		appendPlainText(error.trimmed());
	}
	
	appendInputArrows();
}

void PythonConsole::addOutputFromScriptEditor(const QString& output, const QString& error) {
	insertPlainText("Running script from the editor...");
	addOutput(output, error);
}

void PythonConsole::recallPreviousCommand() {
	if (!isTypingAllowed()) {
		return;
	}

	if (commandHistoryIndex_ > 0) {
		commandHistoryIndex_--;
		cleanCurrentLine();
		insertPlainText(commandHistory_[commandHistoryIndex_]);
	}
}

void PythonConsole::recallNextCommand() {
	if (!isTypingAllowed()) {
		return;
	}

	if (commandHistoryIndex_ < commandHistory_.size() - 1) {
		commandHistoryIndex_++;
		cleanCurrentLine();
		insertPlainText(commandHistory_[commandHistoryIndex_]);
	} else {
		// If we reach the latest command, clear the console
		cleanCurrentLine();
		commandHistoryIndex_ = commandHistory_.size();
	}
}

void PythonConsole::accumulateCommand() {
	if (!isTypingAllowed()) {
		return;
	}

	const auto command = getCurrentCommand();
	if (!command.isEmpty()) {
		commandAccumulator_ += command + "\n";
	}

	if (command.isEmpty()) {
		if (!commandAccumulator_.isEmpty()) {
			executeCommand(commandAccumulator_.trimmed());
			commandAccumulator_.clear();
		} else {
			appendInputArrows();
		}
	} else {
		if (python_api::isCompleteCommand(commandAccumulator_.trimmed().toStdString())) {
			executeCommand(commandAccumulator_.trimmed());
			commandAccumulator_.clear();
		} else {
			appendInputDots();
		}
		addCommandToHistory(command);
	}
}

void PythonConsole::executeCommand(const QString& command) {
	moveCursorToLastPosition();
	Q_EMIT runPythonScript(command);
}

void PythonConsole::addCommandToHistory(const QString& command) {
	if (command != lastCommand_) {
		commandHistory_ << command;
		lastCommand_ = command;
	}

	commandHistoryIndex_ = commandHistory_.size();
}

QString PythonConsole::getCurrentCommand() const {
	QString command = textCursor().block().text();
	if (command.startsWith(">>> ")) {
		command.remove(">>> ");
	} else if (command.startsWith(".  .  . ")) {
		command.remove(".  .  . ");
	}
	return command;
}

}  // namespace raco::common_widgets
