/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "log_system/log.h"

#include <chrono>
#include <memory>
#include <vector>
#include <QAbstractItemModel>
#include <QItemSelection>
#include <QFont>
#include <QTimer>

namespace raco::common_widgets {

class LogViewSink;

class LogViewModel : public QAbstractItemModel {
	Q_OBJECT

public:
	enum ColumnIndex {
		COLUMNINDEX_TIMESTAMP,
		COLUMNINDEX_LEVEL,
		COLUMNINDEX_CATEGORY,
		COLUMNINDEX_MESSAGE,
		COLUMN_COUNT
	};

	struct LogEntry {
		QString timestamp_;
		spdlog::level::level_enum level_;
		QString category_;
		QString message_;
		QString fullLogMessageRaw_;
	};

	static QString textForLogLevel(spdlog::level::level_enum level);
	static QColor colorForLogLevel(spdlog::level::level_enum level);

	explicit LogViewModel(QObject* parent = nullptr);
	~LogViewModel();

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	const std::vector<LogViewModel::LogEntry>& entries() const;

	int warningCount() const;
	int errorCount() const;

	std::string getStringFromSelection(QItemSelection selection) const;

Q_SIGNALS:
	void entriesChanged();

public Q_SLOTS:
	void addEntry(const LogViewModel::LogEntry& entry);
	void clear();

private Q_SLOTS:
	void flushNewEntries();

private:	
	const std::chrono::milliseconds FLUSH_TIMER_INTERVAL = std::chrono::milliseconds(100);
	const int MAX_ENTRY_COUNT = 10000;
		
	std::shared_ptr<LogViewSink> sink_;
	std::vector<LogEntry> entries_;
	std::vector<LogEntry> entriesToAdd_;
	QFont boldFont_;
	QTimer* flushTimer_;
};

}  // namespace raco::common_widgets