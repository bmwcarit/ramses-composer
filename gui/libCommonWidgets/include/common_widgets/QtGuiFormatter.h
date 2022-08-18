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

#include <QWidget>
#include <spdlog/fmt/fmt.h>

struct QWidgetInfo {
	QWidget* widget { nullptr };
};

template <>
struct fmt::formatter<QWidgetInfo> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QWidgetInfo& e, FormatContext& ctx) {
		if (e.widget) {
			return fmt::format_to(
				ctx.out(),
				R"({} {{ objectName: "{}" }})",
				e.widget->metaObject()->className(),
				e.widget->objectName().toStdString());
		} else {
			return fmt::format_to(ctx.out(), "nullptr");
		}
	}
};
