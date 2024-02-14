/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/AbstractViewMainWindow.h"

#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_widgets/AbstractViewContentWidget.h"

#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>

namespace raco::ramses_widgets {

AbstractViewMainWindow::AbstractViewMainWindow(RendererBackend& rendererBackend, ramses_adaptor::AbstractSceneAdaptor* abstractScene, object_tree::view::ObjectTreeDockManager* objectTreeDockManager, core::CommandInterface *commandInterface, QWidget* parent)
	: QMainWindow{parent},
	  abstractScene_(abstractScene),
	  treeDockManager_(objectTreeDockManager) {
	previewWidget_ = new AbstractViewContentWidget{rendererBackend, abstractScene, objectTreeDockManager, commandInterface};
	setCentralWidget(previewWidget_);

	auto toolBar = new QToolBar(this);
	toolBar->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolBar);

	auto statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	auto modeLabel = new QLabel("Mode: ", statusBar);
	statusBar->addWidget(modeLabel);

	QObject::connect(previewWidget_, &AbstractViewContentWidget::modeChanged, [modeLabel](const QString& description) {
		modeLabel->setText(description);
	});

	auto scaleLabel = new QLabel{QString(fmt::format("scale: {}", abstractScene_->gridScale()).c_str()), statusBar};
	statusBar->addPermanentWidget(scaleLabel);

	QObject::connect(abstractScene, &ramses_adaptor::AbstractSceneAdaptor::scaleChanged, [scaleLabel](float newScale) {
		scaleLabel->setText(QString(fmt::format("scale: {}", newScale).c_str()));
	});

	connect(previewWidget_, &AbstractViewContentWidget::selectionRequested, this, [this](const QString objectID) {
		Q_EMIT selectionRequested(objectID, {});
	});

	// TODO for whatever reason using an Qt::WidgetWithChildrenShortcut won't work. figure out why!
	auto focusAction = toolBar->addAction("focus", [this]() {
		auto objects = treeDockManager_->getSelection();
		if (objects.empty()) {
			objects = abstractScene_->project().instances();
		}
		focusCamera(objects);
	});
	focusAction->setShortcut(QKeySequence("Ctrl+F"));
	focusAction->setShortcutContext(Qt::ApplicationShortcut);

	auto resetAction = toolBar->addAction("reset", [this]() {
		abstractScene_->cameraController().reset();
	});
	resetAction->setShortcut(QKeySequence("Ctrl+0"));
	resetAction->setShortcutContext(Qt::ApplicationShortcut);

	{
		auto menu = new QMenu(toolBar);
		auto highlightGroup = new QActionGroup(this);
		auto actionHighlightNone = menu->addAction("None");
		auto actionHighlightColor = menu->addAction("Color");
		auto actionHighlightTransparency = menu->addAction("Transparency");
		actionHighlightNone->setCheckable(true);
		actionHighlightColor->setCheckable(true);
		actionHighlightTransparency->setCheckable(true);
		highlightGroup->addAction(actionHighlightNone);
		highlightGroup->addAction(actionHighlightColor);
		highlightGroup->addAction(actionHighlightTransparency);
		actionHighlightNone->setChecked(true);

		QObject::connect(highlightGroup, &QActionGroup::triggered, [this, actionHighlightNone, actionHighlightColor](QAction* action) {
			highlightMode_ = action == actionHighlightNone ? HighlightMode::None : (action == actionHighlightColor ? HighlightMode::Color : HighlightMode::Transparency);
			updateHighlighted();
		});

		auto button = new QToolButton(this);
		button->setMenu(menu);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setText("Highlight");
		toolBar->addWidget(button);
	}

	{
		auto menu = new QMenu(toolBar);
		auto gizmoGroup = new QActionGroup(this);
		auto actionGizmoLocator = menu->addAction("Select");
		auto actionGizmoTranslate = menu->addAction("Translate");
		auto actionGizmoRotate = menu->addAction("Rotate");
		auto actionGizmoScale = menu->addAction("Scale");
		actionGizmoLocator->setCheckable(true);
		actionGizmoTranslate->setCheckable(true);
		actionGizmoRotate->setCheckable(true);
		actionGizmoScale->setCheckable(true);
		gizmoGroup->addAction(actionGizmoLocator);
		gizmoGroup->addAction(actionGizmoTranslate);
		gizmoGroup->addAction(actionGizmoRotate);
		gizmoGroup->addAction(actionGizmoScale);
		actionGizmoLocator->setChecked(true);

		using GizmoMode = ramses_adaptor::AbstractSceneAdaptor::GizmoMode;
		std::map<QAction*, GizmoMode> modeMap = {
			{actionGizmoLocator, GizmoMode::Locator},
			{actionGizmoTranslate, GizmoMode::Translate},
			{actionGizmoRotate, GizmoMode::Rotate},
			{actionGizmoScale, GizmoMode::Scale}};

		QObject::connect(gizmoGroup, &QActionGroup::triggered, [this, modeMap](QAction* action) {
			auto it = modeMap.find(action);
			if (it != modeMap.end()) {
				abstractScene_->setGizmoMode(it->second);
			}
		});

		auto button = new QToolButton(this);
		button->setMenu(menu);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setText("Gizmo Mode");
		toolBar->addWidget(button);
	}
}

AbstractViewMainWindow::~AbstractViewMainWindow() {
	delete previewWidget_;
}

void AbstractViewMainWindow::focusCamera(const std::vector<core::SEditorObject>& objects) {
	auto bbox = abstractScene_->getBoundingBox(objects);
	if (!bbox.empty()) {
		abstractScene_->cameraController().focus(bbox);
	}
}

void AbstractViewMainWindow::onSelectionChanged(const core::SEditorObjectSet& objects) {
	updateHighlighted();
	abstractScene_->attachGizmo(treeDockManager_->getSelection());
}

void AbstractViewMainWindow::onSelectionCleared() {
	updateHighlighted();
	abstractScene_->attachGizmo({});
}

void AbstractViewMainWindow::updateHighlighted() {
	switch (highlightMode_) {
		case HighlightMode::None:
			abstractScene_->setHighlightedObjects({});
			abstractScene_->setHighlightUsingTransparency(false);
			break;
		case HighlightMode::Color:
			abstractScene_->setHighlightedObjects(treeDockManager_->getSelection());
			abstractScene_->setHighlightUsingTransparency(false);
			break;
		case HighlightMode::Transparency:
			abstractScene_->setHighlightedObjects(treeDockManager_->getSelection());
			abstractScene_->setHighlightUsingTransparency(true);
			break;
	}
}

}  // namespace raco::ramses_widgets
