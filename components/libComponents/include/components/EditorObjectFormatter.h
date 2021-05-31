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

#include <core/EditorObject.h>
#include <spdlog/fmt/fmt.h>
#include <vector>

template <>
struct fmt::formatter<raco::core::SEditorObject> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::core::SEditorObject& c, FormatContext& ctx) {
		return formatter<string_view>::format(c->objectName(), ctx);
	}
};

template <>
struct fmt::formatter<std::vector<raco::core::SEditorObject>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::vector<raco::core::SEditorObject>& c, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("{}", fmt::join(c, ", ")), ctx);
	}
};

