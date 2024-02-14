/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/ErrorView.h"

#include "common_widgets/log_model/LogViewModel.h"
#include "common_widgets/NoContentMarginsLayout.h"
#include "common_widgets/RaCoClipboard.h"
#include "core/CommandInterface.h"
#include "core/Errors.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Queries.h"
#include "user_types/LuaScript.h"
#include "style/Colors.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>

namespace {

using namespace raco;

inline const int extRefProjectIdUserRole = Qt::UserRole;

QString errorLevelToString(const core::ErrorLevel &level) {
	switch (level) {
		case core::ErrorLevel::NONE:
			return "None";
		case core::ErrorLevel::INFORMATION:
			return "Information";
		case core::ErrorLevel::WARNING:
			return "Warning";
		case core::ErrorLevel::ERROR:
			return "Error";
		default:
			assert(false && "Unknown error level detected when trying to regenerate Error View");
			return "";
	}
}

class ErrorViewModel : public QStandardItemModel {
public:
	ErrorViewModel(QWidget* parent = nullptr) : QStandardItemModel(parent) {}

protected:
	QVariant data(const QModelIndex& index, int role) const override {
		auto column = index.column();
		if (role == Qt::ForegroundRole) {
			if (QStandardItemModel::data(index, extRefProjectIdUserRole).isValid()) {
				if (column == common_widgets::ErrorView::ErrorViewColumns::OBJECT) {
					return style::Colors::color(style::Colormap::externalReference);
				}
			}
		}

		auto originalData = QStandardItemModel::data(index, role);

		if (column == common_widgets::ErrorView::ErrorViewColumns::MESSAGE && role == Qt::DisplayRole) {
			// no multi-line error messages in Error View.
			return originalData.toString().replace('\n', ' ');
		}

		return originalData;
	}
};


}  // namespace

namespace raco::common_widgets {

ErrorView::ErrorView(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, bool embeddedInExportView, LogViewModel* logViewModel, QWidget* parent) :
	QWidget{parent}, commandInterface_{commandInterface}, logViewModel_(logViewModel), showFilterLayout_(!embeddedInExportView) {
	auto* errorViewLayout{new NoContentMarginsLayout<QVBoxLayout>{this}};

	tableModel_ = new ErrorViewModel(this);
	tableModel_->setColumnCount(ErrorViewColumns::COLUMN_COUNT);
	tableModel_->setHorizontalHeaderLabels({"Level", "Object", "Property", "Message"});

	proxyModel_ = new QSortFilterProxyModel(this);
	proxyModel_->setSourceModel(tableModel_);

	tableView_ = new QTableView(this);
	tableView_->setModel(proxyModel_);
	tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tableView_->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	tableView_->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
	tableView_->setSortingEnabled(true);
	tableView_->setContextMenuPolicy(Qt::CustomContextMenu);
	tableView_->setAlternatingRowColors(true);
	tableView_->horizontalHeader()->setStretchLastSection(true);
	tableView_->verticalHeader()->setVisible(false);
	tableView_->verticalHeader()->setMinimumSectionSize(ROW_HEIGHT);
	tableView_->verticalHeader()->setMaximumSectionSize(ROW_HEIGHT);
	tableView_->verticalHeader()->setDefaultSectionSize(ROW_HEIGHT);
	connect(tableView_, &QTableView::doubleClicked, [this](const auto& selectedIndex) {
		auto modelIndex = proxyModel_->mapToSource(selectedIndex);
		Q_EMIT objectSelectionRequested(QString::fromStdString(indexToObjID_[modelIndex.row()]));
	});
	connect(tableView_, &QTableView::customContextMenuRequested, this, &ErrorView::createCustomContextMenu);
	errorViewLayout->addWidget(tableView_);

	if (showFilterLayout_) {
		auto filterLayout = new QHBoxLayout{this};
		filterLayout->setContentsMargins(5, 0, 10, 0);
		showWarningsCheckBox_ = new QCheckBox("Show Warnings", this);
		showWarningsCheckBox_->setChecked(true);
		connect(showWarningsCheckBox_, &QCheckBox::stateChanged, [this]() {
			filterRows();
		});
		filterLayout->addWidget(showWarningsCheckBox_);

		showErrorsCheckBox_ = new QCheckBox("Show Errors", this);
		showErrorsCheckBox_->setChecked(true);
		connect(showErrorsCheckBox_, &QCheckBox::stateChanged, [this]() {
			filterRows();
		});
		filterLayout->addWidget(showErrorsCheckBox_);

		showExternalReferencesCheckBox_ = new QCheckBox("Show Externals", this);
		showExternalReferencesCheckBox_->setChecked(true);
		connect(showExternalReferencesCheckBox_, &QCheckBox::stateChanged, [this]() {
			filterRows();
		});
		filterLayout->addWidget(showExternalReferencesCheckBox_);

		filterLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

		errorAmountLabel_ = new QLabel(this);
		filterLayout->addWidget(errorAmountLabel_);

		errorViewLayout->addLayout(filterLayout);

		if (logViewModel) {
			connect(logViewModel, &LogViewModel::entriesChanged, this, &ErrorView::updateErrorAmountLabel);
		}
	}

	regenerateTable();

	if (embeddedInExportView) {
		// embedded Error View is in modal window that does not allow any scene/property change
		return;
	}

	errorChangeSubscription_ = dispatcher->registerOnErrorChangedInScene([this]() {
		regenerateTable();
	});

	objNameChangeSubscription_ = dispatcher->registerOnPropertyChange("objectName", [this](core::ValueHandle handle) {
		if (objIDs_.count(handle.rootObject()->objectID()) > 0) {
			regenerateTable();
		}
	});

	objChildrenChangeSubscription_ = dispatcher->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		regenerateTable();
	});

}

std::vector<std::string> ErrorView::rowToVector(const QModelIndex& rowIndex) const {
	if (!rowIndex.isValid()) {
		return {};
	}

	std::vector<std::string> selectedRowData(ErrorViewColumns::COLUMN_COUNT + 1);
	auto row = rowIndex.row();
	for (auto column = 0; column < ErrorViewColumns::COLUMN_COUNT; ++column) {
		selectedRowData[column] = tableModel_->data(tableModel_->index(row, column)).toString().toStdString();
	}
	selectedRowData[ErrorViewColumns::COLUMN_COUNT] = indexToObjID_[row];

	return selectedRowData;
}

void ErrorView::filterRows() {
	auto warningsVisible = !showWarningsCheckBox_ || showWarningsCheckBox_->isChecked();
	auto errorsVisible = !showErrorsCheckBox_ || showErrorsCheckBox_->isChecked();
	auto extRefsVisible = !showExternalReferencesCheckBox_ || showExternalReferencesCheckBox_->isChecked();
	const auto WARNING = errorLevelToString(core::ErrorLevel::WARNING);

	for (auto row = 0; row < tableModel_->rowCount(); ++row) {
		auto modelIndex = proxyModel_->mapToSource(proxyModel_->index(row, LEVEL));
		auto errorLevel = tableModel_->itemFromIndex(modelIndex)->text();
		auto hiddenByLevel = (errorLevel == WARNING) ? !warningsVisible : !errorsVisible;

		if (hiddenByLevel) {
			tableView_->setRowHidden(row, true);
			continue;
		}

		auto isExtRef = tableModel_->itemFromIndex(modelIndex)->data(extRefProjectIdUserRole).isValid();
		tableView_->setRowHidden(row, !extRefsVisible && isExtRef);
	}

	updateErrorAmountLabel();
}


void ErrorView::regenerateTable() {
	auto cachedSelectedRow = rowToVector(proxyModel_->mapToSource(tableView_->selectionModel()->currentIndex()));

	tableModel_->removeRows(0, tableModel_->rowCount());
	objIDs_.clear();
	indexToObjID_.clear();

	
	for (const auto& [object, objErrors] : commandInterface_->errors().getAllErrors()) {
		for (const auto& [errorValueHandle, error] : objErrors) {
			if (error.level() <= core::ErrorLevel::INFORMATION) {
				continue;
			}

			auto rootObj = !errorValueHandle ? nullptr : errorValueHandle.rootObject();

			QList<QStandardItem*> items;
			items.append(new QStandardItem(errorLevelToString(error.level())));
			auto* objectItem = new QStandardItem(QString::fromStdString(rootObj ? rootObj->objectName() : "(This Project)"));
			items.append(objectItem);
			items.append(new QStandardItem(QString::fromStdString((errorValueHandle && errorValueHandle.isProperty()) ? errorValueHandle.getPropName() : "")));

			auto errorMsg = QString::fromStdString(error.message());
			auto errorMessageItem = new QStandardItem(errorMsg);
			errorMessageItem->setToolTip(errorMsg);
			items.append(errorMessageItem);

			auto extRefAnno = rootObj ? rootObj->query<core::ExternalReferenceAnnotation>() : nullptr;
			if (extRefAnno) {
				auto projectId = QString::fromStdString(extRefAnno->projectID_.asString());
				auto projectName = commandInterface_->project()->lookupExternalProjectName(*extRefAnno->projectID_);
				objectItem->setToolTip(QString::fromStdString(projectName));
				for (const auto& item : items) {
					item->setData(projectId, extRefProjectIdUserRole);
				}
			}

			tableModel_->appendRow(items);

			auto errorID = rootObj ? rootObj->objectID() : commandInterface_->project()->currentPath();
			indexToObjID_.emplace_back(errorID);
			objIDs_.insert(errorID);
		}
	}

	if (!cachedSelectedRow.empty()) {
		for (auto row = 0; row < tableModel_->rowCount() && !tableView_->selectionModel()->hasSelection(); ++row) {
			auto currentRow = rowToVector(tableModel_->index(row, 0));

			// ignore object name when comparing rows because it might have changed after regenerating.
			if (currentRow[LEVEL] == cachedSelectedRow[LEVEL]
				&& currentRow[PROPERTY] == cachedSelectedRow[PROPERTY]
				&& currentRow[MESSAGE] == cachedSelectedRow[MESSAGE]
				&& currentRow.back() == cachedSelectedRow.back()) {
				auto rowToSelect = proxyModel_->mapFromSource(tableModel_->index(row, 0));
				tableView_->selectionModel()->setCurrentIndex(rowToSelect, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
				tableView_->scrollTo(rowToSelect);
			}
		}
	}

	filterRows();
}

void ErrorView::createCustomContextMenu(const QPoint& p) {
	auto modelIndex = proxyModel_->mapToSource(tableView_->indexAt(p));
	if (!modelIndex.isValid()) {
		return;
	}

	auto* treeViewMenu = new QMenu(this);
	treeViewMenu->addAction("Go to Object", this, [this, &modelIndex]() {
		Q_EMIT objectSelectionRequested(QString::fromStdString(indexToObjID_[modelIndex.row()]));
	});
	treeViewMenu->addAction("Copy Message to Clipboard", this, [this, &modelIndex]() {
		auto errorMessage = tableModel_->item(modelIndex.row(), ErrorViewColumns::MESSAGE)->text().toStdString();
		RaCoClipboard::set(errorMessage);
	});

	auto projectId = modelIndex.data(extRefProjectIdUserRole).toString();
	if (projectId != nullptr) {
		auto projectPath = commandInterface_->project()->lookupExternalProjectPath(projectId.toStdString());
		auto actionOpenProject = treeViewMenu->addAction("Open External Project", [this, projectPath]() {
			if (!projectPath.empty()) {
				auto appPath = QCoreApplication::applicationFilePath();
				QProcess::startDetached(appPath, QStringList(QString::fromStdString(projectPath)));
			}
		});
		actionOpenProject->setEnabled(!projectPath.empty());
	}

	treeViewMenu->exec(tableView_->viewport()->mapToGlobal(p));
}

void ErrorView::updateErrorAmountLabel() {
	if (errorAmountLabel_ == nullptr) {
		return;
	}

	auto displayedWarningAmount = 0;
	auto totalWarningAmount = 0;
	auto displayedErrorAmount = 0;
	auto totalErrorAmount = 0;

	for (const auto& [object, objErrors] : commandInterface_->errors().getAllErrors()) {
		for (const auto& [errorValueHandle, error] : objErrors) {
			if (error.level() <= core::ErrorLevel::INFORMATION) {
				continue;
			}

			auto rootObj = !errorValueHandle ? nullptr : errorValueHandle.rootObject();

			if (error.level() == core::ErrorLevel::WARNING) {
				++totalWarningAmount;
				if (showWarningsCheckBox_->isChecked() && (!rootObj || !rootObj->query<core::ExternalReferenceAnnotation>() || showExternalReferencesCheckBox_->isChecked())) {
					++displayedWarningAmount;
				}
			} else if (error.level() == core::ErrorLevel::ERROR) {
				++totalErrorAmount;
				if (showErrorsCheckBox_->isChecked() && (!rootObj || !rootObj->query<core::ExternalReferenceAnnotation>() || showExternalReferencesCheckBox_->isChecked())) {
					++displayedErrorAmount;
				}
			}
		}
	}

	std::string logErrorString = "";
	if (logViewModel_) {
		auto logErrorAmount = logViewModel_->errorCount();
		if (logErrorAmount > 0) {
			logErrorString = fmt::format(" (additional errors in log)");
		}
	}

	errorAmountLabel_->setText(QString::fromStdString(fmt::format("Warnings: {} / {} Errors: {} / {}{}", displayedWarningAmount, totalWarningAmount, displayedErrorAmount, totalErrorAmount, logErrorString)));
}

}  // namespace raco::common_widgets
