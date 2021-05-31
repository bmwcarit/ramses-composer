/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_widgets/RendererBackend.h"
#include "components/DebugInstanceCounter.h"
#include <QLabel>
#include <QMainWindow>
#include <memory>

//
// Widget Hierarchy:
// - root : PreviewMainWindow
//   -> PreviewScrollAreaWidget
//      -> viewport()
//         -> PreviewContentWidget
//

namespace Ui {
class PreviewMainWindow;
}

namespace raco::ramses_widgets {

class PreviewContentWidget;
class PreviewScrollAreaWidget;

class PreviewMainWindow final : public QMainWindow {
	DEBUG_INSTANCE_COUNTER(PreviewMainWindow);

public:
	PreviewMainWindow(RendererBackend& rendererBackend, const QSize& sceneSize, QWidget* parent = nullptr);
	~PreviewMainWindow();
	void displayScene(ramses::sceneId_t sceneId);

public Q_SLOTS:
	void setViewport(const QSize& sceneSize);

private:
	std::unique_ptr<Ui::PreviewMainWindow> ui_;
	PreviewContentWidget* previewWidget_;
	PreviewScrollAreaWidget* scrollAreaWidget_;
	QLabel* sceneIdLabel_;
};

}  // namespace raco::ramses_widgets