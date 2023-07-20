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

#include <core/EditorObject.h>
#include <spdlog/fmt/fmt.h>
#include <vector>

template <typename TUserType>
struct fmt::formatter<std::shared_ptr<TUserType>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::shared_ptr<TUserType>& c, FormatContext& ctx) {
		static_assert(std::is_base_of<raco::core::EditorObject, TUserType>::value);
		if (c) {
			return formatter<string_view>::format(c->objectName(), ctx);
		} else {
			return formatter<string_view>::format("<None>", ctx);
		}
	}
};

template <typename TUserType>
struct fmt::formatter<std::vector<std::shared_ptr<TUserType>>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::vector<std::shared_ptr<TUserType>>& c, FormatContext& ctx) {
		static_assert(std::is_base_of<raco::core::EditorObject, TUserType>::value);
		return formatter<string_view>::format(fmt::format("{}", fmt::join(c, ", ")), ctx);
	}
};

template <typename TUserType>
struct fmt::formatter<std::set<std::shared_ptr<TUserType>>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::set<std::shared_ptr<TUserType>>& c, FormatContext& ctx) {
		static_assert(std::is_base_of<raco::core::EditorObject, TUserType>::value);
		return formatter<string_view>::format(fmt::format("{}", fmt::join(c, ", ")), ctx);
	}
};
