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

#include <functional>
#include <list>
#include <string>
#include <vector>

#include <QImage>

namespace raco::property_browser {

struct ColorChannels {
	typedef std::list<std::function<int(QRgb)>> ChannelOperations;

	struct ChannelExtractOperation {
		std::string displayName_;
		std::function<QRgb(QRgb)> extractFunction_;
		std::function<bool(const QImage&)> channelExistsPredicate_;
	};

	// Ordered named operations leaving only specific channel in RGBA value.
	inline const static std::vector<ChannelExtractOperation> channels_ = {
		{"Gray", [](QRgb c) { return qRgba(qRed(c), qGreen(c), qBlue(c), 0); }, [](const QImage& q) { return q.isGrayscale(); }},
		{"Red", [](QRgb c) { return qRgba(qRed(c), 0, 0, 0); }, [](const QImage& q) { return !q.isGrayscale(); }},
		{"Green", [](QRgb c) { return qRgba(0, qGreen(c), 0, 0); }, [](const QImage& q) { return !q.isGrayscale(); }},
		{"Blue", [](QRgb c) { return qRgba(0, 0, qBlue(c), 0); }, [](const QImage& q) { return !q.isGrayscale(); }},
		{"Alpha", [](QRgb c) { return qRgba(0, 0, 0, qAlpha(c)); }, [](const QImage& q) { return q.hasAlphaChannel(); }},
	};
};

}  // namespace raco::property_browser