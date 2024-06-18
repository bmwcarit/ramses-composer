/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/RunScriptDialog.h"

#include "core/PathManager.h"
#include "python_api/PythonAPI.h"
#include "utils/u8path.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QTextStream>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QSplitter>


namespace raco::common_widgets {

RunScriptDialog::RunScriptDialog(std::map<QString, qint64>& scriptEntries, std::map<QString, qint64>& commandLineParamEntries, QWidget* parent)
	: QWidget{parent},
	  scriptEntries_{scriptEntries}{
	// Python script path
	const auto pathLabel = new QLabel("Python Script Path");
	scriptPathEdit_ = new QComboBox();
	scriptPathEdit_->setEditable(true);

	scriptPathURIButton_ = new QPushButton("...");
	scriptPathURIButton_->setMaximumWidth(30);
	QObject::connect(scriptPathURIButton_, &QPushButton::clicked, [this]() {
		 auto pythonPath = scriptPathEdit_->currentText();
		 if (pythonPath.isEmpty()) {
			const auto cachedScriptPath = core::PathManager::getCachedPath(core::PathManager::FolderTypeKeys::Script, pythonPath.toStdString());
			pythonPath = QString::fromStdString(cachedScriptPath.string());
		 }

		 const auto pythonScriptPath = QFileDialog::getOpenFileName(this, "Open Python Script", pythonPath, "Python Scripts (*.py);; All files (*.*)");
		 if (!pythonScriptPath.isEmpty()) {
			scriptPathEdit_->setCurrentText(pythonScriptPath);
			core::PathManager::setCachedPath(core::PathManager::FolderTypeKeys::Script, QFileInfo(pythonScriptPath).absoluteDir().absolutePath().toStdString());
			
			scriptTextEdit_->updateText(getPythonFileContent(pythonScriptPath));
		 }
	});

	// Python script editor
	scriptTextEdit_ = new PythonScriptEditor();
	QObject::connect(scriptTextEdit_, &QPlainTextEdit::textChanged, this, &RunScriptDialog::updateButtonStates);
	scriptTextEdit_->setFont(QFont("RobotoRegular", 10));

	scriptButtonBox_ = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help};
	scriptButtonBox_->button(QDialogButtonBox::Ok)->setText("Save");
	scriptButtonBox_->button(QDialogButtonBox::Cancel)->setText("Save As");
	scriptButtonBox_->button(QDialogButtonBox::Help)->setText("Run Script");
	connect(scriptButtonBox_, &QDialogButtonBox::accepted, this, [this]() {
		saveScript(scriptPathEdit_->currentText());
	});
	connect(scriptButtonBox_, &QDialogButtonBox::rejected, this, &RunScriptDialog::saveScriptAs);
	connect(scriptButtonBox_, &QDialogButtonBox::helpRequested, this, [this]() {
		runScript(scriptTextEdit_->getExpression(), ScriptSource::Editor);
	});

	// Python console
	pythonConsole_ = new PythonConsole();
	QObject::connect(pythonConsole_, &PythonConsole::runPythonScript, this, [this](const QString& script) {
		runScript(script, ScriptSource::Console);
	});
	pythonConsole_->setFont(QFont("RobotoRegular", 10));

	clearConsoleButtonBox_ = new QDialogButtonBox{QDialogButtonBox::Ok};
	clearConsoleButtonBox_->button(QDialogButtonBox::Ok)->setText("Clear Console");
	connect(clearConsoleButtonBox_, &QDialogButtonBox::accepted, this, &RunScriptDialog::clearConsole);

	// Layouts
	const auto scriptPathLayout = new QHBoxLayout();
	scriptPathLayout->addWidget(pathLabel);
	scriptPathLayout->addWidget(scriptPathEdit_);
	scriptPathLayout->addWidget(scriptPathURIButton_);

	const auto scriptLayout = new QVBoxLayout();
	scriptLayout->addLayout(scriptPathLayout);
	scriptLayout->addWidget(scriptTextEdit_);
	scriptLayout->addWidget(scriptButtonBox_);

	const auto consoleLayout = new QVBoxLayout();
	consoleLayout->addWidget(pythonConsole_);
	consoleLayout->addWidget(clearConsoleButtonBox_);
	
	const auto verticalSplitter = new QSplitter(Qt::Vertical);
	verticalSplitter->addWidget(new QWidget);
	verticalSplitter->addWidget(new QWidget);
	
	verticalSplitter->widget(0)->setLayout(scriptLayout);
	verticalSplitter->widget(1)->setLayout(consoleLayout);

	// This is only needed when the dialog is docked, probably it's a AdvancedDocking issue
	QObject::connect(verticalSplitter, &QSplitter::splitterMoved, [this](int pos, int index) {
		update();
	});

	const auto mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(verticalSplitter);

	// Python API connections
	connect(this, &RunScriptDialog::runScriptRequested, [this](const QString& pythonScript, const ScriptSource source) {
		setScriptIsRunning(true);
		repaint();

		const auto currentRunStatus = python_api::runPythonScript(pythonScript.toStdString());
		if (source == ScriptSource::Editor) {
			addOutputFromScriptEditor(currentRunStatus.stdOutBuffer, currentRunStatus.stdErrBuffer);
		} else {
			addOutputFromConsole(currentRunStatus.stdOutBuffer, currentRunStatus.stdErrBuffer);
		}

		setScriptIsRunning(false);
	});

	// Code highlighting
	scriptEditorHighlighter_ = new PythonHighlighter(scriptTextEdit_->document());
	consoleHighlighter_ = new PythonHighlighter(pythonConsole_->document(), true);

	// Shortcuts (Alt+Enter / Alt+Return)
	const auto enterShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Enter), this);
	connect(enterShortcut, &QShortcut::activated, this, [this]() {
		runScript(scriptTextEdit_->getExpression(), ScriptSource::Editor);
	});
	const auto returnShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Return), this);
	connect(returnShortcut, &QShortcut::activated, this, [this]() {
		runScript(scriptTextEdit_->getExpression(), ScriptSource::Editor);
	});

	// Updates
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("Run Python Script");
	updateComboBoxItems();
	updateButtonStates();
}

void RunScriptDialog::addOutputFromScriptEditor(const std::string& outBuffer, const std::string& errorBuffer) const {
	pythonConsole_->addOutputFromScriptEditor(QString::fromStdString(outBuffer), QString::fromStdString(errorBuffer));
}

void RunScriptDialog::addOutputFromConsole(const std::string& outBuffer, const std::string& errorBuffer) const {
	pythonConsole_->addOutput(QString::fromStdString(outBuffer), QString::fromStdString(errorBuffer));
}

void RunScriptDialog::setScriptIsRunning(bool isRunning) const {
	scriptPathEdit_->setDisabled(isRunning);
	scriptPathURIButton_->setDisabled(isRunning);
	scriptButtonBox_->setDisabled(isRunning);
}

void RunScriptDialog::updateButtonStates() const {
	scriptTextEdit_->toPlainText().isEmpty() ? scriptButtonBox_->button(QDialogButtonBox::Ok)->setEnabled(false) : scriptButtonBox_->button(QDialogButtonBox::Ok)->setEnabled(true);
	scriptTextEdit_->toPlainText().isEmpty() ? scriptButtonBox_->button(QDialogButtonBox::Cancel)->setEnabled(false) : scriptButtonBox_->button(QDialogButtonBox::Cancel)->setEnabled(true);
}

void RunScriptDialog::saveScript(const QString& filePath) const {
	if (!filePath.isEmpty()) {
		QFile file(filePath);

		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&file);
			out << scriptTextEdit_->getExpression();
			file.close();

			scriptPathEdit_->setCurrentText(filePath);
		} else {
			QMessageBox::warning(
				nullptr,
				"File Save Error",
				"Failed to open file for writing:\n" + file.errorString(),
				QMessageBox::Ok);
		}
	}
}

void RunScriptDialog::saveScriptAs() const {
	auto pythonPath = scriptPathEdit_->currentText();
	if (pythonPath.isEmpty()) {
		const auto cachedScriptPath = core::PathManager::getCachedPath(core::PathManager::FolderTypeKeys::Script, pythonPath.toStdString());
		pythonPath = QString::fromStdString(cachedScriptPath.string());
	}

	const auto filePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save As...",
		pythonPath,
		"Python Files (*.py);;All Files (*)");

	saveScript(filePath);
}

void RunScriptDialog::clearConsole() const {
	pythonConsole_->clearConsole();
}

void RunScriptDialog::runScript(const QString& script, const ScriptSource source) {
	const auto scriptPath = scriptPathEdit_->currentText();

	scriptEntries_[scriptPath] = QDateTime::currentMSecsSinceEpoch();

	updateComboBoxItems();
	Q_EMIT runScriptRequested(script, source);
}

void RunScriptDialog::updateComboBoxItems() const {
	auto updateItems = [](QComboBox* comboBox, auto& map) {
		comboBox->clear();

		std::vector<std::pair<QString, qint64>> mapSortedByValue(map.begin(), map.end());

		std::sort(mapSortedByValue.begin(), mapSortedByValue.end(), [](const std::pair<QString, qint64>& l, const std::pair<QString, qint64>& r) {
			return l.second > r.second;
		});

		if (map.size() > ENTRIES_SIZE) {
			map.erase(mapSortedByValue.back().first);
			mapSortedByValue.pop_back();
		}

		for (const auto& [string, date] : mapSortedByValue) {
			comboBox->addItem(string);
		}
	};
}

QString RunScriptDialog::getPythonFileContent(const QString& filePath) const {
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}

	QTextStream textStream(&file);
	QString fileContents = textStream.readAll();

	return fileContents;
}

}  // namespace raco::common_widgets
