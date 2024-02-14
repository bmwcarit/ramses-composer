/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/AbstractViewContentWidget.h"

#include "core/CommandInterface.h"

#include "ramses_widgets/SceneStateEventHandler.h"

#include "ramses_adaptor/utilities.h"

#include <QPaintEvent>
#include <QPlatformSurfaceEvent>
#include <QScreen>
#include <QShortcut>

#include <ramses/renderer/RamsesRenderer.h>
#include <ramses/renderer/RendererSceneControl.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace raco::ramses_widgets {

AbstractViewContentWidget::AbstractViewContentWidget(RendererBackend& rendererBackend, ramses_adaptor::AbstractSceneAdaptor* abstractScene, object_tree::view::ObjectTreeDockManager* objectTreeDockManager, core::CommandInterface* commandInterface, QWidget* parent)
	: QWidget(parent),
	  rendererBackend_(rendererBackend),
	  ramsesPreview_{std::make_unique<RamsesAbstractViewWindow>(reinterpret_cast<void*>(winId()), rendererBackend)},
	  abstractScene_(abstractScene),
	  treeDockManager_(objectTreeDockManager),
	  commandInterface_(commandInterface),
	  transformationController_(abstractScene, commandInterface) {
	// In order to prevent Qt from interfering with Ramses rendering into the window we need to set these flags
	// and also override the QWidget::paintEngine method (returning a nullptr).
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setSceneId(abstractScene_->sceneId());
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	QObject::connect(&rendererBackend_.eventHandler(), &SceneStateEventHandler::pickRequest, this, &AbstractViewContentWidget::handlePickRequest);
}

void AbstractViewContentWidget::setSceneId(ramses::sceneId_t id) {
	if (ramsesPreview_) {
		ramsesPreview_->nextState().sceneId = id;
		update();
	}
}

bool AbstractViewContentWidget::event(QEvent* event) {
	if (event->type() == QEvent::Type::PlatformSurface) {
		switch (dynamic_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType()) {
			case QPlatformSurfaceEvent::SurfaceEventType::SurfaceAboutToBeDestroyed:
				ramsesPreview_.reset();
				break;
		}
	}
	return QWidget::event(event);
}

void AbstractViewContentWidget::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	updateCameraViewport();
	if (ramsesPreview_) {
		ramsesPreview_->nextState().viewportSize = size() * window()->screen()->devicePixelRatio();
		update();
	}
}

void AbstractViewContentWidget::updateCameraViewport() {
	auto ramsesSize = size() * window()->screen()->devicePixelRatio();
	abstractScene_->rescaleCameraToViewport(ramsesSize.width(), ramsesSize.height());
}

void AbstractViewContentWidget::paintEvent(QPaintEvent* e) {
	if (ramsesPreview_->currentState() != ramsesPreview_->nextState()) {
		ramsesPreview_->commit();
	}
}

void AbstractViewContentWidget::emitModeChanged() {
	std::map<DragMode, QString> modeDesc = {
		{DragMode::None, ""},
		{DragMode::CameraOrbit, "Orbit"},
		{DragMode::CameraPan, "Pan"},
		{DragMode::CameraZoom, "Zoom"},
		{DragMode::Translate, "Translate"},
		{DragMode::Rotate, "Rotate"},
		{DragMode::Scale, "Scale"}};

	std::map<Axis, QString> axisLabel = {
		{Axis::X, "X"},
		{Axis::Y, "Y"},
		{Axis::Z, "Z"}};
	std::map<Axis, QString> planeLabel = {
		{Axis::X, "YZ"},
		{Axis::Y, "XZ"},
		{Axis::Z, "XY"}};

	QString text = modeDesc.find(dragMode_)->second;
	if (dragMode_ >= DragMode::Translate) {
		switch (transformMode_) {
			case TransformMode::Axis:
				text = text + " " + axisLabel[axis_];
				break;
			case TransformMode::Plane:
				text = text + " " + planeLabel[axis_];
				break;
			case TransformMode::Free:
				break;
		}
	}

	if (!text_.empty()) {
		std::string msg = fmt::format(" by '{}' -> {}", text_, value_);
		text = text + QString::fromStdString(msg);
	}

	Q_EMIT modeChanged(text);
}

void AbstractViewContentWidget::setMode(DragMode dragMode, TransformMode transformMode) {
	dragMode_ = dragMode;
	transformMode_ = transformMode;
	text_.clear();
	value_ = 0.0;
	emitModeChanged();
}

void AbstractViewContentWidget::beginDrag() {
	dragInitialPos_.reset();

	switch (dragMode_) {
		case DragMode::Translate:
			transformationController_.beginDrag(activeObject_, TransformationController::DragMode::Translate, transformMode_, axis_);
			break;
		case DragMode::Scale:
			transformationController_.beginDrag(activeObject_, TransformationController::DragMode::Scale, transformMode_, axis_);
			break;
		case DragMode::Rotate:
			transformationController_.beginDrag(activeObject_, TransformationController::DragMode::Rotate, transformMode_, axis_);
			break;
	}
	if (abstractScene_->gizmoMode() != ramses_adaptor::AbstractSceneAdaptor::GizmoMode::None) {
		gizmoMode_ = abstractScene_->gizmoMode();
	}
	abstractScene_->setGizmoMode(ramses_adaptor::AbstractSceneAdaptor::GizmoMode::None);
}

void AbstractViewContentWidget::endDrag() {
	setMode(DragMode::None);
	transformationController_.endDrag();
	if (gizmoMode_) {
		abstractScene_->setGizmoMode(gizmoMode_.value());
		gizmoMode_.reset();
	}
}

void AbstractViewContentWidget::abortDrag() {
	setMode(DragMode::None);
	transformationController_.abortDrag();
	if (gizmoMode_) {
		abstractScene_->setGizmoMode(gizmoMode_.value());
		gizmoMode_.reset();
	}
}

void AbstractViewContentWidget::setupMode(const std::vector<core::SEditorObject>& selection, DragMode dragMode) {
	if (selection.size() != 1) {
		return;
	}
	activeObject_ = selection.front();

	bool canSetProperty = false;
	switch (dragMode) {
		case DragMode::Translate:
			canSetProperty = commandInterface_->canSetHandle({activeObject_, &user_types::Node::translation_});
			break;
		case DragMode::Rotate:
			canSetProperty = commandInterface_->canSetHandle({activeObject_, &user_types::Node::rotation_});
			break;
		case DragMode::Scale:
			canSetProperty = commandInterface_->canSetHandle({activeObject_, &user_types::Node::scaling_});
			break;
	}
	if (!abstractScene_->hasModelMatrix(activeObject_)) {
		canSetProperty = false;
	}
	if (canSetProperty) {
		setMode(dragMode);
		beginDrag();
	} else {
		setMode(DragMode::None);
	}
}

void AbstractViewContentWidget::setupTransformMode(TransformMode mode, Axis axis) {
	if (dragMode_ != DragMode::None) {
		transformMode_ = mode;
		axis_ = axis;
		emitModeChanged();

		beginDrag();
	}
}

void AbstractViewContentWidget::updateFromText() {
	size_t numProcessed = 0;
	float value;
	try {
		value = std::stof(text_, &numProcessed);
	} catch (std::exception&) {
	}
	if (numProcessed > 0 && numProcessed == text_.length()) {
		value_ = value;

		switch (dragMode_) {
			case DragMode::Translate:
				transformationController_.translate(value_);
				break;
			case DragMode::Rotate:
				transformationController_.rotate(value_);
				break;
			case DragMode::Scale:
				transformationController_.scale(value_);
				break;
		}
	}
	emitModeChanged();
}

void AbstractViewContentWidget::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Escape) {
		abortDrag();
	} else {
		switch (dragMode_) {
			case DragMode::None:
				switch (event->key()) {
					case Qt::Key_G:
						setupMode(treeDockManager_->getSelection(), DragMode::Translate);
						return;
					case Qt::Key_S:
						setupMode(treeDockManager_->getSelection(), DragMode::Scale);
						break;
					case Qt::Key_R:
						setupMode(treeDockManager_->getSelection(), DragMode::Rotate);
						break;
				}
				break;

			case DragMode::Translate:
			case DragMode::Rotate:
			case DragMode::Scale: {
				TransformMode transformationMode = TransformMode::Axis;
				if (dragMode_ != DragMode::Rotate && event->modifiers() == Qt::ShiftModifier) {
					transformationMode = TransformMode::Plane;
				}
				switch (event->key()) {
					case Qt::Key_X:
						setupTransformMode(transformationMode, Axis::X);
						break;
					case Qt::Key_Y:
						setupTransformMode(transformationMode, Axis::Y);
						break;
					case Qt::Key_Z:
						setupTransformMode(transformationMode, Axis::Z);
						break;

					case Qt::Key_Backspace:
						if (!text_.empty()) {
							text_.resize(text_.size() - 1);
							updateFromText();
						}
						break;

					case Qt::Key_Return:
						endDrag();
						break;

					default: {
						// Exclude free and plane translation from numeric input since they need 2 input values and we only have 1:
						if (dragMode_ != DragMode::Translate || transformMode_ == TransformMode::Axis) {
							std::string text = event->text().toStdString();
							if (text.size() == 1) {
								auto c = text[0];
								if (std::string("1234567890.-").find(c) != std::string::npos) {
									text_ = text_ + c;
									updateFromText();
								}
							}
						}
					} break;
				}
				break;
			}
		}
	}

	QWidget::keyPressEvent(event);
}

void AbstractViewContentWidget::keyReleaseEvent(QKeyEvent* event) {
	QWidget::keyReleaseEvent(event);
}

void AbstractViewContentWidget::mousePressEvent(QMouseEvent* event) {
	mouseLastPos_ = event->pos();

	if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
		if (event->modifiers() == Qt::NoModifier) {
			setMode(DragMode::CameraOrbit);
		} else if (event->modifiers() == Qt::ShiftModifier) {
			setMode(DragMode::CameraPan);
		} else if (event->modifiers() == Qt::ControlModifier) {
			setMode(DragMode::CameraZoom);
		}
	}

	if (dragMode_ == DragMode::None && event->button() == Qt::LeftButton) {
		// Picking
		auto pos = event->localPos();
		float relX = 2.0 * pos.x() / width() - 1.0;
		float relY = 1.0 - 2.0 * pos.y() / height();

		auto sceneControl = rendererBackend_.renderer().getSceneControlAPI();
		sceneControl->handlePickEvent(ramsesPreview_->currentState().sceneId, relX, relY);
		sceneControl->flush();

		dragMode_ = DragMode::PickRequested;
	}
}

void AbstractViewContentWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		endDrag();
	} else {
		abortDrag();
	}
}

void AbstractViewContentWidget::mouseMoveEvent(QMouseEvent* event) {
	// camera controls as in Blender:
	// - no modifier -> orbit
	// - Shift -> pan
	// - Ctrl -> zoom

	if (!dragInitialPos_) {
		dragInitialPos_ = event->pos();
	}

	bool snap = event->modifiers() & Qt::ControlModifier;

	auto deltaPos = event->pos() - mouseLastPos_;
	switch (dragMode_) {
		case DragMode::CameraOrbit:
			abstractScene_->cameraController().orbitCamera(deltaPos);
			break;
		case DragMode::CameraPan:
			abstractScene_->cameraController().panCamera(deltaPos);
			break;
		case DragMode::CameraZoom:
			abstractScene_->cameraController().zoomCameraMove(deltaPos);
			break;

		case DragMode::Translate:
			if (text_.empty()) {
				transformationController_.dragTranslate(dragInitialPos_.value(), event->pos(), width(), height(), snap, abstractScene_->gridScale());
			}
			break;
		case DragMode::Scale:
			if (text_.empty()) {
				transformationController_.dragScale(dragInitialPos_.value(), event->pos(), width(), height(), snap);
			}
			break;
		case DragMode::Rotate:
			if (text_.empty()) {
				transformationController_.dragRotate(dragInitialPos_.value(), event->pos(), width(), height(), snap);
			}
			break;
	}
	mouseLastPos_ = event->pos();
}

void AbstractViewContentWidget::wheelEvent(QWheelEvent* event) {
	if (!event->angleDelta().isNull()) {
		abstractScene_->cameraController().zoomCameraWheel(event->angleDelta());
	}
}

void AbstractViewContentWidget::handlePickRequest(std::vector<ramses::pickableObjectId_t> pickIds) {
	auto [axis, element] = abstractScene_->getPickedGizmoElement(pickIds);
	if (axis != -1) {
		if (dragMode_ == DragMode::PickRequested) {
			using GizmoMode = ramses_adaptor::AbstractSceneAdaptor::GizmoMode;
			auto gizmoMode = abstractScene_->gizmoMode();
			if (gizmoMode == GizmoMode::Translate || gizmoMode == GizmoMode::Rotate || gizmoMode == GizmoMode::Scale) {
				std::map<GizmoMode, DragMode> modeMap = {
					{GizmoMode::Translate, DragMode::Translate},
					{GizmoMode::Rotate, DragMode::Rotate},
					{GizmoMode::Scale, DragMode::Scale}};
				DragMode dragMode = modeMap[abstractScene_->gizmoMode()];
				switch (element) {
					case ramses_adaptor::GizmoTriad::PickElement::Axis:
						setupMode(treeDockManager_->getSelection(), dragMode);
						setupTransformMode(TransformMode::Axis, static_cast<Axis>(axis));
						break;
					case ramses_adaptor::GizmoTriad::PickElement::Plane:
						if (dragMode != DragMode::Rotate) {
							setupMode(treeDockManager_->getSelection(), dragMode);
							setupTransformMode(TransformMode::Plane, static_cast<Axis>(axis));
						}
						break;
					case ramses_adaptor::GizmoTriad::PickElement::Central:
						// Don't enable free scaling or rotation when picking central gizmo element since the mouse is too close to the origin
						// and this would lead to erratic behaviour.
						if (dragMode == DragMode::Translate) {
							setupMode(treeDockManager_->getSelection(), dragMode);
						}
						break;
				}
			}
		}
	} else {
		auto obj = abstractScene_->getPickedObject(pickIds.front());
		if (obj) {
			Q_EMIT selectionRequested(QString::fromStdString(obj->objectID()));
		}
	}
}

}  // namespace raco::ramses_widgets