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

#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QTextBlock>
#include <QMimeData>
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QScrollBar>

namespace raco::common_widgets {

class PythonConsole : public QPlainTextEdit {
	Q_OBJECT

public:
	PythonConsole(QWidget* parent = nullptr);
	void clearConsole();
	void addOutput(const QString& output, const QString& error);
	void addOutputFromScriptEditor(const QString& output, const QString& error);

Q_SIGNALS:
	void runPythonScript(const QString& script);

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void insertFromMimeData(const QMimeData* source) override;

private slots:
	void insertCompletion(const QString& completion);

private:
	QString getCompletionPrefix() const;
	void updateCompleterModel(const QString& prefix) const;
	void appendNewLine();
	void appendInputArrows();
	void insertInputArrows();
	void appendInputDots();
	void insertPythonVersion();
	void moveCursorToLineBeginning();
	void moveCursorToLastPosition();
	void recallPreviousCommand();
	void recallNextCommand();
	void accumulateCommand();
	void executeCommand(const QString& command);
	void addCommandToHistory(const QString& command);
	void cleanCurrentLine();
	bool isMovingLeftAllowed() const;
	bool isMovingRightAllowed() const;
	bool isDeletingAllowed() const;
	bool isActionAllowed(int positionOffset, bool checkLine = false) const;
	bool isTypingAllowed() const;
	QString getCurrentCommand() const;

	QString pythonVersion_{""};
	int commandHistoryIndex_{0};
	QStringList commandHistory_{};
	QString commandAccumulator_{""};
	QString lastCommand_{""};
	QCompleter* completer_;
};

}  // namespace raco::common_widgets
