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

#include "PythonHighlighter.h"
#include "PythonScriptEditor.h"
#include "PythonConsole.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>

namespace raco::common_widgets {
class RaCoApplication;

class RunScriptDialog final : public QWidget {
	Q_OBJECT
public:
	enum class ScriptSource {
		Editor,
		Console
	};

	static constexpr auto ENTRIES_SIZE = 3;

	RunScriptDialog(std::map<QString, qint64>& scriptEntries, std::map<QString, qint64>& commandLineParamEntries, QWidget* parent = nullptr);
	
	void addOutputFromScriptEditor(const std::string& outBuffer, const std::string& errorBuffer) const;
	void addOutputFromConsole(const std::string& outBuffer, const std::string& errorBuffer) const;
	void setScriptIsRunning(bool isRunning) const;

Q_SIGNALS:
	void runScriptRequested(const QString& script, const ScriptSource source);

private slots:
	void updateButtonStates() const;
	void runScript(const QString& script, const ScriptSource source);
	void saveScript(const QString& filePath) const;
	void saveScriptAs() const;
	void clearConsole() const;

private:
	void updateComboBoxItems() const;
	QString getPythonFileContent(const QString& filePath) const;

	QComboBox* scriptPathEdit_;
	QPushButton* scriptPathURIButton_;

	PythonScriptEditor* scriptTextEdit_;
	QDialogButtonBox* scriptButtonBox_;
	PythonConsole* pythonConsole_;
	QDialogButtonBox* clearConsoleButtonBox_;
	
	PythonHighlighter* scriptEditorHighlighter_;
	PythonHighlighter* consoleHighlighter_;
	
	std::map<QString, qint64>& scriptEntries_;
};

}  // namespace raco::common_widgets
