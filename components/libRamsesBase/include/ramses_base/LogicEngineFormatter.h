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

#include <ramses-logic/LogicEngine.h>
#include <spdlog/fmt/fmt.h>

struct LogicEngineErrors {
	const rlogic::LogicEngine& engine;
};

template <>
struct fmt::formatter<rlogic::ErrorData> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const rlogic::ErrorData& error, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "{}", error.message);
	}
};

template <>
struct fmt::formatter<LogicEngineErrors> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const LogicEngineErrors& errors, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "{}", fmt::join(errors.engine.getErrors(), "\n"));
	}
};
