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

#include <QStandardItemModel>
#include <QTableView>
#include <memory>

namespace raco::common_widgets {

class LogWidgetSink;

class LogWidget final : public QTableView {
public:
	explicit LogWidget(QWidget* parent = nullptr);
	~LogWidget();

private:
	QStandardItemModel model_;
	std::shared_ptr<LogWidgetSink> sink_;
};

}  // namespace raco::common_widgets
