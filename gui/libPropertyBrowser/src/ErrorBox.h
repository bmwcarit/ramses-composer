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

#include "core/ErrorItem.h"
#include "style/Colors.h"
#include <QTextEdit>

namespace raco::property_browser {

class ErrorBox final : public QTextEdit {
public:
	explicit ErrorBox(const QString& content, core::ErrorLevel errorLevel, QWidget* parent);

	void updateContent(const QString& content);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent* event) override;
};

}  // namespace raco::property_browser
