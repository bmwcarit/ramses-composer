/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/log_model/LogViewModel.h"

#include "common_widgets/log_model/LogViewSink.h"

#include "style/Colors.h"

#include <sstream>

namespace raco::common_widgets {

QString LogViewModel::textForLogLevel(spdlog::level::level_enum level) {
	using namespace spdlog::level;

	switch (level) {
		case level_enum::trace:
			return "Trace";
		case level_enum::debug:
			return "Debug";
		case level_enum::info:
			return "Info";
		case level_enum::warn:
			return "Warn";
		case level_enum::err:
			return "Error";
		case level_enum::critical:
			return "Critical";
		default:
			return "Other";
	}
}

QColor LogViewModel::colorForLogLevel(spdlog::level::level_enum level) {
	using namespace spdlog::level;
	using namespace raco::style;

	switch (level) {
		case level_enum::trace:
			return Colors::color(Colormap::textDisabled);
		case level_enum::debug:
			return Colors::color(Colormap::textDisabled);
		case level_enum::info:
			return Colors::color(Colormap::text);
		case level_enum::warn:
			return Colors::color(Colormap::warningColor);
		case level_enum::err:
			return Colors::color(Colormap::errorColor);
		case level_enum::critical:
			return Colors::color(Colormap::errorColor).lighter();
		default:
			return Colors::color(Colormap::text);
	}
}

LogViewModel::LogViewModel(QObject* parent) : QAbstractItemModel(parent), boldFont_() {
	boldFont_.setBold(true);
	flushTimer_ = new QTimer(this);
	flushTimer_->setSingleShot(true);
	connect(flushTimer_, &QTimer::timeout, this, &LogViewModel::flushNewEntries);

	qRegisterMetaType<LogEntry>("LogViewModel::LogEntry");

	sink_ = std::make_shared<LogViewSink>(this);
	log_system::registerSink(sink_);
}

LogViewModel::~LogViewModel() {
	log_system::unregisterSink(sink_);
}

QModelIndex LogViewModel::index(int row, int column, const QModelIndex& parent) const {
	assert(!parent.isValid());
	return createIndex(row, column);
};

QModelIndex LogViewModel::parent(const QModelIndex& child) const {
	return {};
}

QVariant LogViewModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case COLUMNINDEX_TIMESTAMP:
				return entries_[index.row()].timestamp_;
			case COLUMNINDEX_LEVEL:
				return textForLogLevel(entries_[index.row()].level_);
			case COLUMNINDEX_CATEGORY:
				return entries_[index.row()].category_;
			case COLUMNINDEX_MESSAGE:
				return entries_[index.row()].message_;
		}
	}
	if (index.column() == COLUMNINDEX_LEVEL) {
		if (role == Qt::UserRole) {
			return entries_[index.row()].level_;
		}
		if (role == Qt::ForegroundRole) {
			return colorForLogLevel(entries_[index.row()].level_);
		}
		if (role == Qt::FontRole && entries_[index.row()].level_ >= spdlog::level::warn) {
			return boldFont_;
		}
		
	}


	return {};
}

int LogViewModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : entries_.size();
}

int LogViewModel::columnCount(const QModelIndex& parent) const {
	return COLUMN_COUNT;
}

QVariant LogViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		switch (section) {
			case COLUMNINDEX_TIMESTAMP:
				return "Timestamp";
			case COLUMNINDEX_LEVEL:
				return "Level";
			case COLUMNINDEX_CATEGORY:
				return "Category";
			case COLUMNINDEX_MESSAGE:
				return "Message";
		}
	}

	return {};
}

const std::vector<LogViewModel::LogEntry>& LogViewModel::entries() const {
	return entries_;
}

void LogViewModel::addEntry(const LogEntry& entry) {
	entriesToAdd_.emplace_back(entry);

	if (!flushTimer_->isActive()) {
		flushTimer_->start(FLUSH_TIMER_INTERVAL);
	}
}

void LogViewModel::clear() {
	beginResetModel();
	entries_.clear();
	endResetModel();
	entriesToAdd_.clear();

	Q_EMIT entriesChanged();
}

void LogViewModel::flushNewEntries() {
	if (entriesToAdd_.empty()) {
		return;
	}

	if (entriesToAdd_.size() > MAX_ENTRY_COUNT) {
		entriesToAdd_.erase(entriesToAdd_.begin(), entriesToAdd_.begin() + (entriesToAdd_.size() - MAX_ENTRY_COUNT));
	}

	if (entries_.size() + entriesToAdd_.size() > MAX_ENTRY_COUNT) {
		beginRemoveRows({}, 0, entries_.size() + entriesToAdd_.size() - MAX_ENTRY_COUNT - 1);
		entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() + entriesToAdd_.size() - MAX_ENTRY_COUNT));
		endRemoveRows();
	}

	beginInsertRows({}, entries_.size(), entries_.size() + entriesToAdd_.size() - 1);
	entries_.insert(entries_.end(), entriesToAdd_.begin(), entriesToAdd_.end());
	endInsertRows();
	entriesToAdd_.clear();

	Q_EMIT entriesChanged();
}

int LogViewModel::warningCount() const {
	return std::count_if(entries_.begin(), entries_.end(), [](const LogEntry& entry) { return entry.level_ == spdlog::level::warn; });
}

int LogViewModel::errorCount() const {
	return std::count_if(entries_.begin(), entries_.end(), [](const LogEntry& entry) { return entry.level_ == spdlog::level::err || entry.level_ == spdlog::level::critical; });
}

std::string LogViewModel::getStringFromSelection(QItemSelection selection) const {
	std::stringstream result;
	for (const auto& index : selection.indexes()) {
		if (index.column() == 0) {
			const auto& entry = entries_[index.row()];
			result << entry.fullLogMessageRaw_.toStdString();
		}
	}
	return result.str();

}
}  // namespace raco::common_widgets
