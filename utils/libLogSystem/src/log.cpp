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
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

namespace raco::log_system {

bool initialized{false};

std::shared_ptr<spdlog::sinks::dist_sink_mt> multiplexSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
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

void init(const char* logFile) {
	if (initialized) {
		LOG_WARNING(LOGGING, "log_system already initialized - call has no effect.");
		return;
	}
	sinks = {
		std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
		multiplexSink};
	if (logFile) {
		try {
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile));
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
	initialized = true;
	LOG_INFO_IF(LOGGING, logFile != nullptr, "log_system initialized logFile: {}", logFile);
	LOG_INFO_IF(LOGGING, logFile == nullptr, "log_system initialized");
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
