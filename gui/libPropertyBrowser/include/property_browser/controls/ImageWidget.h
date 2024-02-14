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

#include "style/Colors.h"

#include <QEvent>
#include <QWidget>

namespace raco::property_browser {

class ImageWidget : public QWidget {
	Q_OBJECT

public:
	explicit ImageWidget(QWidget* parent = nullptr);

	bool hasHeightForWidth() const override;
	int heightForWidth(int w) const override;
	bool eventFilter(QObject* obj, QEvent* event) override;

	void setHeightSourceWidget(QWidget* widget);
	void setPixmap(const QPixmap& pixmap);
	void setCheckerEnabled(bool enabled);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QPixmap createCheckerTilePixmap() const;
	int getMaximumHeight() const;
	int widthForHeight(int h) const;

	// ErrorBox palette.
	QBrush checkerLightBrush_ = style::Colors::brush(style::Colormap::grayButton);
	QBrush checkerDarkBrush_ = style::Colors::brush(style::Colormap::grayEdit);

	bool isCheckerEnabled_ = true;
	QPixmap imagePixmap_;

	QWidget* heightSourceWidget_ = nullptr;
	inline static constexpr double maxHeightShare_ = 0.75;
	inline static constexpr double checkerTileSizeFactor_ = 1.2;
};

}  // namespace raco::property_browser