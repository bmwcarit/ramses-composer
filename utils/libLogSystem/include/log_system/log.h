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

#define SPDLOG_ACTIVE_LEVEL 0
#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

namespace raco::log_system {

using Sink = spdlog::sinks::sink;
using SinkPtr = spdlog::sink_ptr;
using LoggerPtr = std::shared_ptr<spdlog::logger>;

constexpr const char* DEFAULT{"DEFAULT"};
constexpr const char* COMMON{"COMMON"};
constexpr const char* CONTEXT{"CONTEXT"};
constexpr const char* USER_TYPES{"USER_TYPES"};
constexpr const char* PROPERTY_BROWSER{"PROPERTY_BROWSER"};
constexpr const char* OBJECT_TREE_VIEW{"OBJECT_TREE_VIEW"};
constexpr const char* STYLE {"STYLE"};
constexpr const char* LOGGING{"LOGGING"};
constexpr const char* DATA_CHANGE{"DATA_CHANGE"};
constexpr const char* PREVIEW_WIDGET {"PREVIEW_WIDGET"};
constexpr const char* RAMSES_BACKEND {"RAMSES_BACKEND"};
constexpr const char* RAMSES_ADAPTOR {"RAMSES_ADAPTOR"};
constexpr const char* DESERIALIZATION{"DESERIALIZATION"};
constexpr const char* PROJECT{"PROJECT"};
constexpr const char* MESH_LOADER{"MESH_LOADER"};

void init(const char* fileName = nullptr);
void deinit();
void registerSink(const SinkPtr sink);
void unregisterSink(const SinkPtr sink);
LoggerPtr get(const char* category);

}  // namespace raco::log_system

/**
 * Usage: 
 * int importantValue {43};
 * LOG_DEBUG(COMMON, "The important value is {}.", importantValue);
 * LOG(DEBUG, COMMON, "The important value is {}.", importantValue);
 * LOG_DEBUG_IF(COMMON, importantValue > 1, "The important value is {}.", importantValue);
 * 
 * The predefined log categories can be found in log.h .
 * For further information see README.md .
 */
#if !defined(NDEBUG)
#define LOG_TRACE(category, message, ...) SPDLOG_LOGGER_TRACE(raco::log_system::get(category), message, ##__VA_ARGS__)
#define LOG_DEBUG(category, message, ...) SPDLOG_LOGGER_DEBUG(raco::log_system::get(category), message, ##__VA_ARGS__)
#else
#define LOG_TRACE(category, message, ...)
#define LOG_DEBUG(category, message, ...)
#endif
#define LOG_INFO(category, message, ...) SPDLOG_LOGGER_INFO(raco::log_system::get(category), message, ##__VA_ARGS__)
#define LOG_WARNING(category, message, ...) SPDLOG_LOGGER_WARN(raco::log_system::get(category), message, ##__VA_ARGS__)
#define LOG_ERROR(category, message, ...) SPDLOG_LOGGER_ERROR(raco::log_system::get(category), message, ##__VA_ARGS__)
#define LOG_CRITICAL(category, message, ...) SPDLOG_LOGGER_CRITICAL(raco::log_system::get(category), message, ##__VA_ARGS__)
#define LOG(SEVERITY, category, message, ...) LOG_##SEVERITY(category, message, ##__VA_ARGS__)

#if !defined(NDEBUG)
#define LOG_TRACE_IF(category, condition, message, ...) \
	if (condition) LOG_TRACE(category, message, ##__VA_ARGS__)
#define LOG_DEBUG_IF(category, condition, message, ...) \
	if (condition) LOG_DEBUG(category, message, ##__VA_ARGS__)
#else
#define LOG_TRACE_IF(category, condition, message, ...)
#define LOG_DEBUG_IF(category, condition, message, ...)
#endif
#define LOG_INFO_IF(category, condition, message, ...) \
	if (condition) LOG_INFO(category, message, ##__VA_ARGS__)
#define LOG_WARNING_IF(category, condition, message, ...) \
	if (condition) LOG_WARNING(category, message, ##__VA_ARGS__)
#define LOG_ERROR_IF(category, condition, message, ...) \
	if (condition) LOG_ERROR(category, message, ##__VA_ARGS__)
#define LOG_CRITICAL_IF(category, condition, message, ...) \
	if (condition) LOG_CRITICAL(category, message, ##__VA_ARGS__)

#define LOG_IF(SEVERITY, category, condition, ...) LOG_##SEVERITY_IF(category, condition, message, ##__VA_ARGS__)
