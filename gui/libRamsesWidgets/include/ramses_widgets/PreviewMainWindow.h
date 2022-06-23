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

#include "components/DataChangeDispatcher.h"
#include "components/DebugInstanceCounter.h"
#include "ramses_widgets/RendererBackend.h"
#include "core/CommandInterface.h"
#include "user_types/PerspectiveCamera.h"
#include "ramses_base/RamsesHandles.h"
#include "style/Icons.h"
#include <QLabel>
#include <QMainWindow>
#include <QToolButton>
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
		raco::core::CommandInterface* commandInterface,
		QWidget* parent = nullptr);
	~PreviewMainWindow();
	void displayScene(ramses::sceneId_t sceneId, core::Vec4f const& backgroundColor);
	void setAxesIconLabel(QLabel * axesIcon);
	void setAxesIcon(const bool& z_up);
	void sceneScaleUpdate(bool zup, float scaleValue, bool scaleUp);

public Q_SLOTS:
	void setViewport(const QSize& sceneSize);
	void setAxes(const bool& z_up);
	void commit();
	void updateAxesIconLabel();
	void setEnableDisplayGrid(bool enable);
	void sceneUpdate(bool z_up);

private:
	std::unique_ptr<Ui::PreviewMainWindow> ui_;
	PreviewContentWidget* previewWidget_;
	PreviewScrollAreaWidget* scrollAreaWidget_;
	QLabel* sceneIdLabel_;
	raco::core::Project* project_;
	raco::core::CommandInterface* commandInterface_;
	raco::ramses_adaptor::SceneBackend* sceneBackend_;
	bool zUp_;
	QLabel* axesIcon_;
	QSize sceneSize_;
	bool updateAxesIconLabel_;
	float scaleValue_;
	int mode_;
	bool haveInited_;
    QLabel *upLabel_{nullptr};
};

}  // namespace raco::ramses_widgets