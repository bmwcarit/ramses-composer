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

#include <QBrush>
#include <QColor>
#include <map>

namespace raco::style {

enum class Colormap {
	// standard colors for palette
	grayBack,
	grayEdit,
	grayButton,
	selected,
	text,
	grayEditDisabled,
	textDisabled,

	// additional colors for custom widgets and custom roles/states
	updatedInBackground,
	errorColor,
	warningColor,
	errorColorDark,
	errorColorLight,
	dockTitleBackground,
	externalReference,
	externalReferenceDisabled,
};

class Colors {
public:
	static QColor& color(Colormap color) {
		return Colors::instance().getColor(color);
	}
	static QBrush& brush(Colormap color) {
		return Colors::instance().getBrush(color);
	}

private:
	Colors() noexcept;
	QColor& getColor(Colormap color) {
		return colors_[color];
	}
	QBrush& getBrush(Colormap color) {
		return brushes_[color];
	}
	static Colors& instance();
	static Colors* instance_;
	std::map<Colormap, QColor> colors_;
	std::map<Colormap, QBrush> brushes_;
};

}  // namespace raco::style
