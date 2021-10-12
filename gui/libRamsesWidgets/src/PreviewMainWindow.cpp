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
#include "user_types/PerspectiveCamera.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/SceneBackend.h"

#include "ui_PreviewMainWindow.h"
#include <QLabel>
#include <QMenu>
#include <QToolButton>

namespace raco::ramses_widgets {

PreviewMainWindow::PreviewMainWindow(RendererBackend& rendererBackend, raco::ramses_adaptor::SceneBackend* sceneBackend, const QSize& sceneSize, raco::core::Project* project,
	raco::components::SDataChangeDispatcher dispatcher, QWidget* parent)
	: QMainWindow{parent},
	  ui_{new Ui::PreviewMainWindow()} {
	ui_->setupUi(this);

	sceneIdLabel_ = new QLabel{"scene id: -", ui_->statusbar};
	auto* pixelLabel = new QLabel{"x: - y: -", ui_->statusbar};
	auto* scaleLabel = new QLabel{"scale: 1.0", ui_->statusbar};
	ui_->statusbar->addWidget(sceneIdLabel_);
	ui_->statusbar->addPermanentWidget(pixelLabel);
	ui_->statusbar->addPermanentWidget(scaleLabel);

	// scroll and zoom logic widget
	scrollAreaWidget_ = new PreviewScrollAreaWidget{sceneSize, this};
	connect(scrollAreaWidget_, &PreviewScrollAreaWidget::scaleChanged, [scaleLabel](double scale) {
		QString content{};
		content.append("scale: ");
		content.append(std::to_string(scale).c_str());
		scaleLabel->setText(content);
	});
	setCentralWidget(scrollAreaWidget_);

	// Actual preview surface
	previewWidget_ = new PreviewContentWidget{rendererBackend, scrollAreaWidget_->viewport()};
	connect(scrollAreaWidget_, &PreviewScrollAreaWidget::viewportRectChanged, previewWidget_, &PreviewContentWidget::setViewportRect);
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
		auto* sizeMenu = new QMenu{this};
		sizeMenu->addAction(ui_->actionSetSizeModeOff);
		sizeMenu->addAction(ui_->actionSetSizeModeVerticalFit);
		sizeMenu->addAction(ui_->actionSetSizeModeHorizontalFit);
		sizeMenu->addAction(ui_->actionSetSizeModeBestFit);
		sizeMenu->addAction(ui_->actionSetSizeModeOriginalFit);
		connect(ui_->actionSetSizeModeOff, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingOff);
		connect(ui_->actionSetSizeModeVerticalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingVerticalFit);
		connect(ui_->actionSetSizeModeHorizontalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingHorizontalFit);
		connect(ui_->actionSetSizeModeBestFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingBestFit);
		connect(ui_->actionSetSizeModeOriginalFit, &QAction::triggered, scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingOriginalFit);
		auto* sizeMenuButton = new QToolButton{this};
		sizeMenuButton->setMenu(sizeMenu);
		sizeMenuButton->setPopupMode(QToolButton::InstantPopup);
		connect(scrollAreaWidget_, &PreviewScrollAreaWidget::autoSizingChanged, [=](PreviewScrollAreaWidget::AutoSizing mode) {
			switch (mode) {
				case PreviewScrollAreaWidget::AutoSizing::OFF:
					sizeMenuButton->setDefaultAction(ui_->actionSetSizeModeOff);
					break;
				case PreviewScrollAreaWidget::AutoSizing::VERTICAL_FIT:
					sizeMenuButton->setDefaultAction(ui_->actionSetSizeModeVerticalFit);
					break;
				case PreviewScrollAreaWidget::AutoSizing::HORIZONTAL_FIT:
					sizeMenuButton->setDefaultAction(ui_->actionSetSizeModeHorizontalFit);
					break;
				case PreviewScrollAreaWidget::AutoSizing::BEST_FIT:
					sizeMenuButton->setDefaultAction(ui_->actionSetSizeModeBestFit);
					break;
				case PreviewScrollAreaWidget::AutoSizing::ORIGINAL_FIT:
					sizeMenuButton->setDefaultAction(ui_->actionSetSizeModeOriginalFit);
					break;
			};
		});
		ui_->toolBar->insertWidget(ui_->actionSelectSizeMode, sizeMenuButton);
	}
}

PreviewMainWindow::~PreviewMainWindow() {
	delete previewWidget_;
}

void PreviewMainWindow::displayScene(ramses::sceneId_t sceneId, data_storage::Vec3f const& backgroundColor) {
	previewWidget_->setBackgroundColor(backgroundColor);
	if (sceneId != previewWidget_->getSceneId()) {
		sceneIdLabel_->setText(QString{"scene id: %1"}.arg(sceneId.getValue()));
		previewWidget_->setSceneId(sceneId);
	}
}

void PreviewMainWindow::setViewport(const QSize& sceneSize) {
	scrollAreaWidget_->setViewport(sceneSize);
}

void PreviewMainWindow::commit() {
	previewWidget_->commit();
}

}  // namespace raco::ramses_widgets
