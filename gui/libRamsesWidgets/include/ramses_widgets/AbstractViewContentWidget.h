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

#include "ramses_widgets/RamsesAbstractViewWindow.h"
#include "ramses_widgets/RendererBackend.h"
#include "ramses_widgets/TransformationController.h"
#include "ramses_adaptor/AbstractSceneAdaptor.h"

#include "object_tree_view/ObjectTreeDockManager.h"
#include <QCloseEvent>
#include <QWidget>
#include <memory>

namespace raco::ramses_adaptor {
class AbstractSceneAdaptor;
}

namespace raco::ramses_widgets {

class AbstractViewContentWidget final : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(AbstractViewContentWidget)
public:
	explicit AbstractViewContentWidget(RendererBackend& rendererBackend, ramses_adaptor::AbstractSceneAdaptor* abstractScene, object_tree::view::ObjectTreeDockManager* objectTreeDockManager, core::CommandInterface* commandInterface, QWidget* parent = nullptr);
	virtual QPaintEngine* paintEngine() const override { return nullptr; }
	void setSceneId(ramses::sceneId_t id);

Q_SIGNALS:
	void newMousePosition(const QPoint globalPosition);
	void selectionRequested(const QString objectID);
	void modeChanged(const QString& modeDescription);

protected:
	void paintEvent(QPaintEvent* event) override;
	bool event(QEvent* event) override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	void resizeEvent(QResizeEvent* /*event*/) override;

	void updateCameraViewport();
	void handlePickRequest(std::vector<ramses::pickableObjectId_t> pickIds);

private:
	enum class DragMode {
		None = 0,

		PickRequested,

		CameraOrbit,
		CameraPan,
		CameraZoom,

		Translate,
		Rotate,
		Scale
	};

	using TransformMode = TransformationController::TransformMode;
	using Axis = TransformationController::Axis;

	void emitModeChanged();
	void setMode(DragMode dragMode, TransformMode transformMode = TransformMode::Free);
	void setupMode(const std::vector<core::SEditorObject>& selection, DragMode dragMode);
	void setupTransformMode(TransformMode mode, Axis axis = Axis::X);
	void beginDrag();
	void endDrag();
	void abortDrag();

	void updateFromText();

	RendererBackend& rendererBackend_;
	std::unique_ptr<RamsesAbstractViewWindow> ramsesPreview_;
	ramses_adaptor::AbstractSceneAdaptor* abstractScene_;
	object_tree::view::ObjectTreeDockManager* treeDockManager_;
	core::CommandInterface* commandInterface_;
	TransformationController transformationController_;

	QPoint mouseLastPos_;
	
	DragMode dragMode_ = DragMode::None;
	Axis axis_;
	TransformMode transformMode_;
	
	// Direct numeric input of transformation value: current input string
	std::string text_;
	// Direct numeric input of transformation value: current value
	// This is the latest valid value converted from text_
	float value_;

	std::optional<ramses_adaptor::AbstractSceneAdaptor::GizmoMode> gizmoMode_;

	core::SEditorObject activeObject_;
	std::optional<QPoint> dragInitialPos_;
};

}  // namespace raco::ramses_widgets
