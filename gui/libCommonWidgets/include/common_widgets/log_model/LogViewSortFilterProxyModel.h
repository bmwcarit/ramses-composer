/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <common_widgets/log_model/LogViewModel.h>

#include <memory>
#include <QSortFilterProxyModel>

namespace raco::common_widgets {

class LogViewSortFilterProxyModel : public QSortFilterProxyModel {
public:
	LogViewSortFilterProxyModel(LogViewModel* model, QObject* parent);
	void setFilterMinLogLevel(spdlog::level::level_enum filterMinLogLevel);
	void setFilterCategory(QString filterCategory);
	void setFilterMessage(QString filterMessage);

	int warningCount() const;
	int errorCount() const;

	std::string getStringFromSelection(QItemSelection selection) const;

protected:
	LogViewModel* model_;
	spdlog::level::level_enum filterMinLogLevel_;
	QString filterCategory_;
	QString filterMessage_;

	bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
	bool filterAcceptsEntry(const LogViewModel::LogEntry& entry) const;
};

}  // namespace raco::common_widgets