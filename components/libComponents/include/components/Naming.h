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

#include <algorithm>
#include <string>
#include <string_view>

namespace raco::components {

enum class NamingStyle {
	Camel,
	Pascal,
	Snake,
	Kebab
};

struct Naming {
	constexpr static NamingStyle defaultStyle() noexcept { return NamingStyle::Pascal; }
	static std::string format(const std::string& s, NamingStyle style = defaultStyle()) noexcept {
		std::string result{s};
		switch (style) {
			case NamingStyle::Camel:
				result[0] = tolower(result[0]);
				break;
			case NamingStyle::Pascal:
				result[0] = toupper(result[0]);
				break;
			case NamingStyle::Snake:
				result.clear();
				std::for_each(s.begin(), s.end(), [&result](const char c) {
					if (isupper(c) && !result.empty()) {
						result.append(1, '_');
					}
					result.append(1, tolower(c));
				});
				break;
			case NamingStyle::Kebab:
				result.clear();
				std::for_each(s.begin(), s.end(), [&result](const char c) {
					if (isupper(c) && !result.empty()) {
						result.append(1, '-');
					}
					result.append(1, tolower(c));
				});
				break;
		}
		return result;
	}
};

}  // namespace raco::components
