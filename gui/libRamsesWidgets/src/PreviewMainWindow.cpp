/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/PreviewMainWindow.h"

#include "components/RaCoPreferences.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_widgets/PreviewContentWidget.h"
#include "ramses_widgets/PreviewScrollAreaWidget.h"
#include "ramses_widgets/SceneStateEventHandler.h"
#include "user_types/PerspectiveCamera.h"
#include "style/Icons.h"

#include "ui_PreviewMainWindow.h"
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QHBoxLayout>


namespace raco::ramses_widgets {

PreviewMainWindow::PreviewMainWindow(RendererBackend& rendererBackend, ramses_adaptor::SceneBackend* sceneBackend, const QSize& sceneSize, core::Project* project,
	components::SDataChangeDispatcher dispatcher, QWidget* parent)
	: QMainWindow{parent},
	  ui_{new Ui::PreviewMainWindow()},
	  project_(project) {
	ui_->setupUi(this);

	sceneIdLabel_ = new QLabel{"scene id: -", ui_->statusbar};
	errorLabel_ = new QLabel{"Status", ui_->statusbar};
	auto* pixelLabel = new QLabel{"x: - y: -", ui_->statusbar};
	auto* scaleLabel = new QLabel{"scale: 1.0", ui_->statusbar};
	ui_->statusbar->addWidget(sceneIdLabel_);
	ui_->statusbar->addWidget(errorLabel_);
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

	connect(previewWidget_, &PreviewContentWidget::newPreviewState, [this](bool status) {
		errorLabel_->setText(QString("Status: ") + QString(status ? "OK" : "ERROR"));
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

	// MSAA button
	{
		auto* msaaMenu = new QMenu{ui_->toolBar};
		msaaMenu->addAction(ui_->actionSetMSAAx0);
		msaaMenu->addAction(ui_->actionSetMSAAx2);
		msaaMenu->addAction(ui_->actionSetMSAAx4);
		msaaMenu->addAction(ui_->actionSetMSAAx8);
		ui_->actionSetMSAAx0->setCheckable(true);
		ui_->actionSetMSAAx0->setChecked(true);
		ui_->actionSetMSAAx2->setCheckable(true);
		ui_->actionSetMSAAx4->setCheckable(true);
		ui_->actionSetMSAAx8->setCheckable(true);

		auto* msaaMenuButton = new QToolButton{ui_->toolBar};
		msaaMenuButton->setMenu(msaaMenu);
		msaaMenuButton->setPopupMode(QToolButton::InstantPopup);

		msaaMenuButton->setText(ui_->actionSetMSAAx0->text());

		auto updateMsaaSelection = [this, msaaMenuButton](QAction* action) {
			msaaMenuButton->setText(action->text());
			ui_->actionSetMSAAx0->setChecked(ui_->actionSetMSAAx0 == action);
			ui_->actionSetMSAAx2->setChecked(ui_->actionSetMSAAx2 == action);
			ui_->actionSetMSAAx4->setChecked(ui_->actionSetMSAAx4 == action);
			ui_->actionSetMSAAx8->setChecked(ui_->actionSetMSAAx8 == action);
		};

		connect(ui_->actionSetMSAAx0, &QAction::triggered, this, [this, msaaMenuButton, updateMsaaSelection]() {
			previewWidget_->setMsaaSampleRate(PreviewMultiSampleRate::MSAA_0X);
			updateMsaaSelection(ui_->actionSetMSAAx0);
			ui_->screenshotButton->setEnabled(true);
		});
		connect(ui_->actionSetMSAAx2, &QAction::triggered, this, [this, msaaMenuButton, updateMsaaSelection]() {
			previewWidget_->setMsaaSampleRate(PreviewMultiSampleRate::MSAA_2X);
			updateMsaaSelection(ui_->actionSetMSAAx2);
			ui_->screenshotButton->setEnabled(false);
		});
		connect(ui_->actionSetMSAAx4, &QAction::triggered, this, [this, msaaMenuButton, updateMsaaSelection]() {
			previewWidget_->setMsaaSampleRate(PreviewMultiSampleRate::MSAA_4X);
			updateMsaaSelection(ui_->actionSetMSAAx4);
			ui_->screenshotButton->setEnabled(false);
		});
		connect(ui_->actionSetMSAAx8, &QAction::triggered, this, [this, msaaMenuButton, updateMsaaSelection]() {
			previewWidget_->setMsaaSampleRate(PreviewMultiSampleRate::MSAA_8X);
			updateMsaaSelection(ui_->actionSetMSAAx8);
			ui_->screenshotButton->setEnabled(false);
		});
		ui_->toolBar->insertWidget(ui_->actionSelectSizeMode, msaaMenuButton);
	}

	// Screenshot button
	ui_->screenshotButton->setIcon(style::Icons::instance().screenshot);
	ui_->screenshotButton->setToolTip("Save Screenshot (F12)");
	connect(ui_->screenshotButton, &QPushButton::clicked, [this]() {
		saveScreenshot();
	});

	// Refresh button
	ui_->refreshButton->setIcon(style::Icons::instance().refresh);
	ui_->refreshButton->setToolTip("Refresh");
	connect(ui_->refreshButton, &QPushButton::clicked, [this]() {
		previewWidget_->commit(true);
	});

	// Buttons layout
	{
		auto* stretchedWidget = new QWidget(ui_->toolBar);
		auto* layout = new QHBoxLayout(stretchedWidget);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addStretch();
		layout->addWidget(ui_->screenshotButton);
		layout->addWidget(ui_->refreshButton);
		stretchedWidget->setLayout(layout);
		ui_->toolBar->addWidget(stretchedWidget);
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

void PreviewMainWindow::setViewport(const QSize& sceneSize) {
	scrollAreaWidget_->setViewport(sceneSize);
}

void PreviewMainWindow::commit(bool forceUpdate) {
	previewWidget_->commit(forceUpdate);
}

void PreviewMainWindow::saveScreenshot() {
	if (previewWidget_->getMsaaSampleRate() > MSAA_0X) {
		QMessageBox::warning(this, "Could not save the screenshot", "Screenshots are only possible for non-MSAA buffers!", QMessageBox::Ok);
		return;
	}

	const auto screenshotDir = components::RaCoPreferences::instance().screenshotDirectory.toStdString();
	if (screenshotDir.empty()) {
		QMessageBox::warning(this, "Could not save the screenshot", "Please make sure that the directory specified in \"File > Preferences > Screenshot Directory\" is not empty.", QMessageBox::Ok);
		return;
	}

	auto projectName = project_->projectName();
	if (projectName.empty()) {
		projectName = "default";
	}

	const auto currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-zzz").toStdString();
	const auto screenshotFileName = screenshotDir + "/" + projectName + "_" + currentTime + ".png";

	const auto saved = previewWidget_->saveScreenshot(screenshotFileName);
	if (!saved) {
		QMessageBox::warning(this, "Saving screenshot failed", "Could not save the screenshot. Please make sure that the directory specified in \"File > Preferences > Screenshot Directory\" exists and is accessable.", QMessageBox::Ok);
	}
}

void PreviewMainWindow::saveScreenshot(const std::string& fullPath) {
	if (previewWidget_->getMsaaSampleRate() > MSAA_0X) {
		throw std::runtime_error{"Could not save screenshot to \"" + fullPath + "\". Screenshots are only possible for non-MSAA buffers!"};
		return;
	}

	const auto saved = previewWidget_->saveScreenshot(fullPath);
	if (!saved) {
		throw std::runtime_error {"Could not save screenshot to \"" + fullPath + "\". Please make sure that the path specified is correct and accessable."};
	}
}


}  // namespace raco::ramses_widgets
