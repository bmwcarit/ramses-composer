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

#include "common_widgets/QtGuiFormatter.h"
#include "log_system/log.h"
#include "components/QtFormatter.h"
#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QWidget>
#include <sstream>
#include <string>

namespace raco::debug {

inline std::string tabs(size_t amount) {
	std::stringstream ss{};
	for (int i{0}; i < amount; i++) {
		ss << "    ";
	}
	return ss.str();
}

inline void dumpLayoutInfo(const QWidget* widget, size_t depth = 0) {
	if (widget->inherits("QLabel")) {
		LOG_DEBUG("DUMP", "{}{}: \"{}\" geometry: {} sizeHint: {}", tabs(depth).c_str(), "QLabel", (qobject_cast<const QLabel*>(widget))->text().toStdString(), widget->geometry(), widget->sizeHint());
	} else if (widget->inherits("QPushButton")) {
		LOG_DEBUG("DUMP", "{}{}: \"{}\" geometry: {} sizeHint: {}", tabs(depth).c_str(), "QPushButton", (qobject_cast<const QPushButton*>(widget))->text().toStdString(), widget->geometry(), widget->sizeHint());
	} else {
		LOG_DEBUG("DUMP", "{}{}:{} geometry: {} sizeHint: {}", tabs(depth).c_str(), widget->metaObject()->className(), widget->objectName(), widget->geometry(), widget->sizeHint());
	}
	if (widget->layout()) {
		LOG_DEBUG("DUMP", "{}layout -> contentsMargins: {}", tabs(depth).c_str(), widget->layout()->contentsMargins());
	}
	for (const auto& child : widget->findChildren<QWidget*>(QString{}, Qt::FindDirectChildrenOnly)) {
		dumpLayoutInfo(child, depth + 1);
	}
}

};	// namespace raco::debug
