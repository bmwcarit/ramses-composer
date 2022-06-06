/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/PreviewMainWindow.h"

#include "ramses_widgets/PreviewContentWidget.h"
#include "ramses_widgets/PreviewScrollAreaWidget.h"
#include "user_types/BaseCamera.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/RenderPass.h"
#include "user_types/Material.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/SceneBackend.h"
#include "core/Queries.h"

#include "ui_PreviewMainWindow.h"
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <windows.h>
#include <log_system/log.h>

namespace raco::ramses_widgets {

using namespace raco::core;
using raco::log_system::PREVIEW_MAIN;

PreviewMainWindow::PreviewMainWindow(RendererBackend& rendererBackend, raco::ramses_adaptor::SceneBackend* sceneBackend, const QSize& sceneSize, raco::core::Project* project,
	raco::components::SDataChangeDispatcher dispatcher, raco::core::CommandInterface* commandInterface, QWidget* parent)
	: project_(project),
	  commandInterface_(commandInterface),
	  sceneBackend_(sceneBackend),
	  QMainWindow{parent},
	  ui_{new Ui::PreviewMainWindow()} {
	ui_->setupUi(this);

	sceneIdLabel_ = new QLabel{"scene id: -", ui_->statusbar};
	auto* pixelLabel = new QLabel{"x: - y: -", ui_->statusbar};
	auto* scaleLabel = new QLabel{"scale: 1.0", ui_->statusbar};
	ui_->statusbar->addWidget(sceneIdLabel_);
	ui_->statusbar->addPermanentWidget(pixelLabel);
	ui_->statusbar->addPermanentWidget(scaleLabel);

	zUp_ = false;//project_->settings()->axes_.asBool();
	updateAxesIconLabel_ = true;
	scaleValue_ = 1.0f;
	// scroll and zoom logic widget
	scrollAreaWidget_ = new PreviewScrollAreaWidget{sceneSize, this};
	connect(scrollAreaWidget_, &PreviewScrollAreaWidget::scaleChanged, [this, scaleLabel](double scale, bool addvalue) {
		QString content{};
		content.append("scale: ");
		content.append(std::to_string(scale).c_str());
		scaleLabel->setText(content);
		this->sceneScaleUpdate(this->zUp_, (float)scale, addvalue);
	});
	setCentralWidget(scrollAreaWidget_);

	// Actual preview surface
	previewWidget_ = new PreviewContentWidget{rendererBackend, scrollAreaWidget_->viewport()};
	connect(scrollAreaWidget_, &PreviewScrollAreaWidget::viewportRectChanged, previewWidget_, &PreviewContentWidget::setViewportRect);
	connect(previewWidget_, &PreviewContentWidget::updateAxesIconLabel, this, &PreviewMainWindow::updateAxesIconLabel);
	connect(previewWidget_, &PreviewContentWidget::newMousePosition, [this, pixelLabel](const QPoint globalPosition) {
		if (auto previewPosition = scrollAreaWidget_->globalPositionToPreviewPosition(globalPosition)) {
			QString content{};
			content.append("x: ");
			content.append(std::to_string(previewPosition->x()).c_str());
			content.append(" y: ");
			content.append(std::to_string(previewPosition->y()).c_str());
			pixelLabel->setText(content);
		} else {
			pixelLabel->setText("x: - y: -");
		}
	});

	// Size mode tool button
	{
		auto* sizeMenu = new QMenu{ui_->toolBar};
		sizeMenu->addAction(ui_->actionSetSizeModeOff);
		sizeMenu->addAction(ui_->actionSetSizeModeVerticalFit);
		sizeMenu->addAction(ui_->actionSetSizeModeHorizontalFit);
		sizeMenu->addAction(ui_->actionSetSizeModeBestFit);
		sizeMenu->addAction(ui_->actionSetSizeModeOriginalFit);
		ui_->actionSetSizeModeOff->setCheckable(true);
		ui_->actionSetSizeModeVerticalFit->setCheckable(true);
		ui_->actionSetSizeModeHorizontalFit->setCheckable(true);
		ui_->actionSetSizeModeBestFit->setCheckable(true);
		ui_->actionSetSizeModeOriginalFit->setCheckable(true);
		connect(ui_->actionSetSizeModeOff, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingOff);
		connect(ui_->actionSetSizeModeVerticalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingVerticalFit);
		connect(ui_->actionSetSizeModeHorizontalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingHorizontalFit);
		connect(ui_->actionSetSizeModeBestFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingBestFit);
		connect(ui_->actionSetSizeModeOriginalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingOriginalFit);
		auto* sizeMenuButton = new QToolButton{ui_->toolBar};
		sizeMenuButton->setMenu(sizeMenu);
		sizeMenuButton->setPopupMode(QToolButton::InstantPopup);
		connect(scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingChanged, [this, sizeMenuButton](PreviewScrollAreaWidget::AutoSizing mode) {
			auto action = ui_->actionSetSizeModeOff;
			switch (mode) {
				case PreviewScrollAreaWidget::AutoSizing::VERTICAL_FIT:
					action = ui_->actionSetSizeModeVerticalFit;
					break;
				case PreviewScrollAreaWidget::AutoSizing::HORIZONTAL_FIT:
					action = ui_->actionSetSizeModeHorizontalFit;
					break;
				case PreviewScrollAreaWidget::AutoSizing::BEST_FIT:
					action = ui_->actionSetSizeModeBestFit;
					break;
				case PreviewScrollAreaWidget::AutoSizing::ORIGINAL_FIT:
					action = ui_->actionSetSizeModeOriginalFit;
					break;
			};
			sizeMenuButton->setText(action->text());
			ui_->actionSetSizeModeOff->setChecked(false);
			ui_->actionSetSizeModeVerticalFit->setChecked(false);
			ui_->actionSetSizeModeHorizontalFit->setChecked(false);
			ui_->actionSetSizeModeBestFit->setChecked(false);
			ui_->actionSetSizeModeOriginalFit->setChecked(false);
			action->setChecked(true);
		});
		ui_->toolBar->insertWidget(ui_->actionSelectSizeMode, sizeMenuButton);
	}
	// Filtering mode tool button
	{
		auto* filteringMenu = new QMenu{ui_->toolBar};
		filteringMenu->addAction(ui_->actionSetFilteringModeNearestNeighbor);
		filteringMenu->addAction(ui_->actionSetFilteringModeLinear);
		ui_->actionSetFilteringModeNearestNeighbor->setCheckable(true);
		ui_->actionSetFilteringModeNearestNeighbor->setChecked(true);


		auto* filteringMenuButton = new QToolButton{ui_->toolBar};
		filteringMenuButton->setMenu(filteringMenu);
		filteringMenuButton->setPopupMode(QToolButton::InstantPopup);

		ui_->actionSetFilteringModeLinear->setCheckable(true);
		filteringMenuButton->setText(ui_->actionSetFilteringModeNearestNeighbor->text());

		connect(ui_->actionSetFilteringModeNearestNeighbor, &QAction::triggered, this, [this, filteringMenuButton]() {
			previewWidget_->setFilteringMode(PreviewFilteringMode::NearestNeighbor);
			filteringMenuButton->setText(ui_->actionSetFilteringModeNearestNeighbor->text());
			ui_->actionSetFilteringModeNearestNeighbor->setChecked(true);
			ui_->actionSetFilteringModeLinear->setChecked(false);
		});
		connect(ui_->actionSetFilteringModeLinear, &QAction::triggered, this, [this, filteringMenuButton]() {
			previewWidget_->setFilteringMode(PreviewFilteringMode::Linear);
			filteringMenuButton->setText(ui_->actionSetFilteringModeLinear->text());
			ui_->actionSetFilteringModeNearestNeighbor->setChecked(false);
			ui_->actionSetFilteringModeLinear->setChecked(true);
		});
		ui_->toolBar->insertWidget(ui_->actionSelectFilteringMode, filteringMenuButton);
	}
	{
		auto* modeMenu = new QMenu{this};
		modeMenu->addAction(ui_->actionModeRoam);
		modeMenu->addAction(ui_->actionModePreview);
		connect(ui_->actionModeRoam, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoModeRoam);
		connect(ui_->actionModePreview, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoModePreview);

		auto* previewModeButton = new QToolButton{this};
		previewModeButton->setMenu(modeMenu);
		previewModeButton->setPopupMode(QToolButton::InstantPopup);
		connect(scrollAreaWidget_, &PreviewScrollAreaWidget::autoPreviewOrRoamModeChanged, [=](PreviewScrollAreaWidget::AutoPreviewMode mode) {
			switch (mode) {
				case PreviewScrollAreaWidget::AutoPreviewMode::ROAM:
					mode_ = 0;
					previewModeButton->setDefaultAction(ui_->actionModeRoam);
					break;
				case PreviewScrollAreaWidget::AutoPreviewMode::PREVIEW:
					mode_ = 1;
					previewModeButton->setDefaultAction(ui_->actionModePreview);
					break;
			};
		});
		ui_->toolBar->insertWidget(ui_->actionPreviewMode, previewModeButton);
	}

}

PreviewMainWindow::~PreviewMainWindow() {
	delete previewWidget_;
}

void PreviewMainWindow::displayScene(ramses::sceneId_t sceneId, core::Vec4f const& backgroundColor) {
	previewWidget_->setBackgroundColor(backgroundColor);
	if (sceneId != previewWidget_->getSceneId()) {
		sceneIdLabel_->setText(QString{"scene id: %1"}.arg(sceneId.getValue()));
		previewWidget_->setSceneId(sceneId);
	}
}

void PreviewMainWindow::sceneScaleUpdate(bool zup, float scaleValue, bool scaleUp) {
	if (mode_ == 1) {
		// preview mode
		return;
	}
	if (scaleValue_ != scaleValue || zup != zUp_) {
		scaleValue_ = scaleValue;
		float x, y, z;
		float cameraScale = (scaleUp == true) ? 0.95 : 1.05;
		(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->getTranslation(x, y, z);
		if (zup) {
			(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setTranslation(x * cameraScale, y * cameraScale, z);
		} else {
			(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setTranslation(x * cameraScale, y, z * cameraScale);
		}
		
		previewWidget_->getRamsesPreview()->getBackgroundScene()->update(zup, scaleValue_);

		auto scene = const_cast<ramses::Scene *>(sceneBackend_->currentScene());
        ramses::RamsesObject* object = scene->findObjectByName("PerspectiveCamera");
        if (object) {
            if (object->getType() == ramses::ERamsesObjectType_PerspectiveCamera) {
                ramses::PerspectiveCamera* camera = static_cast<ramses::PerspectiveCamera*>(object);
				if (zup) {
					camera->setTranslation(x * cameraScale, y * cameraScale, z);
				} else {
					camera->setTranslation(x * cameraScale, y, z * cameraScale);
				}
			}
		}
	}
}

void PreviewMainWindow::setAxesIconLabel(QLabel * axesIcon) {
	axesIcon_ = axesIcon;
}

void PreviewMainWindow::setViewport(const QSize& sceneSize) {
	scrollAreaWidget_->setViewport(sceneSize);
}

void PreviewMainWindow::updateAxesIconLabel() {
	updateAxesIconLabel_ = true;
}

void PreviewMainWindow::setAxesIcon(const bool& z_up) {
	if (sceneSize_.height() != 0 && sceneSize_.width() != 0) {
		previewWidget_->setMask({0, 0, 1, 1});
		previewWidget_->update();
		axesIcon_->clear();
		if (z_up) {
			QPixmap pix = QPixmap(":zUp");
			axesIcon_->setPixmap(pix);
		} else {
			QPixmap pix = QPixmap(":yUp");
			axesIcon_->setPixmap(pix);
		}
		scrollAreaWidget_->setForceUpdateFlag(true);
	}
}

/*
** z_up == true,  +Z up, +Y forward
** z_up == false, +Y up, +Z forward
*/
void PreviewMainWindow::setAxes(const bool& z_up) {
	CameraParam_t cameraParam;

	if (zUp_ == z_up) {
		if (updateAxesIconLabel_) {
			updateAxesIconLabel_ = false;
			scrollAreaWidget_->setForceUpdateFlag(false);
			RamsesPreviewWindow::State state = previewWidget_->getRamsesPreview()->nextState();
			previewWidget_->setMask({state.maskViewportPosition.x(), state.maskViewportPosition.y(),
				state.maskViewportSize.width(), state.maskViewportSize.height()});
		}
		goto end;
	}
	setAxesIcon(z_up);

	for (const auto& object : project_->instances()) {
		if (&object->getTypeDescription() == &user_types::PerspectiveCamera::typeDescription ||
			&object->getTypeDescription() == &user_types::OrthographicCamera::typeDescription ||
			&object->getTypeDescription() == &user_types::Node::typeDescription ||
			&object->getTypeDescription() == &user_types::MeshNode::typeDescription) {
			if (object->getParent() == nullptr) {
				ValueHandle translation_x{object, &user_types::Node::translation_, &core::Vec3f::x};
				ValueHandle translation_y{object, &user_types::Node::translation_, &core::Vec3f::y};
				ValueHandle translation_z{object, &user_types::Node::translation_, &core::Vec3f::z};
				ValueHandle rotation_x{object, &user_types::Node::rotation_, &core::Vec3f::x};
				double x = translation_x.asDouble();
				double y = translation_y.asDouble();
				double z = translation_z.asDouble();
				if (z_up) {
					commandInterface_->set(translation_y, -z);
					commandInterface_->set(translation_z, y);
				} else {
					commandInterface_->set(translation_y, z);
					commandInterface_->set(translation_z, -y);
				}
				double offset = z_up ? (rotation_x.asDouble() + 90.0) : (rotation_x.asDouble() - 90.0);
				if (offset < -360.0) {
					commandInterface_->set(rotation_x, (offset + 360.0));
				} else if (offset > 360.0) {
					commandInterface_->set(rotation_x, (offset - 360.0));
				} else {
					commandInterface_->set(rotation_x, offset);
				}
			}
		}
	}
	previewWidget_->getRamsesPreview()->getBackgroundScene()->update(z_up, scaleValue_);
	zUp_ = z_up;

end:
	for (const auto& object : project_->instances()) {
		if (&object->getTypeDescription() == &user_types::PerspectiveCamera::typeDescription) {
			ValueHandle translation_x{object, &user_types::Node::translation_, &core::Vec3f::x};
			ValueHandle translation_y{object, &user_types::Node::translation_, &core::Vec3f::y};
			ValueHandle translation_z{object, &user_types::Node::translation_, &core::Vec3f::z};
			ValueHandle rotation_x{object, &user_types::Node::rotation_, &core::Vec3f::x};
			ValueHandle rotation_y{object, &user_types::Node::rotation_, &core::Vec3f::y};
			ValueHandle rotation_z{object, &user_types::Node::rotation_, &core::Vec3f::z};
			ValueHandle scaling_x{object, &user_types::Node::scale_, &core::Vec3f::x};
			ValueHandle scaling_y{object, &user_types::Node::scale_, &core::Vec3f::y};
			ValueHandle scaling_z{object, &user_types::Node::scale_, &core::Vec3f::z};
			ValueHandle viewport_x{object, &user_types::BaseCamera::viewport_, &user_types::CameraViewport::offsetX_};
			ValueHandle viewport_y{object, &user_types::BaseCamera::viewport_, &user_types::CameraViewport::offsetY_};
			ValueHandle viewport_w{object, &user_types::BaseCamera::viewport_, &user_types::CameraViewport::width_};
			ValueHandle viewport_h{object, &user_types::BaseCamera::viewport_, &user_types::CameraViewport::height_};
			ValueHandle frustum_near{object, &user_types::PerspectiveCamera::frustum_, &user_types::PerspectiveFrustum::near_};
			ValueHandle frustum_far{object, &user_types::PerspectiveCamera::frustum_, &user_types::PerspectiveFrustum::far_};
			ValueHandle frustum_fov{object, &user_types::PerspectiveCamera::frustum_, &user_types::PerspectiveFrustum::fov_};
			ValueHandle frustum_aspect{object, &user_types::PerspectiveCamera::frustum_, &user_types::PerspectiveFrustum::aspect_};
			cameraParam.translation[0] = translation_x.as<float>();
			cameraParam.translation[1] = translation_y.as<float>();
			cameraParam.translation[2] = translation_z.as<float>();
			cameraParam.rotation[0] = rotation_x.as<float>();
			cameraParam.rotation[1] = rotation_y.as<float>();
			cameraParam.rotation[2] = rotation_z.as<float>();
			cameraParam.scaling[0] = scaling_x.as<float>();
			cameraParam.scaling[1] = scaling_y.as<float>();
			cameraParam.scaling[2] = scaling_z.as<float>();
			cameraParam.viewport[0] = viewport_x.as<int>();
			cameraParam.viewport[1] = viewport_y.as<int>();
			cameraParam.viewport[2] = viewport_w.as<int>();
			cameraParam.viewport[3] = viewport_h.as<int>();
			cameraParam.frustum[0] = frustum_near.as<float>();
			cameraParam.frustum[1] = frustum_far.as<float>();
			cameraParam.frustum[2] = frustum_fov.as<float>();
			cameraParam.frustum[3] = frustum_aspect.as<float>();
			ramses::ERotationConvention type = ramses::ERotationConvention::XYZ;
			if (cameraParam != currentCamera_) {
				(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setTranslation(cameraParam.translation[0], cameraParam.translation[1], cameraParam.translation[2]);
				(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setRotation(cameraParam.rotation[0], cameraParam.rotation[1], cameraParam.rotation[2], type);
				(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setScaling(cameraParam.scaling[0], cameraParam.scaling[1], cameraParam.scaling[2]);
				(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setViewport(cameraParam.viewport[0], cameraParam.viewport[1], cameraParam.viewport[2], cameraParam.viewport[3]);
				(*(previewWidget_->getRamsesPreview()->getBackgroundScene()->getCamera()))->setFrustum(cameraParam.frustum[0], cameraParam.frustum[1], cameraParam.frustum[2], cameraParam.frustum[3]);
				previewWidget_->getRamsesPreview()->getBackgroundScene()->getScene()->flush();
			}
		}
	}

	currentCamera_ = cameraParam;
}

void PreviewMainWindow::commit() {
	previewWidget_->commit();

	const QSize areaSize = scrollAreaWidget_->viewport()->size();
	if (sceneSize_ != areaSize) {
		sceneSize_ = areaSize;
		QRect scrollRect = QRect(scrollAreaWidget_->pos(), scrollAreaWidget_->size());
		axesIcon_->setGeometry(scrollRect.width() - 130, scrollRect.y() + 20, 100, 100);
	}
}

void PreviewMainWindow::setEnableDisplayGrid(bool enable) {
	previewWidget_->setEnableDisplayGrid(enable);
}

}  // namespace raco::ramses_widgets
