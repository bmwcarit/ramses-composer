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

#include "PreviewContentWidget.h"
#include "components/DataChangeDispatcher.h"
#include "components/DebugInstanceCounter.h"
#include "ramses_widgets/RendererBackend.h"
#include <QLabel>
#include <QMainWindow>
#include <memory>

namespace raco::user_types {

class BaseCamera;
using SBaseCamera = std::shared_ptr<BaseCamera>;
}

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

namespace raco::ramses_adaptor {
class SceneBackend;
}

namespace raco::ramses_widgets {

class PreviewContentWidget;
class PreviewScrollAreaWidget;

class PreviewMainWindow final : public QMainWindow {
	DEBUG_INSTANCE_COUNTER(PreviewMainWindow);

public:
	PreviewMainWindow(RendererBackend& rendererBackend,
		raco::ramses_adaptor::SceneBackend* sceneBackend,
		const QSize& sceneSize,
		raco::core::Project* project,
		raco::components::SDataChangeDispatcher dispatcher,
		QWidget* parent = nullptr);
	~PreviewMainWindow();
	void displayScene(ramses::sceneId_t sceneId, core::Vec4f const& backgroundColor);

public Q_SLOTS:
	void setViewport(const QSize& sceneSize);
	void commit(bool forceUpdate);
	void saveScreenshot();
	void saveScreenshot(const std::string& fullPath);

private:
	std::unique_ptr<Ui::PreviewMainWindow> ui_;
	core::Project* project_;
	PreviewContentWidget* previewWidget_;
	PreviewScrollAreaWidget* scrollAreaWidget_;
	QLabel* sceneIdLabel_;
};

}  // namespace raco::ramses_widgets