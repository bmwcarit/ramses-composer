/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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
#include "user_types/PrefabInstance.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>

namespace {

QString errorLevelToString(const raco::core::ErrorLevel &level) {
	switch (level) {
		case raco::core::ErrorLevel::NONE:
			return "None";
		case raco::core::ErrorLevel::INFORMATION:
			return "Information";
		case raco::core::ErrorLevel::WARNING:
			return "Warning";
		case raco::core::ErrorLevel::ERROR:
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
		auto originalData = QStandardItemModel::data(index, role);

		if (column == raco::common_widgets::ErrorView::ErrorViewColumns::MESSAGE && role == Qt::DisplayRole) {
			// no multi-line error messages in Error View.
			return originalData.toString().replace('\n', ' ');
		}

		return originalData;
	}
};


}  // namespace

namespace raco::common_widgets {

ErrorView::ErrorView(raco::core::CommandInterface* commandInterface, raco::components::SDataChangeDispatcher dispatcher, bool embeddedInExportView, LogViewModel* logViewModel, QWidget* parent) :
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
	const auto WARNING = errorLevelToString(raco::core::ErrorLevel::WARNING);

	for (auto row = 0; row < tableModel_->rowCount(); ++row) {
		auto modelIndex = proxyModel_->mapToSource(proxyModel_->index(row, LEVEL));
		auto errorLevel = tableModel_->itemFromIndex(modelIndex)->text();

		tableView_->setRowHidden(row, (errorLevel == WARNING) ? !warningsVisible : !errorsVisible);
	}
}


void ErrorView::regenerateTable() {
	auto cachedSelectedRow = rowToVector(proxyModel_->mapToSource(tableView_->selectionModel()->currentIndex()));

	tableModel_->removeRows(0, tableModel_->rowCount());
	objIDs_.clear();
	indexToObjID_.clear();

	auto warningAmount = 0;
	auto errorAmount = 0;

	const auto& errors = commandInterface_->errors();
	for (const auto& [errorValueHandle, error] : errors.getAllErrors()) {
		if (error.level() <= core::ErrorLevel::INFORMATION) {
			continue;
		}

		auto rootObj = !errorValueHandle ? nullptr : errorValueHandle.rootObject();

		// TODO: RAOS-815 - we need to clarify what we actually want to show/count
		if (!rootObj || !raco::core::Queries::isReadOnly(rootObj) || rootObj->query<core::ExternalReferenceAnnotation>()) {
			QList<QStandardItem*> items;
			items.append(new QStandardItem(errorLevelToString(error.level())));
			items.append(new QStandardItem(QString::fromStdString(rootObj ? rootObj->objectName() : "(This Project)")));
			items.append(new QStandardItem(QString::fromStdString((errorValueHandle && errorValueHandle.isProperty()) ? errorValueHandle.getPropName() : "")));

			auto errorMsg = QString::fromStdString(error.message());
			auto errorMessageItem = new QStandardItem(errorMsg);
			errorMessageItem->setToolTip(errorMsg);
			items.append(errorMessageItem);

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

	updateErrorAmountLabel();

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

	treeViewMenu->exec(tableView_->viewport()->mapToGlobal(p));
}

void ErrorView::updateErrorAmountLabel() {
	if (errorAmountLabel_) {

		auto warningAmount = 0;
		auto errorAmount = 0;

		const auto& errors = commandInterface_->errors();
		for (const auto& [errorValueHandle, error] : errors.getAllErrors()) {
			if (error.level() <= core::ErrorLevel::INFORMATION) {
				continue;
			}

			auto rootObj = !errorValueHandle ? nullptr : errorValueHandle.rootObject();

			// TODO: RAOS-815 - we need to clarify what we actually want to show/count
			if (!rootObj || !raco::core::Queries::isReadOnly(rootObj) || rootObj->query<core::ExternalReferenceAnnotation>()) {
				if (error.level() == core::ErrorLevel::WARNING) {
					++warningAmount;
				} else if (error.level() == core::ErrorLevel::ERROR) {
					++errorAmount;
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

		errorAmountLabel_->setText(QString::fromStdString(fmt::format("Warnings: {} Errors: {}{}", warningAmount, errorAmount, logErrorString)));
	}
}

}  // namespace raco::common_widgets
