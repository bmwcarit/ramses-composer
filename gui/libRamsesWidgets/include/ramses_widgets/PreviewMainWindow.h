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

	struct CameraParam_t {
		float translation[3];
		float rotation[3];
		float scaling[3];
		int viewport[4];
		float frustum[4];

		bool operator!=(const CameraParam_t& other) {
			return (this->translation[0] != other.translation[0]
				|| this->translation[1] != other.translation[1]
				|| this->translation[2] != other.translation[2]
				|| this->rotation[0] != other.rotation[0]
				|| this->rotation[1] != other.rotation[1]
				|| this->rotation[2] != other.rotation[2]
				|| this->scaling[0] != other.scaling[0]
				|| this->scaling[1] != other.scaling[1]
				|| this->scaling[2] != other.scaling[2]
				|| this->viewport[0] != other.viewport[0]
				|| this->viewport[1] != other.viewport[1]
				|| this->viewport[2] != other.viewport[2]
				|| this->viewport[3] != other.viewport[3]
				|| this->frustum[0] != other.frustum[0]
				|| this->frustum[1] != other.frustum[1]
				|| this->frustum[2] != other.frustum[2]
				|| this->frustum[3] != other.frustum[3]);
		};
	};

	CameraParam_t currentCamera_;
};

}  // namespace raco::ramses_widgets