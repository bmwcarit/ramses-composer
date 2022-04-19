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

#if defined(_WIN32)
#include <filesystem>
#include <processthreadsapi.h>
#endif

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

void init(spdlog::filename_t logFileName) {
	if (initialized) {
		LOG_WARNING(LOGGING, "log_system already initialized - call has no effect.");
		return;
	}

	multiplexSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	sinks = {consoleSink, multiplexSink};

	std::string logFileSinkError = "";
	if (!logFileName.empty()) {
#if defined(_WIN32)
		// Under Windows, the renaming of the log file that rotating_file_sink_mt perfoms will fail, when another RaCo instance is already opened.
		// Thus if a log file already exists, we will check whether we can rename it and if not, append the process id to the file name to have a non colliding file name.
		// Technically, there is a tiny race condition in here, when two new instances launch exactly at the same time and execttue this code block at the same time.
		// This is very unlikely to happen. If it happens, it will cause the log file of one instance to be lost, but both of them will keep running.
		// Under Linux, these precautions are not necessary, since renaming files that are being written to works transparently.
		if (std::filesystem::exists(logFileName)) {
			try {
				std::filesystem::rename(logFileName, logFileName);
			} catch(const std::filesystem::filesystem_error& e) {
				logFileName = logFileName.substr(0, logFileName.size() - 4) + L"-" + std::to_wstring(GetCurrentProcessId()) + L".log";
			}
		}
#endif

		try {
			sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, MAX_LOG_FILE_SIZE_BYTES, MAX_LOG_FILE_AMOUNT, true));
		} catch (const spdlog::spdlog_ex& ex) {
			logFileSinkError = ex.what();
		}
	}

	// Resize stdout buffer
	const auto stdoutBufferSize = 50000;
	setvbuf(stdout, nullptr, _IOFBF, stdoutBufferSize);
	spdlog::register_logger(makeLogger(COMMON));
	spdlog::register_logger(makeLogger(PROPERTY_BROWSER));
	spdlog::register_logger(makeLogger(OBJECT_TREE_VIEW));
	spdlog::register_logger(makeLogger(USER_TYPES));
	spdlog::register_logger(makeLogger(CONTEXT));
	spdlog::register_logger(makeLogger(LOGGING));
	spdlog::register_logger(makeLogger(PREVIEW_WIDGET));
	spdlog::register_logger(makeLogger(RAMSES_BACKEND));
	spdlog::register_logger(makeLogger(RAMSES_ADAPTOR));
	spdlog::register_logger(makeLogger(DESERIALIZATION));
	spdlog::register_logger(makeLogger(PROJECT));
	spdlog::register_logger(makeLogger(DEFAULT));
	spdlog::register_logger(makeLogger(MESH_LOADER));
	spdlog::register_logger(makeLogger(RAMSES_LOGIC));
	spdlog::set_default_logger(spdlog::get(DEFAULT));
	spdlog::set_pattern("%^[%L] [%D %T:%f] [%n] [%s:%#] [%!] %v");

#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(logFileName));
#else
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", logFileName);
#endif

	if (!logFileSinkError.empty()) {
		LOG_ERROR(LOGGING, "Log file initialization failed: {}", logFileSinkError);
	}

	initialized = true;
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
