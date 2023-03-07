/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/LogView.h"

#include "common_widgets/NoContentMarginsLayout.h"

#include <QApplication>
#include <QComboBox>
#include <QClipboard>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QShortcut>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>


namespace raco::common_widgets {

LogView::LogView(LogViewModel* model, QWidget* parent) : model_(model) {

	proxyModel_ = new LogViewSortFilterProxyModel(model_, this);

	auto* logViewLayout{new NoContentMarginsLayout<QVBoxLayout>{this}};

	tableView_ = new QTableView(this);
	tableView_->setModel(proxyModel_);
	tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tableView_->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	tableView_->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
	tableView_->setContextMenuPolicy(Qt::CustomContextMenu);
	tableView_->setAlternatingRowColors(true);
	tableView_->setWordWrap(true);
	tableView_->setSortingEnabled(true);
	tableView_->setColumnWidth(LogViewModel::COLUMNINDEX_TIMESTAMP, 120);
	tableView_->setColumnWidth(LogViewModel::COLUMNINDEX_LEVEL, 60);
	tableView_->setColumnWidth(LogViewModel::COLUMNINDEX_CATEGORY, 120);
	tableView_->verticalHeader()->hide();
	tableView_->verticalHeader()->setDefaultSectionSize(18);
	tableView_->horizontalHeader()->setStretchLastSection(true);
	connect(tableView_, &QTableView::customContextMenuRequested, this, &LogView::customMenuRequested);

	auto* clearButton = new QPushButton("Clear Log", this);
	connect(clearButton, &QPushButton::clicked, model_, &LogViewModel::clear);

	auto filterLevelBox = new QComboBox(this);
	filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::trace), spdlog::level::trace);
	filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::debug), spdlog::level::debug);
	filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::info), spdlog::level::info);
	filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::warn), spdlog::level::warn);
	filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::err), spdlog::level::err);
	// Let's not allow users to hide errors, so disable filter for critical log level
	// filterLevelBox->addItem(LogViewModel::textForLogLevel(spdlog::level::critical), spdlog::level::critical);
	QObject::connect(filterLevelBox, qOverload<int>(&QComboBox::activated), this, [this, filterLevelBox](int index) {
		auto filter = static_cast<spdlog::level::level_enum>(filterLevelBox->itemData(index, Qt::UserRole).toInt());
		proxyModel_->setFilterMinLogLevel(filter);
		updateWarningErrorLabel();
	});
	filterLevelBox->setCurrentIndex(2);
	proxyModel_->setFilterMinLogLevel(spdlog::level::info);

	auto filterCategoryBox = new QComboBox(this);
	filterCategoryBox->addItem("All Categories", "");
	filterCategoryBox->addItem(raco::log_system::DEFAULT, raco::log_system::DEFAULT);
	filterCategoryBox->addItem(raco::log_system::COMMON, raco::log_system::COMMON);
	filterCategoryBox->addItem(raco::log_system::CONTEXT, raco::log_system::CONTEXT);
	filterCategoryBox->addItem(raco::log_system::USER_TYPES, raco::log_system::USER_TYPES);
	filterCategoryBox->addItem(raco::log_system::PROPERTY_BROWSER, raco::log_system::PROPERTY_BROWSER);
	filterCategoryBox->addItem(raco::log_system::OBJECT_TREE_VIEW, raco::log_system::OBJECT_TREE_VIEW);
	filterCategoryBox->addItem(raco::log_system::LOGGING, raco::log_system::LOGGING);
	filterCategoryBox->addItem(raco::log_system::PREVIEW_WIDGET, raco::log_system::PREVIEW_WIDGET);
	filterCategoryBox->addItem(raco::log_system::RAMSES_BACKEND, raco::log_system::RAMSES_BACKEND);
	filterCategoryBox->addItem(raco::log_system::RAMSES_ADAPTOR, raco::log_system::RAMSES_ADAPTOR);
	filterCategoryBox->addItem(raco::log_system::DESERIALIZATION, raco::log_system::DESERIALIZATION);
	filterCategoryBox->addItem(raco::log_system::PROJECT, raco::log_system::PROJECT);
	filterCategoryBox->addItem(raco::log_system::PYTHON, raco::log_system::PYTHON);
	filterCategoryBox->addItem(raco::log_system::MESH_LOADER, raco::log_system::MESH_LOADER);
	filterCategoryBox->addItem(raco::log_system::RAMSES, raco::log_system::RAMSES);
	filterCategoryBox->addItem(raco::log_system::RAMSES_LOGIC, raco::log_system::RAMSES_LOGIC);
	QObject::connect(filterCategoryBox, qOverload<int>(&QComboBox::activated), this, [this, filterCategoryBox](int index) {
		auto filter = filterCategoryBox->itemData(index, Qt::UserRole).toString();
		proxyModel_->setFilterCategory(filter);
		updateWarningErrorLabel();
	});

	auto searchLineEdit = new QLineEdit(this);
	searchLineEdit->setFixedHeight(18);
	searchLineEdit->setFixedWidth(150);
	searchLineEdit->setPlaceholderText("Search Message...");
	QObject::connect(searchLineEdit, &QLineEdit::textChanged, this, [this](QString filter) {
		proxyModel_->setFilterMessage(filter);
		updateWarningErrorLabel();
	});

	warningErrorLabel_ = new QLabel(this);
	QObject::connect(model_, &LogViewModel::entriesChanged, this, &LogView::updateWarningErrorLabel);
	updateWarningErrorLabel();

	auto* filterLayoutWidget = new QWidget(this);
	auto* filterLayout = new NoContentMarginsLayout<QHBoxLayout>(filterLayoutWidget);
	filterLayout->addItem(new QSpacerItem(5, 0));
	filterLayout->addWidget(clearButton);
	filterLayout->addWidget(filterLevelBox);
	filterLayout->addWidget(filterCategoryBox);
	filterLayout->addWidget(searchLineEdit);
	filterLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
	filterLayout->addWidget(warningErrorLabel_);
	filterLayout->addItem(new QSpacerItem(5, 0));
		
	logViewLayout->addWidget(tableView_);
	logViewLayout->addWidget(filterLayoutWidget);

	auto copyShortcut = new QShortcut(QKeySequence::Copy, this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
	QObject::connect(copyShortcut, &QShortcut::activated, this, [this]() {
		copySelectedRows();
	});
}

void LogView::customMenuRequested(QPoint pos) {

	if (!tableView_->selectionModel()->hasSelection()) {
		return;
	}

	QMenu* menu = new QMenu(this);
	menu->addAction("Copy", [this]() { copySelectedRows(); }, QKeySequence::Copy);
	menu->popup(tableView_->viewport()->mapToGlobal(pos));
}

void LogView::updateWarningErrorLabel() {
	warningErrorLabel_->setText(QString::fromStdString(fmt::format("Warnings: {} / {} Errors: {} / {}",
		proxyModel_->warningCount(),
		model_->warningCount(),
		proxyModel_->errorCount(),
		model_->errorCount())));
}

void LogView::copySelectedRows() {
	auto selection = tableView_->selectionModel()->selection();

	if (!selection.isEmpty()) {
		QMimeData* mimeData = new QMimeData();
		mimeData->setData("text/plain", proxyModel_->getStringFromSelection(selection).c_str());
		QApplication::clipboard()->setMimeData(mimeData);
	}
}


}  // namespace raco::common_widgets
