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
#include "style/Colors.h"
#include "utils/u8path.h"

#include <QCompleter>
#include <QDateTime>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>


namespace raco::common_widgets {

RunScriptDialog::RunScriptDialog(std::map<QString, qint64>& scriptEntries, std::map<QString, qint64>& commandLineParamEntries, QWidget* parent)
	: QWidget{parent},
	  scriptEntries_{scriptEntries},
	  commandLineParamEntries_{commandLineParamEntries}
{

	layout_ = new QGridLayout(this);

	scriptSettingsLayout_ = new QGridLayout(this);

	scriptPathEdit_ = new QComboBox(this);
	scriptPathEdit_->setEditable(true);

	scriptPathURIButton_ = new QPushButton("...", this);
	scriptPathURIButton_->setMaximumWidth(30);
	scriptSettingsLayout_->addWidget(new QLabel{"Python Script Path", this}, 0, 0);
	scriptSettingsLayout_->addWidget(scriptPathEdit_, 0, 1);
	scriptSettingsLayout_->addWidget(scriptPathURIButton_, 0, 2);
	QObject::connect(scriptPathEdit_, &QComboBox::currentTextChanged, this, &RunScriptDialog::updateButtonStates);
	QObject::connect(scriptPathURIButton_, &QPushButton::clicked, [this]() {
		auto pythonPath = scriptPathEdit_->currentText();
		if (pythonPath.isEmpty()) {
			auto cachedScriptPath = raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script, pythonPath.toStdString());
			pythonPath = QString::fromStdString(cachedScriptPath.string());
		}

		auto pythonScriptPath = QFileDialog::getOpenFileName(this, "Open Python Script", pythonPath, "Python Scripts (*.py);; All files (*.*)");
		if (!pythonScriptPath.isEmpty()) {
			scriptPathEdit_->setCurrentText(pythonScriptPath);
			raco::core::PathManager::setCachedPath(raco::core::PathManager::FolderTypeKeys::Script, QFileInfo(pythonScriptPath).absoluteDir().absolutePath().toStdString());
		}
	});

	argumentsEdit_ = new QComboBox(this);
	argumentsEdit_->setEditable(true);
	scriptSettingsLayout_->addWidget(new QLabel{"Command Line Arguments", this}, 1, 0);
	scriptSettingsLayout_->addWidget(argumentsEdit_, 1, 1);

	buttonBox_ = new QDialogButtonBox{QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this};
	buttonBox_->button(QDialogButtonBox::Ok)->setText("Run Script");
	buttonBox_->button(QDialogButtonBox::Cancel)->setText("Clear Output");
	connect(buttonBox_, &QDialogButtonBox::accepted, this, &RunScriptDialog::runScript);
	connect(buttonBox_, &QDialogButtonBox::rejected, [this]() { statusTextBlock_->clear(); });

	statusTextBlock_ = new QPlainTextEdit(this);
	statusTextBlock_->setReadOnly(true);
	statusTextBlock_->setPlaceholderText("Python Script Outputs...");

	layout_->addLayout(scriptSettingsLayout_, 0, 0);
	layout_->addWidget(buttonBox_, 1, 0);
	layout_->addWidget(statusTextBlock_, 2, 0);

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("Run Python Script");
	updateComboBoxItems();
	updateButtonStates();
}

void RunScriptDialog::addPythonOutput(const std::string& outBuffer, const std::string& errorBuffer) {
	auto launchTime = QDateTime::currentDateTime();
	statusTextBlock_->appendHtml(QString("<b><font color=\"%1\">===== SCRIPT RUN AT %2 =====</font></b>").arg(raco::style::Colors::color(raco::style::Colormap::externalReference).name()).arg(launchTime.toString("hh:mm:ss.zzz")));

	statusTextBlock_->appendPlainText(QString::fromStdString(outBuffer));

	if (!errorBuffer.empty()) {
		statusTextBlock_->appendHtml(QString("<font color=\"%1\">%2</font><br>").arg(raco::style::Colors::color(raco::style::Colormap::errorColor).name()).arg(QString::fromStdString(errorBuffer).toHtmlEscaped()));
	}
	statusTextBlock_->appendHtml(QString("<b><font color=\"%1\">===== SCRIPT RUN AT %2 FINISHED =====</font></b><br>").arg(raco::style::Colors::color(raco::style::Colormap::externalReference).name()).arg(launchTime.toString("hh:mm:ss.zzz")));
}

void RunScriptDialog::setScriptIsRunning(bool isRunning) {
	scriptPathEdit_->setDisabled(isRunning);
	argumentsEdit_->setDisabled(isRunning);
	scriptPathURIButton_->setDisabled(isRunning);
	buttonBox_->setDisabled(isRunning);
}

Q_SLOT void RunScriptDialog::updateButtonStates() {
	auto path = scriptPathEdit_->currentText();
	auto fileInfo = QFileInfo(path);
	if (scriptPathEdit_->currentText().isEmpty() || !fileInfo.exists() || !fileInfo.isReadable()) {
		buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(false);
		return;
	}

	buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void RunScriptDialog::runScript() {
	auto scriptPath = scriptPathEdit_->currentText();
	auto commandLineArgumentString = argumentsEdit_->currentText();
	auto commandLineArguments = commandLineArgumentString.split(' ', Qt::SkipEmptyParts);

	scriptEntries_[scriptPath] = QDateTime::currentMSecsSinceEpoch();
	if (!commandLineArguments.isEmpty()) {
		commandLineParamEntries_[commandLineArgumentString] = QDateTime::currentMSecsSinceEpoch();
	}

	updateComboBoxItems();
	Q_EMIT pythonScriptRunRequested(scriptPath, commandLineArguments);
}

void RunScriptDialog::updateComboBoxItems() {
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

	auto resetCommandLineParams = argumentsEdit_->currentText().isEmpty();

	updateItems(scriptPathEdit_, scriptEntries_);
	updateItems(argumentsEdit_, commandLineParamEntries_);

	if (resetCommandLineParams) {
		argumentsEdit_->setCurrentText("");
	}
}

}  // namespace raco::common_widgets
