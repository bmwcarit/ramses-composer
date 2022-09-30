/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "log_system/log.h"

#include <iostream>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>
#include <locale>
#include <codecvt>

#if defined(_WIN32)
#include <filesystem>
#include <processthreadsapi.h>
#endif

#include "utils/u8path.h"

#include <chrono>
#include <filesystem>
#include <regex>

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

void setup_loggers() {
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
	spdlog::register_logger(makeLogger(PYTHON));
	spdlog::register_logger(makeLogger(DEFAULT));
	spdlog::register_logger(makeLogger(MESH_LOADER));
	spdlog::register_logger(makeLogger(RAMSES));
	spdlog::register_logger(makeLogger(RAMSES_LOGIC));
	spdlog::register_logger(makeLogger(TRACE_PLAYER));
	spdlog::set_default_logger(spdlog::get(DEFAULT));
	spdlog::set_pattern("%^[%L] [%D %T:%f] [%n] [%s:%#] [%!] %v");
}


void init(const raco::utils::u8path& logFilePath) {
	if (initialized) {
		LOG_WARNING(LOGGING, "log_system already initialized - call has no effect.");
		return;
	}

	multiplexSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	sinks = {consoleSink, multiplexSink};

	std::string logFileSinkError;

	if (!logFilePath.empty()) {
		try {
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.internalPath().native()));
		} catch (const spdlog::spdlog_ex& ex) {
			logFileSinkError = ex.what();
		}
	}

	setup_loggers();

#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
#endif
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", logFilePath.string());

	if (!logFileSinkError.empty()) {
		LOG_ERROR(LOGGING, "Log file initialization failed: {}", logFileSinkError);
	}

	initialized = true;
}


void init(const raco::utils::u8path& logFileDirectory, const std::string& logFileBaseName, int64_t pid) {
	if (initialized) {
		LOG_WARNING(LOGGING, "log_system already initialized - call has no effect.");
		return;
	}

	multiplexSink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	sinks = {consoleSink, multiplexSink};

	std::string logFileSinkError;

	std::filesystem::path logFilePath;

	if (!logFileBaseName.empty()) {
		// Append procecss id to the log file name to avoid name collisions between multiple processes.
		std::string logFileFullName = logFileBaseName + "-" + std::to_string(pid) + LOG_FILE_EXTENSION;
		logFilePath = (logFileDirectory / logFileFullName).internalPath().native();
		try {
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath));
		} catch (const spdlog::spdlog_ex& ex) {
			logFileSinkError = ex.what();
		}

		// Remove oldest logfiles:

		// Step 1: get directory listing of all files matching logfile naming pattern
		std::vector<std::filesystem::directory_entry> logFiles;
		std::regex regex(logFileBaseName + "-[0-9]+" + LOG_FILE_EXTENSION);
		for (auto entry : std::filesystem::directory_iterator(logFileDirectory)) {
			auto fname = entry.path().filename().string();
			if (std::regex_match(fname, regex)) {
				logFiles.emplace_back(entry);
			}
		}

		// Step 2: Sort into oldest first order:
		std::sort(logFiles.begin(), logFiles.end(), [](auto& left, auto& right) {
			return left.last_write_time() < right.last_write_time();
		});

		// Step 3: removed oldest files keeping at most MAX_LOG_FILES files.
		if (logFiles.size() > MAX_LOG_FILES) {
			for (size_t i = 0; i < logFiles.size() - MAX_LOG_FILES; i++) {
				auto& entry = logFiles[i];
				std::filesystem::remove(entry);
			}
		}
	}

	setup_loggers();

#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
#endif
	LOG_INFO(LOGGING, "log_system initialized logFileName: {}", logFilePath.string());

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
