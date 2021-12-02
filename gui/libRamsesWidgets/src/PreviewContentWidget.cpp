/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/PreviewContentWidget.h"

#include "log_system/log.h"
#include "ramses_widgets/BuildOptions.h"
#include "ramses_widgets/RamsesPreviewWindow.h"
#include "components/QtFormatter.h"
#include <QPaintEvent>
#include <QPlatformSurfaceEvent>

namespace raco::ramses_widgets {

PreviewContentWidget::PreviewContentWidget(RendererBackend& rendererBackend, QWidget* parent)
	: QWidget(parent), ramsesPreview_{std::make_unique<RamsesPreviewWindow>(reinterpret_cast<void*>(winId()), rendererBackend)} {
	// In order to prevent Qt from interfering with Ramses rendering into the window we need to set these flags
	// and also override the QWidget::paintEngine method (returning a nullptr).
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setMouseTracking(true);
}

void PreviewContentWidget::setSceneId(ramses::sceneId_t id) {
	if (ramsesPreview_) {
		ramsesPreview_->state().sceneId = id;
		update();
	}
}

void PreviewContentWidget::setBackgroundColor(data_storage::Vec4f backgroundColor) {
	if (ramsesPreview_) {
		ramsesPreview_->state().backgroundColor = QColor::fromRgbF(backgroundColor.x.asDouble(), backgroundColor.y.asDouble(), backgroundColor.z.asDouble(), backgroundColor.w.asDouble());
		update();
	}
}

ramses::sceneId_t PreviewContentWidget::getSceneId() {
	if (ramsesPreview_) {
		return ramsesPreview_->state().sceneId;
	} else {
		return ramses::sceneId_t::Invalid();
	}
}

bool PreviewContentWidget::event(QEvent* event) {
	LOG_TRACE(raco::log_system::PREVIEW_WIDGET, "{}", *event);
	if (event->type() == QEvent::Type::PlatformSurface) {
		switch (dynamic_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType()) {
			case QPlatformSurfaceEvent::SurfaceEventType::SurfaceAboutToBeDestroyed:
				ramsesPreview_.reset();
				break;
		}
	}
	return QWidget::event(event);
}

void PreviewContentWidget::setViewportRect(
	const QSize areaSize,
	const QPoint viewportPosition,
	const QPoint viewportOffset,
	const QSize viewportSize,
	const QSize virtualSize,
	const QSize targetSize) {
	LOG_TRACE(raco::log_system::PREVIEW_WIDGET, "");
	if (!ramsesPreview_) {
		return;
	}
	if constexpr (BuildOptions::minimalPreviewDisplayArea) {
		// Minimal resize to size of actual viewport
		resize(viewportSize);
		move(viewportPosition);
		ramsesPreview_->state().viewportOffset = viewportOffset;
		ramsesPreview_->state().viewportSize = viewportSize;
		ramsesPreview_->state().virtualSize = virtualSize;
		ramsesPreview_->state().targetSize = targetSize;
	} else {
		// resize to entire area
		resize(areaSize);
		setMask({viewportPosition.x(), viewportPosition.y(), viewportSize.width(), viewportSize.height()});
		ramsesPreview_->state().viewportOffset = (-1 * viewportPosition) + viewportOffset;
		ramsesPreview_->state().viewportSize = areaSize;
		ramsesPreview_->state().virtualSize = virtualSize;
		ramsesPreview_->state().targetSize = targetSize;
	}
	update();
}

void PreviewContentWidget::commit() {
	ramsesPreview_->commit();
}

void PreviewContentWidget::mouseMoveEvent(QMouseEvent* event) {
	Q_EMIT newMousePosition(event->globalPos());
	QWidget::mouseMoveEvent(event);
}

}  // namespace raco::ramses_widgets