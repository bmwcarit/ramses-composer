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

#include "components/DataChangeDispatcher.h"

#include <QWidget>

class QSortFilterProxyModel;
class QLabel;
class QCheckBox;
class QStandardItem;
class QStandardItemModel;
class QTableView;

namespace raco::core {
class CommandInterface;
}

namespace raco::common_widgets {

class LogViewModel;

class ErrorView : public QWidget {
	Q_OBJECT

public:
	enum ErrorViewColumns {
		LEVEL,
		OBJECT,
		PROPERTY,
		MESSAGE,
		COLUMN_COUNT
	};

	ErrorView(raco::core::CommandInterface *commandInterface, raco::components::SDataChangeDispatcher dispatcher, bool embeddedInExportView, LogViewModel *logViewModel, QWidget *parent = nullptr);

Q_SIGNALS:
	void objectSelectionRequested(const QString &objectID);

protected:
	static inline const auto ROW_HEIGHT = 22;

	std::vector<std::string> rowToVector(const QModelIndex &rowIndex) const;
	void filterRows();
	void regenerateTable();

	raco::core::CommandInterface *commandInterface_;
	components::Subscription errorChangeSubscription_;
	components::Subscription objNameChangeSubscription_;
	components::Subscription objChildrenChangeSubscription_;
	QTableView *tableView_;
	QCheckBox *showWarningsCheckBox_ = nullptr;
	QCheckBox *showErrorsCheckBox_ = nullptr;
	QLabel *errorAmountLabel_ = nullptr;
	QStandardItemModel *tableModel_;
	QSortFilterProxyModel *proxyModel_;
	std::vector<std::string> indexToObjID_;
	std::set<std::string> objIDs_;
	LogViewModel *logViewModel_;
	bool showFilterLayout_ = false;
	bool showExtRefErrors_ = false;

protected Q_SLOTS:
	void createCustomContextMenu(const QPoint &p);
	void updateErrorAmountLabel();
};

}  // namespace raco::common_widgets
