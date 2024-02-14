/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/PreviewContentWidget.h"

#include "ramses_widgets/BuildOptions.h"
#include "ramses_widgets/RamsesPreviewWindow.h"
#include "components/QtFormatter.h"
#include <QPaintEvent>
#include <QPlatformSurfaceEvent>
#include <QScreen>

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
		ramsesPreview_->nextState().sceneId = id;
		update();
	}
}

void PreviewContentWidget::setBackgroundColor(core::Vec4f backgroundColor) {
	if (ramsesPreview_) {
		ramsesPreview_->nextState().backgroundColor = QColor::fromRgbF(backgroundColor.x.asDouble(), backgroundColor.y.asDouble(), backgroundColor.z.asDouble(), backgroundColor.w.asDouble());
		update();
	}
}

PreviewMultiSampleRate PreviewContentWidget::getMsaaSampleRate() {
	return ramsesPreview_->nextState().sampleRate;
}

void PreviewContentWidget::setMsaaSampleRate(PreviewMultiSampleRate sampleRate) {
	if (ramsesPreview_) {
		ramsesPreview_->nextState().sampleRate = sampleRate;
		update();
	}
}

ramses::sceneId_t PreviewContentWidget::getSceneId() {
	if (ramsesPreview_) {
		return ramsesPreview_->nextState().sceneId;
	} else {
		return ramses::sceneId_t::Invalid();
	}
}

bool PreviewContentWidget::event(QEvent* event) {
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
	if (!ramsesPreview_) {
		return;
	}
	auto devicePixelScaleFactor = window()->screen()->devicePixelRatio();
	if constexpr (BuildOptions::minimalPreviewDisplayArea) {
		// Minimal resize to size of actual viewport
		resize(viewportSize);
		move(viewportPosition);
		ramsesPreview_->nextState().viewportOffset = viewportOffset * devicePixelScaleFactor;
		ramsesPreview_->nextState().viewportSize = viewportSize * devicePixelScaleFactor;
		ramsesPreview_->nextState().virtualSize = virtualSize;
		ramsesPreview_->nextState().targetSize = targetSize;
	} else {
		// resize to entire area
		resize(areaSize);
		move(0, 0);
		setMask({viewportPosition.x(), viewportPosition.y(), viewportSize.width(), viewportSize.height()});
		ramsesPreview_->nextState().viewportOffset = ((-1 * viewportPosition) + viewportOffset) * devicePixelScaleFactor;
		ramsesPreview_->nextState().viewportSize = areaSize * devicePixelScaleFactor;
		ramsesPreview_->nextState().virtualSize = virtualSize;
		ramsesPreview_->nextState().targetSize = targetSize;
	}
	update();
}

void PreviewContentWidget::paintEvent(QPaintEvent* e) {
	// time scene id changes to paint event instead of our mainWindow timer
	// because the mainWindow timer interval is actually too high for RaCo
	// to register all scene id changes in the UI (?)
	if (ramsesPreview_->currentState().sceneId != ramsesPreview_->nextState().sceneId) {
		ramsesPreview_->commit();
	}
}

void PreviewContentWidget::commit(bool forceUpdate) {
	const auto& currentState = ramsesPreview_->currentState();
	auto& nextState = ramsesPreview_->nextState();
	if (forceUpdate || nextState != currentState && nextState.sceneId == currentState.sceneId) {
		ramsesPreview_->commit(forceUpdate);
	}
}

bool PreviewContentWidget::saveScreenshot(const std::string& fullPath) {
	return ramsesPreview_->saveScreenshot(fullPath);
}

void PreviewContentWidget::mouseMoveEvent(QMouseEvent* event) {
	Q_EMIT newMousePosition(event->globalPos());
	QWidget::mouseMoveEvent(event);
}

}  // namespace raco::ramses_widgets