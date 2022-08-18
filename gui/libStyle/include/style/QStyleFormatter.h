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

#include <QString>
#include <QWidget>
#include <QStyle>
#include <QRect>
#include <QMetaEnum>
#include <QStyleOption>
#include <spdlog/fmt/fmt.h>
#include <vector>

template <>
struct fmt::formatter<QStyle::ComplexControl> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStyle::ComplexControl& c, FormatContext& ctx) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QStyle::ComplexControl>();
		return formatter<string_view>::format(metaEnum.valueToKey(c), ctx);
	}
};

template <>
struct fmt::formatter<QStyle::ControlElement> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStyle::ControlElement& c, FormatContext& ctx) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QStyle::ControlElement>();
		return formatter<string_view>::format(metaEnum.valueToKey(c), ctx);
	}
};

template <>
struct fmt::formatter<QStyle::PrimitiveElement> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStyle::PrimitiveElement& c, FormatContext& ctx) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QStyle::PrimitiveElement>();
		return formatter<string_view>::format(metaEnum.valueToKey(c), ctx);
	}
};

template <>
struct fmt::formatter<QRect> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QRect& rect, FormatContext& ctx) {
        QMetaEnum metaEnum = QMetaEnum::fromType<QStyle::ComplexControl>();
		return formatter<string_view>::format(fmt::format("QRect( x: {}, y: {}, w: {}, h: {})", rect.topLeft().x(), rect.topLeft().y(), rect.size().width(), rect.size().height()), ctx);
	}
};

template <>
struct fmt::formatter<QString> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QString& s, FormatContext& ctx) {
		return formatter<string_view>::format(s.toStdString().c_str(), ctx);
	}
};

template <>
struct fmt::formatter<QWidget> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QWidget& widget, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("{}( objectName: \"{}\" )",  widget.metaObject()->className(), widget.objectName()), ctx);
	}
};

template <>
struct fmt::formatter<QStyleOption> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStyleOption& styleOption, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QStyleOption( type: {}, rect: {} )", styleOption.type, styleOption.rect), ctx);
	}
};

template <>
struct fmt::formatter<QStyleOptionComplex> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStyleOptionComplex& styleOption, FormatContext& ctx) {
		return formatter<string_view>::format(fmt::format("QStyleOptionComplex( type: {}, rect: {} )", styleOption.type, styleOption.rect), ctx);
	}
};
