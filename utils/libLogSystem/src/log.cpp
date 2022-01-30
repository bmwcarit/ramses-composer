/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "log_system/log.h"

#include <iostream>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>
#include <locale>
#include <codecvt>

namespace raco::log_system {

bool initialized{false};

std::shared_ptr<spdlog::sinks::dist_sink_mt> multiplexSink;
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
std::vector<spdlog::sink_ptr> sinks{};

std::shared_ptr<spdlog::logger> makeLogger(const char* name, spdlog::level::level_enum level = spdlog::level::level_enum::trace) {
	auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
	logger->set_level(level);
	logger->flush_on(level);
	return logger;
}

void registerSink(const spdlog::sink_ptr sink) {
	multiplexSink->add_sink(sink);
}

void unregisterSink(const spdlog::sink_ptr sink) {
	multiplexSink->remove_sink(sink);
}

void setConsoleLogLevel(spdlog::level::level_enum level) {
	consoleSink->set_level(level);
}

void init(const spdlog::filename_t& logFileName) {
	if (initialized) {
		LOG_WARNING(LOGGING, "log_system already initialized - call has no effect.");
		return;
	}

	multiplexSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	sinks = {consoleSink, multiplexSink};

	if (!logFileName.empty()) {
		try {
			sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, MAX_LOG_FILE_SIZE_BYTES, MAX_LOG_FILE_AMOUNT, false));
		} catch (const spdlog::spdlog_ex& ex) {
			LOG_ERROR(LOGGING, "Log file initialization failed: {}\n", ex.what());
		}
	}

	// Resize stdout buffer
	const auto stdoutBufferSize = 50000;
	setvbuf(stdout, nullptr, _IOFBF, stdoutBufferSize);
	spdlog::register_logger(makeLogger(COMMON));
	spdlog::register_logger(makeLogger(PROPERTY_BROWSER, spdlog::level::debug));
	spdlog::register_logger(makeLogger(OBJECT_TREE_VIEW, spdlog::level::debug));
	spdlog::register_logger(makeLogger(USER_TYPES, spdlog::level::debug));
	spdlog::register_logger(makeLogger(CONTEXT, spdlog::level::debug));
	// Try catch block above may already have created this, so check first:
	if (!spdlog::get(LOGGING)) {
		spdlog::register_logger(makeLogger(LOGGING));
	}
	spdlog::register_logger(makeLogger(PREVIEW_WIDGET, spdlog::level::debug));
	spdlog::register_logger(makeLogger(RAMSES_BACKEND));
	spdlog::register_logger(makeLogger(RAMSES_ADAPTOR));
	spdlog::register_logger(makeLogger(DESERIALIZATION));
	spdlog::register_logger(makeLogger(PROJECT));
	spdlog::register_logger(makeLogger(DATA_CHANGE, spdlog::level::debug));
	spdlog::register_logger(makeLogger(STYLE, spdlog::level::off));
	spdlog::register_logger(makeLogger(DEFAULT));
	spdlog::register_logger(makeLogger(MESH_LOADER));
	spdlog::set_default_logger(spdlog::get(DEFAULT));
    spdlog::set_pattern("%^[%L] [%D %T:%f] [%n] [%s:%#] [%!] %v");

#if _WIN64
	SetConsoleOutputCP(CP_UTF8);
#endif

	initialized = true;

#if defined(_WIN32)
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(logFileName));
#else
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", logFileName);
#endif
}

void deinit() {
	spdlog::drop_all();
}

LoggerPtr get(const char* context) {
	if (auto logger = spdlog::get(context)) {
		return logger;
	}
	spdlog::register_logger(makeLogger(context));
	LOG_WARNING(LOGGING, "Dynamic logging context created: {}", context);
	return spdlog::get(context);
}

}  // namespace raco::log_system
