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

#include "common_widgets/log_model/LogViewSink.h"
#include "common_widgets/log_model/LogViewSortFilterProxyModel.h"

#include <chrono>
#include <memory>
#include <QKeyEvent>
#include <QLabel>
#include <QTableView>
#include <QWidget>

namespace raco::common_widgets {

class LogView : public QWidget {
	Q_OBJECT

public:
	explicit LogView(LogViewModel* model_, QWidget* parent = nullptr);

public Q_SLOTS:
	void updateWarningErrorLabel();
	void customMenuRequested(QPoint pos);

protected:
	QTableView* tableView_;

	bool event(QEvent* event) override;
	void copySelectedRows();
	void keyPressEvent(QKeyEvent* event) override;


private:
	LogViewModel* model_;
	LogViewSortFilterProxyModel* proxyModel_;
	QLabel* warningErrorLabel_;

};

}  // namespace raco::common_widgets