/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/LogWidget.h"

#include <QHeaderView>
#include <QList>
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <iomanip>
#include <log_system/log.h>
#include <memory>
#include <spdlog/sinks/base_sink.h>
#include <sstream>
#include <string>

namespace raco::common_widgets {

class LogWidgetSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
	explicit LogWidgetSink(QStandardItemModel* model) : model_{model} {}
	void sink_it_(const spdlog::details::log_msg& msg) override {
		auto time = std::chrono::system_clock::to_time_t(msg.time);
		std::stringstream timeSS{};
		struct tm buf;
#if (defined(Q_OS_WIN))
		localtime_s(&buf, &time);
#else
		localtime_r(&time, &buf);
#endif
		timeSS << std::put_time(&buf, "%F %T");
		model_->appendRow(QList{
			new QStandardItem{timeSS.str().c_str()},
			new QStandardItem{msg.logger_name.data()},
			new QStandardItem{msg.payload.data()}});
	}
	void flush_() override {}

private:
	QStandardItemModel* model_;
};

LogWidget::LogWidget(QWidget* parent) : QTableView{parent}, model_{}, sink_{std::make_shared<LogWidgetSink>(&model_)} {
	log_system::registerSink(sink_);
	verticalHeader()->hide();
	setGridStyle(Qt::PenStyle::NoPen);

	model_.setHeaderData(0, Qt::Horizontal, "timestamp");
	model_.setHeaderData(1, Qt::Horizontal, "category");
	model_.setHeaderData(2, Qt::Horizontal, "message");

	setModel(&model_);
}

LogWidget::~LogWidget() {
	log_system::unregisterSink(sink_);
}

}  // namespace raco::common_widgets
