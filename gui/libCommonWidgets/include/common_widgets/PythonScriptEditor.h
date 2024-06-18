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
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>

namespace raco::common_widgets {

class PythonScriptEditor : public QPlainTextEdit {
public:
	PythonScriptEditor(QWidget* parent = nullptr);
	QString getExpression() const;
	void updateText(QString text);

protected:
	void insertFromMimeData(const QMimeData* source) override;
	QMimeData* createMimeDataFromSelection() const override;
	void keyPressEvent(QKeyEvent* event) override;

private slots:
	void insertCompletion(const QString& completion);
	void duplicateCurrentLine() const;
	void updateCompleterModel(const QString& prefix) const;

private:
	QString getCompletionPrefix() const;
	void addSpaces(int spaceCount);
	int getSpacesAtBeginningOfLine() const;

	QCompleter* completer_;
	const QChar indentationSymbol_{QChar(0x2219)};
};

}  // namespace raco::common_widgets
