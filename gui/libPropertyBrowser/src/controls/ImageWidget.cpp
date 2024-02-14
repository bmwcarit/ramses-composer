/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/controls/ImageWidget.h"

#include <QPainter>
#include <QScreen>
#include <QApplication>

#include <cmath>

namespace raco::property_browser {

ImageWidget::ImageWidget(QWidget* parent)
	: QWidget(parent) {
}

void ImageWidget::setHeightSourceWidget(QWidget* widget) {
	heightSourceWidget_ = widget;
	heightSourceWidget_->installEventFilter(this);
}

bool ImageWidget::eventFilter(QObject* obj, QEvent* event) {
	if (event->type() == QEvent::Resize) {
		updateGeometry();
	}
	return QObject::eventFilter(obj, event);
}

int ImageWidget::getMaximumHeight() const {
	return heightSourceWidget_ ? static_cast<int>(heightSourceWidget_->height() * maxHeightShare_) : std::numeric_limits<int>::max();
}

void ImageWidget::setPixmap(const QPixmap& pixmap) {
	imagePixmap_ = pixmap;

	// Size might have changed
	updateGeometry();

	// Schedule repaint
	update();
}

bool ImageWidget::hasHeightForWidth() const {
	return true;
}

int ImageWidget::heightForWidth(int w) const {
	if (!imagePixmap_.width()) {
		return 0;
	}
	return std::min(static_cast<int>(std::floor(w * static_cast<double>(imagePixmap_.height()) / imagePixmap_.width())), getMaximumHeight());
}

int ImageWidget::widthForHeight(int h) const {
	if (!imagePixmap_.height()) {
		return 0;
	}
	return static_cast<int>(std::round(h * static_cast<double>(imagePixmap_.width()) / imagePixmap_.height()));
}

QPixmap ImageWidget::createCheckerTilePixmap() const {
	const int checkerTileSize = static_cast<int>(checkerTileSizeFactor_ * QApplication::fontMetrics().height());

	QPixmap tilePixmap{checkerTileSize, checkerTileSize};
	QPainter painter{&tilePixmap};
	painter.fillRect(0, 0, checkerTileSize, checkerTileSize, checkerLightBrush_);
	const auto squareSize = checkerTileSize / 2;
	painter.fillRect(0, 0, squareSize, squareSize, checkerDarkBrush_);
	painter.fillRect(squareSize, squareSize, squareSize, squareSize, checkerDarkBrush_);
	painter.end();
	return tilePixmap;
}

void ImageWidget::setCheckerEnabled(bool enabled) {
	isCheckerEnabled_ = enabled;
	update();
}

void ImageWidget::paintEvent(QPaintEvent* paintEvent) {
	QPainter painter{this};

	if (imagePixmap_.isNull() || !imagePixmap_.width() || !imagePixmap_.height()) {
		return;
	}

	const auto verticalScale = static_cast<double>(height()) / imagePixmap_.height();
	const auto sx{static_cast<int>(imagePixmap_.width() * verticalScale)};
	const QRect fillRect{(width() - sx) / 2, 0, sx, static_cast<int>(imagePixmap_.height() * verticalScale)};

	if (isCheckerEnabled_) {
		painter.drawTiledPixmap(fillRect, createCheckerTilePixmap());
	} else {
		painter.fillRect(fillRect, Qt::black);
	}

	painter.drawPixmap(fillRect, imagePixmap_, imagePixmap_.rect());
}

}  // namespace raco::property_browser