/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/log_model/LogViewSink.h"

#include "common_widgets/log_model/LogViewModel.h"

#include <QMetaObject>
#include <iomanip>
#include <sstream>
#include <string>

namespace raco::common_widgets {

LogViewSink::LogViewSink(LogViewModel* model) : spdlog::sinks::base_sink<std::mutex>(), model_(model) {
}

void LogViewSink::sink_it_(const spdlog::details::log_msg& msg) {
	auto time = std::chrono::system_clock::to_time_t(msg.time);
	struct tm localTime;
#if (defined(Q_OS_WIN))
	localtime_s(&localTime, &time);
#else
	localtime_r(&time, &localTime);
#endif
	std::stringstream timeStringStream;
	timeStringStream << std::put_time(&localTime, "%F %T");

	spdlog::memory_buf_t messageRaw;
	base_sink<std::mutex>::formatter_->format(msg, messageRaw);

	LogViewModel::LogEntry entry = {
		timeStringStream.str().c_str(),
		msg.level,
		QString::fromLocal8Bit(msg.logger_name.data(), (int)msg.logger_name.size()),
		QString::fromLocal8Bit(msg.payload.data(), (int)msg.payload.size()),
		QString::fromLocal8Bit(messageRaw.data(), messageRaw.size())
	};

	// Always call addEntry on the main thread, in case Ramses Composer becomes multi-threaded in the future.
	QMetaObject::invokeMethod(model_, "addEntry", Qt::ConnectionType::QueuedConnection, Q_ARG(LogViewModel::LogEntry, entry));
}

void LogViewSink::flush_() {}

}  // namespace raco::common_widgets
