/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/ExportDialog.h"

#include "common_widgets/ErrorView.h"
#include "components/RaCoNameConstants.h"
#include "core/PathManager.h"
#include "core/SceneBackendInterface.h"
#include "utils/stdfilesystem.h"
#include "utils/u8path.h"

#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTreeWidget>
#include <map>
#include <spdlog/fmt/fmt.h>

namespace {

QStandardItemModel* createSummaryModel(const raco::core::SceneBackendInterface* backend, QObject* parent) {
	auto* listViewModel = new QStandardItemModel{parent};
	listViewModel->setColumnCount(2);
	listViewModel->setHorizontalHeaderItem(0, new QStandardItem{"Type"});
	listViewModel->setHorizontalHeaderItem(1, new QStandardItem{"Name"});

	auto sceneItems = backend->getSceneItemDescriptions();

	std::map<raco::core::SceneBackendInterface::SceneItemDesc*, QStandardItem*> parents{};
	for (auto& item : sceneItems) {
		using ItemList = QList<QStandardItem*>;
		auto* col0 = new QStandardItem{QString::fromStdString(item.type_)};
		auto* col1 = new QStandardItem{QString::fromStdString(item.objectName_)};
		if (item.parentIndex_ != -1) {
			parents[&sceneItems[item.parentIndex_]]->appendRow(ItemList{} << col0 << col1);
		} else {
			listViewModel->appendRow(ItemList{} << col0 << col1);
		}
		parents[&item] = col0;
	}

	return listViewModel;
}

}  // namespace

namespace raco::common_widgets {

ExportDialog::ExportDialog(application::RaCoApplication* application, LogViewModel * logViewModel, QWidget* parent) : QDialog{parent}, application_{application} {
	setWindowTitle(QString{"Export Project - %1"}.arg(application->activeRaCoProject().name()));

	auto* content = new QGroupBox{"Export Configuration:", this};
	auto* contentLayout_ = new QGridLayout{content};

	contentLayout_->setAlignment(Qt::AlignTop);

	pathEdit_ = new QLineEdit(content);
	pathEdit_->setMinimumWidth(600);
	contentLayout_->addWidget(new QLabel{"Export path:", content}, 0, 0);
	contentLayout_->addWidget(pathEdit_, 0, 1);

	ramsesEdit_ = new QLineEdit(content);
	contentLayout_->addWidget(new QLabel{"Ramses file:", content}, 1, 0);
	contentLayout_->addWidget(ramsesEdit_, 1, 1);

	logicEdit_ = new QLineEdit(content);
	contentLayout_->addWidget(new QLabel{"Logic file:", content}, 2, 0);
	contentLayout_->addWidget(logicEdit_, 2, 1);

	compressEdit_ = new QCheckBox(content);
	contentLayout_->addWidget(new QLabel{"Compress:", content}, 3, 0);
	contentLayout_->addWidget(compressEdit_, 3, 1);
	compressEdit_->setChecked(true);

	if (application_->activeProjectPath().size() > 0) {
		auto projectPath = raco::utils::u8path(application_->activeProjectPath());
		auto ramsesPath = projectPath;
		ramsesPath.replace_extension(raco::names::FILE_EXTENSION_RAMSES_EXPORT);
		auto logicPath = projectPath;
		logicPath.replace_extension(raco::names::FILE_EXTENSION_LOGIC_EXPORT);
		pathEdit_->setText(QString::fromStdString(application_->activeProjectFolder()));
		ramsesEdit_->setText(QString::fromStdString(ramsesPath.string()));
		logicEdit_->setText(QString::fromStdString(logicPath.string()));
	} else {
		pathEdit_->setText(QString::fromStdString(raco::utils::u8path::current().string()));
		ramsesEdit_->setText(QString("unknown.").append(raco::names::FILE_EXTENSION_RAMSES_EXPORT));
		logicEdit_->setText(QString("unknown.").append(raco::names::FILE_EXTENSION_LOGIC_EXPORT));
	}

	auto* summaryBox = new QGroupBox{"Summary", this};
	auto* summaryBoxLayout = new QVBoxLayout{summaryBox};

	auto* tabWidget = new QTabWidget(summaryBox);
	summaryBoxLayout->addWidget(tabWidget);

	auto* listView = new QTreeView{tabWidget};
	listView->setModel(createSummaryModel(application->sceneBackend(), listView));
	listView->setMinimumHeight(500);
	tabWidget->addTab(listView, QString{"Scene ID: %1"}.arg(application->sceneBackend()->currentSceneIdValue()));

	hasErrors_ = false;

	auto* commandInterface = application->activeRaCoProject().commandInterface();
	if (!commandInterface->errors().getAllErrors().empty()) {
		bool hasComposerErrors = false;
		bool hasComposerWarnings = false;

		for (const auto& errorPair : commandInterface->errors().getAllErrors()) {
			if (errorPair.second.level() == raco::core::ErrorLevel::WARNING) {
				hasComposerWarnings = true;
			} else if (errorPair.second.level() == raco::core::ErrorLevel::ERROR) {
				hasErrors_ = true;
				hasComposerErrors = true;
				break;
			}
		}

		if (hasComposerErrors || hasComposerWarnings) {
			auto* errorView = new ErrorView(commandInterface, application->dataChangeDispatcher(), true, logViewModel, this);
			tabWidget->setCurrentIndex(tabWidget->addTab(errorView, hasComposerErrors ? "Composer Errors" : "Composer Warnings"));
		}
	}

	if (!application->sceneBackend()->sceneValid()) {
		auto* textBox = new QTextEdit(summaryBox);
		textBox->setAcceptRichText(false);

		QString message(application->sceneBackend()->getValidationReport(core::ErrorLevel::ERROR).c_str());
		if (!message.isEmpty()) {
			textBox->setText(message);
			tabWidget->setCurrentIndex(tabWidget->addTab(textBox, "Ramses Errors"));
			hasErrors_ = true;
		} else {
			message = QString::fromStdString(application->sceneBackend()->getValidationReport(core::ErrorLevel::WARNING));
			if (!message.isEmpty()) {
				textBox->setText(message);
				tabWidget->setCurrentIndex(tabWidget->addTab(textBox, "Ramses Warnings"));
			}
		}
	}

	buttonBox_ = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};

	layout_ = new QGridLayout{this};
	layout_->setRowStretch(0, 0);
	layout_->setRowStretch(1, 1);
	layout_->setRowStretch(2, 0);
	layout_->addWidget(content, 0, 0);
	layout_->addWidget(summaryBox, 1, 0);
	layout_->addWidget(buttonBox_, 2, 0);
	connect(buttonBox_, &QDialogButtonBox::accepted, this, &ExportDialog::exportProject);
	connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));

	QObject::connect(pathEdit_, &QLineEdit::textChanged, this, &ExportDialog::updateButtonStates);
	QObject::connect(ramsesEdit_, &QLineEdit::textChanged, this, &ExportDialog::updateButtonStates);
	QObject::connect(logicEdit_, &QLineEdit::textChanged, this, &ExportDialog::updateButtonStates);
	updateButtonStates();
}

void ExportDialog::exportProject() {
	std::string error;

	auto dir = pathEdit_->text().toStdString();
	auto ramsesFilePath = raco::utils::u8path(ramsesEdit_->text().toStdString()).normalizedAbsolutePath(dir);
	auto rlogicFilePath = raco::utils::u8path(logicEdit_->text().toStdString()).normalizedAbsolutePath(dir);

	if (application_->exportProject(
			ramsesFilePath.string(),
			rlogicFilePath.string(),
			compressEdit_->isChecked(), error, true)) {
		accept();
	} else {
		QMessageBox::critical(
			this, "Export Error", error.c_str(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void ExportDialog::updateButtonStates() {
	const bool enableButton = !ramsesEdit_->text().isEmpty() && !logicEdit_->text().isEmpty() && !pathEdit_->text().isEmpty();
	buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(enableButton);
	buttonBox_->button(QDialogButtonBox::Ok)->setText(enableButton && hasErrors_ ? "Export (with errors)" : "Export");
}

}  // namespace raco::common_widgets
