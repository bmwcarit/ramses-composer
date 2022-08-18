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

#include <QEvent>
#include <QMetaEnum>
#include <spdlog/fmt/fmt.h>

template <>
struct fmt::formatter<QEvent::Type> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QEvent::Type& e, FormatContext& ctx) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QEvent::Type>();
		return formatter<string_view>::format(metaEnum.valueToKey(e), ctx);
	}
};

template <>
struct fmt::formatter<QEvent> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QEvent& e, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QEvent( type: {} )", e.type()), ctx);
	}
};

template <>
struct fmt::formatter<QSize> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QSize& e, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QSize( {}, {} )", e.width(), e.height() ), ctx);
	}
};

template <>
struct fmt::formatter<QString> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QString& e, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QString( {} )", e.toStdString()), ctx);
	}
};

template <>
struct fmt::formatter<QRect> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QRect& e, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QRect( {}, {}, w {}, h {} )", e.x(), e.y(), e.width(), e.height() ), ctx);
	}
};

template <>
struct fmt::formatter<QMargins> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QMargins& e, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QMargins( left {}, top {}, bottom {}, right {} )", e.left(), e.top(), e.bottom(), e.right() ), ctx);
	}
};


