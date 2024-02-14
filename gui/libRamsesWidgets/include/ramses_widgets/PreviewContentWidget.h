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

#include "ramses_widgets/RamsesPreviewWindow.h"
#include "ramses_widgets/RendererBackend.h"
#include <QCloseEvent>
#include <QWidget>
#include <memory>


namespace raco::ramses_widgets {


class PreviewContentWidget final : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(PreviewContentWidget)
public:
	explicit PreviewContentWidget(RendererBackend& rendererBackend, QWidget* parent = nullptr);
	virtual QPaintEngine* paintEngine() const override { return nullptr; }
	ramses::sceneId_t getSceneId();
	void setSceneId(ramses::sceneId_t id);
	void setBackgroundColor(core::Vec4f backgroundColor);
	void setMsaaSampleRate(PreviewMultiSampleRate sampleRate);
	PreviewMultiSampleRate getMsaaSampleRate();
	void commit(bool forceUpdate);
	bool saveScreenshot(const std::string& fullPath);

public Q_SLOTS:
	/**
	 * @brief setup viewport size and position including scaling
	 * 
	 * The areaSize, viewportPosition, viewportOffset, and viewportSize parameters are given in Qt virtual pixels units.
	 * The virtualSize and targetSize parameters are given in device pixel units.
	*/
	void setViewportRect(
		const QSize areaSize,
		const QPoint viewportPosition,
		const QPoint viewportOffset,
		const QSize viewportSize,
		const QSize virtualSize,
		const QSize targetSize);

Q_SIGNALS:
	void newMousePosition(const QPoint globalPosition);

protected:
	void paintEvent(QPaintEvent* event) override;
	bool event(QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	std::unique_ptr<RamsesPreviewWindow> ramsesPreview_;
};

}  // namespace raco::ramses_widgets
