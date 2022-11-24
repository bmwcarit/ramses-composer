/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "style/Colors.h"

namespace raco::style {

Colors::Colors() noexcept {
	colors_ = {
		{Colormap::grayBack, QColor(50, 50, 50)},
		{Colormap::grayEdit, QColor(20, 20, 20)},
		{Colormap::grayButton, QColor(90, 90, 90)},
		{Colormap::selected, QColor(170, 140, 0)},
		{Colormap::text, QColor(255, 255, 255)},
		{Colormap::grayEditDisabled, QColor(30, 30, 30)},
		{Colormap::textDisabled, QColor(200, 200, 200)},

		// additional colors for custom widgets and custom roles/states
		{Colormap::updatedInBackground, QColor(45, 100, 150)},
		{Colormap::warningColor, QColor(170, 100, 30)},
		{Colormap::errorColor, QColor(180, 20, 20)},
		{Colormap::errorColorDark, QColor(140, 0, 0)},
		{Colormap::errorColorLight, QColor(255, 120, 120)},
		{Colormap::dockTitleBackground, QColor(0, 0, 0)},
		{Colormap::externalReference, QColor(170, 250, 70)},
		{Colormap::externalReferenceDisabled, QColor(115, 195, 15)}};

	for (const auto& [key, value] : colors_) {
		brushes_[key] = QBrush(value);
	}
}

Colors& Colors::instance() {
	if (Colors::instance_ == nullptr) {
		Colors::instance_ = new Colors{};
	}
	return *Colors::instance_;
}

Colors* Colors::instance_{nullptr};

}  // namespace raco::style
