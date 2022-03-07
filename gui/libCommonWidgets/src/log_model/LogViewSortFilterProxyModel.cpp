/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/log_model/LogViewSortFilterProxyModel.h"

namespace raco::common_widgets {

LogViewSortFilterProxyModel::LogViewSortFilterProxyModel(LogViewModel* model, QObject* parent) : QSortFilterProxyModel(parent), model_(model) {
	setSourceModel(model);
}

void LogViewSortFilterProxyModel::setFilterMinLogLevel(spdlog::level::level_enum filterMinLogLevel) {
	filterMinLogLevel_ = filterMinLogLevel;
	invalidateFilter();
}
void LogViewSortFilterProxyModel::setFilterCategory(QString filterCategory) {
	filterCategory_ = filterCategory;
	invalidateFilter();
}

void LogViewSortFilterProxyModel::setFilterMessage(QString filterMessage) {
	filterMessage_ = filterMessage;
	invalidateFilter();
}

bool LogViewSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
	// For the timestamp column, we just sort by row index instead to avoid duplicate timestamps messing up the sort order.
	if (sortColumn() == LogViewModel::COLUMNINDEX_TIMESTAMP) {
		return source_left.row() < source_right.row();
	} else if (sortColumn() == LogViewModel::COLUMNINDEX_LEVEL) {
		return model_->entries()[source_left.row()].level_ < model_->entries()[source_right.row()].level_;
	}

	return QSortFilterProxyModel::lessThan(source_left, source_right);
}

bool LogViewSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	return filterAcceptsEntry(model_->entries()[sourceRow]);
}

bool LogViewSortFilterProxyModel::filterAcceptsEntry(const LogViewModel::LogEntry& entry) const {
	return entry.level_ >= filterMinLogLevel_ &&
		   (filterCategory_.isEmpty() || entry.category_ == filterCategory_) &&
		   (filterMessage_.isEmpty() || entry.message_.contains(filterMessage_, Qt::CaseInsensitive));
}

int LogViewSortFilterProxyModel::warningCount() const {
	return std::count_if(model_->entries().begin(), model_->entries().end(), [this](const LogViewModel::LogEntry& entry) { return filterAcceptsEntry(entry) && entry.level_ == spdlog::level::warn; });
}

int LogViewSortFilterProxyModel::errorCount() const {
	return std::count_if(model_->entries().begin(), model_->entries().end(), [this](const LogViewModel::LogEntry& entry) { return filterAcceptsEntry(entry) && (entry.level_ == spdlog::level::err || entry.level_ == spdlog::level::critical); });
}

std::string LogViewSortFilterProxyModel::getStringFromSelection(QItemSelection selection) const {
	return model_->getStringFromSelection(mapSelectionToSource(selection));
}

}  // namespace raco::common_widgets
