/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/ExportDialog.h"

#include "common_widgets/ErrorView.h"
#include "common_widgets/PropertyBrowserButton.h"
#include "components/RaCoNameConstants.h"
#include "core/SceneBackendInterface.h"
#include "user_types/Animation.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderPass.h"
#include "utils/u8path.h"

#include <QFileDialog>
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

using namespace raco;

const std::vector<std::string> userTypesAllowedInSceneGraph {
	user_types::Node::typeDescription.typeName,
	user_types::MeshNode::typeDescription.typeName,
	user_types::Prefab::typeDescription.typeName,
	user_types::PrefabInstance::typeDescription.typeName,
	user_types::OrthographicCamera::typeDescription.typeName,
	user_types::PerspectiveCamera::typeDescription.typeName,
//	user_types::Animation::typeDescription.typeName, // Animations currently aren't properly parented inside the Scene Graph
//	user_types::LuaScript::typeDescription.typeName, // LuaScripts currently aren't properly parented inside the Scene Graph
//	user_types::LuaInterface::typeDescription.typeName // LuaInterfaces currently aren't properly parented inside the Scene Graph
};

QStandardItemModel* createSummaryModel(const std::vector<core::SceneBackendInterface::SceneItemDesc>& sceneItems, QObject* parent) {
	auto* listViewModel = new QStandardItemModel{parent};
	listViewModel->setColumnCount(2);
	listViewModel->setHorizontalHeaderItem(0, new QStandardItem{"Name"});
	listViewModel->setHorizontalHeaderItem(1, new QStandardItem{"Type"});

	auto* sceneGraphParent = new QStandardItem("Scene Graph");
	auto* resourceParent = new QStandardItem("Resources");
	listViewModel->appendRow(sceneGraphParent);
	listViewModel->appendRow(resourceParent);

	std::map<const core::SceneBackendInterface::SceneItemDesc*, QStandardItem*> parents{};
	for (const auto& item : sceneItems) {
		using ItemList = QList<QStandardItem*>;
		auto* col0 = new QStandardItem{QString::fromStdString(item.objectName_)};
		auto* col1 = new QStandardItem{QString::fromStdString(item.type_)};
		if (item.parentIndex_ != -1) {
			parents[&sceneItems[item.parentIndex_]]->appendRow(ItemList{} << col0 << col1);
		} else {
			if (std::find(userTypesAllowedInSceneGraph.begin(), userTypesAllowedInSceneGraph.end(), item.type_) != userTypesAllowedInSceneGraph.end()) {
				sceneGraphParent->appendRow(ItemList{} << col0 << col1);
			} else {
				resourceParent->appendRow(ItemList{} << col0 << col1);
			}
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
	auto* contentLayout = new QGridLayout{content};

	contentLayout->setAlignment(Qt::AlignTop);

	ramsesEdit_ = new QLineEdit(content);
	ramsesEdit_->setMinimumWidth(600);
	contentLayout->addWidget(new QLabel{"Ramses file:", content}, 0, 0);
	contentLayout->addWidget(ramsesEdit_, 0, 1);

	auto* selectRamsesDirectoryButton = new PropertyBrowserButton("  ...  ", this);
	contentLayout->addWidget(selectRamsesDirectoryButton, 0, 2);
	setupFilePickerButton(selectRamsesDirectoryButton, ramsesEdit_, names::FILE_EXTENSION_RAMSES_EXPORT);

	compressEdit_ = new QCheckBox(content);
	contentLayout->addWidget(new QLabel{"Compress:", content}, 2, 0);
	contentLayout->addWidget(compressEdit_, 2, 1);
	compressEdit_->setChecked(true);

	contentLayout->addWidget(new QLabel{"Lua saving mode:", content}, 3, 0);
	luaSavingModeCombo_ = new QComboBox(content);
	luaSavingModeCombo_->addItem("SourceCodeOnly", static_cast<int>(application::ELuaSavingMode::SourceCodeOnly));
	luaSavingModeCombo_->addItem("ByteCodeOnly", static_cast<int>(application::ELuaSavingMode::ByteCodeOnly));
	luaSavingModeCombo_->addItem("SourceAndByteCode", static_cast<int>(application::ELuaSavingMode::SourceAndByteCode));
	// SourceCodeOnly is default setting
	luaSavingModeCombo_->setCurrentIndex(0);
	contentLayout->addWidget(luaSavingModeCombo_, 3, 1);

	if (!application_->activeProjectPath().empty()) {
		ramsesEdit_->setText(application_->activeRaCoProject().name() + "." + names::FILE_EXTENSION_RAMSES_EXPORT);
	} else {
		auto currentPath = QString::fromStdString(utils::u8path::current().string() + "/");
		ramsesEdit_->setText(currentPath + QString("unknown.").append(names::FILE_EXTENSION_RAMSES_EXPORT));
	}

	auto* summaryBox = new QGroupBox{"Summary", this};
	auto* summaryBoxLayout = new QVBoxLayout{summaryBox};

	auto* tabWidget = new QTabWidget(summaryBox);
	summaryBoxLayout->addWidget(tabWidget);

	std::string message;
	std::vector<core::SceneBackendInterface::SceneItemDesc> sceneDescription;
	auto sceneStatus = application->getExportSceneDescriptionAndStatus(sceneDescription, message);

	auto* sceneDescriptionView = new QTreeView{tabWidget};
	sceneDescriptionView->setModel(createSummaryModel(sceneDescription, sceneDescriptionView));
	sceneDescriptionView->setMinimumHeight(500);
	sceneDescriptionView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	auto* proxy = sceneDescriptionView->model();
	for (int i = 0; i < proxy->rowCount(); ++i)	{
		auto index = proxy->index(i, 0);
		sceneDescriptionView->expand(index);
	}

	sceneDescriptionView->setColumnWidth(0, 500);
	tabWidget->addTab(sceneDescriptionView, QString{"Scene ID: %1"}.arg(application->sceneBackend()->currentSceneIdValue()));

	auto racoErrorLevel = application->activeRaCoProject().errors()->maxErrorLevel();
	bool showExportWithErrors = racoErrorLevel == core::ErrorLevel::ERROR;
	if (racoErrorLevel >= core::ErrorLevel::WARNING) {
		auto* commandInterface = application->activeRaCoProject().commandInterface();
		auto* errorView = new ErrorView(commandInterface, application->dataChangeDispatcher(), true, logViewModel, this);
		tabWidget->setCurrentIndex(tabWidget->addTab(errorView, racoErrorLevel == core::ErrorLevel::ERROR ? "Composer Errors" : "Composer Warnings"));
	}

	if (sceneStatus != core::ErrorLevel::NONE) {
		auto* textBox = new QTextEdit(summaryBox);
		textBox->setAcceptRichText(false);

		textBox->setText(QString::fromStdString(message));
		if (sceneStatus == core::ErrorLevel::ERROR) {
			showExportWithErrors = true;
		}

		tabWidget->setCurrentIndex(tabWidget->addTab(textBox, sceneStatus == core::ErrorLevel::ERROR ? "Ramses Errors" : "Ramses Warnings"));
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

	QObject::connect(ramsesEdit_, &QLineEdit::textChanged, this, &ExportDialog::updatePaths);
	updatePaths();
	buttonBox_->button(QDialogButtonBox::Ok)->setText(showExportWithErrors ? "Export (with errors)" : "Export");

	setAttribute(Qt::WA_DeleteOnClose);
}

void ExportDialog::exportProject() {
	auto ramsesFilePath = getAbsoluteExportFilePath(ramsesEdit_);

	std::string error;
	if (application_->exportProject(
			ramsesFilePath.string(),
			compressEdit_->isChecked(),
			error,
			true,
			static_cast<application::ELuaSavingMode>(luaSavingModeCombo_->currentData().toInt()))) {
		accept();
	} else {
		QMessageBox::critical(
			this, "Export Error", error.c_str(), QMessageBox::Ok, QMessageBox::Ok);
	}
}

utils::u8path ExportDialog::getAbsoluteExportFilePath(QLineEdit* path) {
	auto dir = utils::u8path(path->text().toStdString());
	if (dir.is_relative()) {
		if (!application_->activeProjectPath().empty()){
			dir = dir.normalizedAbsolutePath(application_->activeProjectFolder());
		}
	}

	return dir;
}

void ExportDialog::updatePaths() {
	auto ramsesPath = getAbsoluteExportFilePath(ramsesEdit_);
	ramsesEdit_->setToolTip(QString::fromStdString(ramsesPath.string()));

	updateButtonStates();
}

void ExportDialog::updateButtonStates() {
	const bool enableButton = !ramsesEdit_->text().isEmpty();
	buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(enableButton);
}

void ExportDialog::setupFilePickerButton(PropertyBrowserButton* button, QLineEdit* pathEdit, const std::string& fileType) {
	QObject::connect(button, &QPushButton::clicked, [this, pathEdit, fileType]() {
		auto currentPath = getAbsoluteExportFilePath(pathEdit);
		auto dir = QString::fromStdString(currentPath.string());
		auto caption = QString::fromStdString(fmt::format("{} file location", fileType));
		auto filter = QString::fromStdString(fmt::format("{0} files (*.{0});;Any files (*)", fileType));
		auto result = QFileDialog::getSaveFileName(this, caption, dir, filter);
		if (!result.isEmpty()) {
			if (!application_->activeProjectPath().empty()) {
				auto newPath = utils::u8path(result.toStdString());
				auto projectFolder = application_->activeProjectFolder();
				auto relativePath = newPath.normalizedRelativePath(projectFolder);
				pathEdit->setText(QString::fromStdString(relativePath.string()));
			} else {
				pathEdit->setText(result);
			}
		}
	});
}

}  // namespace raco::common_widgets
