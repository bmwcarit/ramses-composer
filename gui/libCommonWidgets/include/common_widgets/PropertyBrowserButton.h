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

#include <QPushButton>

namespace raco::common_widgets {

class PropertyBrowserButton : public QPushButton {
	Q_OBJECT

public:
	static inline auto MAXIMUM_WIDTH_PX = 20;

	explicit PropertyBrowserButton(const QString &text, QWidget *parent = nullptr);
	PropertyBrowserButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

private:
	void setUp();
};

}  // namespace raco::common_widgets
