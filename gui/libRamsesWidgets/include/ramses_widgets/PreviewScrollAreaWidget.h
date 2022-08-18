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

#include <QAbstractScrollArea>
#include <QSize>
#include <optional>

namespace raco::ramses_widgets {

class PreviewScrollAreaWidget final : public QAbstractScrollArea {
	Q_OBJECT
public:
	enum class AutoSizing {
		OFF,
		VERTICAL_FIT,
		HORIZONTAL_FIT,
		BEST_FIT,
		ORIGINAL_FIT
	};
	explicit PreviewScrollAreaWidget(const QSize& sceneSize, QWidget* parent = nullptr);

	bool isGlobalPositionInsidePreview(const QPoint& p);
	std::optional<QPoint> globalPositionToPreviewPosition(const QPoint& p);

public Q_SLOTS:
	void setAutoSizing(const AutoSizing mode);
	void autoSizingVerticalFit();
	void autoSizingHorizontalFit();
	void autoSizingBestFit();
	void autoSizingOriginalFit();
	void autoSizingOff();
	void setViewport(const QSize& sceneSize);

private Q_SLOTS:
	void setScaleValue(const double value);

Q_SIGNALS:
	void viewportRectChanged(
		const QSize areaSize,
		const QPoint viewportPosition,
		const QPoint viewportOffset,
		const QSize viewportSize,
		const QSize virtualSize,
		const QSize targetSize) const;
	void scaleChanged(const double scale);
	void autoSizingChanged(const AutoSizing mode);

protected:
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	void updateViewport();
	void updateScrollbarSize(const QSize& widgetSiz) noexcept;
	QSize scaledSize() const noexcept;

	AutoSizing sizeMode_{AutoSizing::BEST_FIT};
	double scaleValue_{1.0};
	QPoint mousePivot_{0, 0};
	QPoint viewportPosition_{0, 0};
	QSize sceneSize_;
};

}  // namespace raco::ramses_widgets
