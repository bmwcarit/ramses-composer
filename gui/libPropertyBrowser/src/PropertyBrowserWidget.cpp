/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserWidget.h"

#include "common_widgets/QtGuiFormatter.h"
#include "core/CoreFormatter.h"
#include "core/Project.h"
#include "core/ProjectSettings.h"
#include "core/SceneBackendInterface.h"
#include "log_system/log.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeView.h"
#include "style/Icons.h"
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QSequentialAnimationGroup>
#include <QTime>

namespace raco::property_browser {

using namespace style;

using SDataChangeDispatcher = components::SDataChangeDispatcher;

template <int Edge>
constexpr bool playEdgeAnimationForPosition(const QPoint& modificationPosition) {
	static_assert(Edge == Qt::BottomEdge || Edge == Qt::TopEdge, "Edge as to be Qt::TopEdget or Qt::BottomEdget");
	// play edge animation regardless of modification position
	return true;
}

template <int Edge>
constexpr QLabel* createNotificationWidget(PropertyBrowserModel* model, QWidget* parent) {
	auto* widget = new QLabel{parent};
	widget->setMinimumHeight(0);
	widget->setMaximumHeight(0);
	auto notificationWidgetPalette = widget->palette();
	notificationWidgetPalette.setColor(QPalette::ColorRole::Window, QColor{25, 25, 200, 60});
	widget->setPalette(notificationWidgetPalette);

	QObject::connect(model, &PropertyBrowserModel::addNotVisible, widget, [widget, parent](QWidget* source) {
		if (playEdgeAnimationForPosition<Edge>(parent->mapFromGlobal(source->mapToGlobal({0, 0})))) {
			auto* group = new QSequentialAnimationGroup{widget};
			auto* outAnimation = new QPropertyAnimation(widget, "maximumHeight");
			outAnimation->setDuration(50);
			outAnimation->setStartValue(0);
			outAnimation->setEndValue(5);
			auto* inAnimation = new QPropertyAnimation(widget, "maximumHeight");
			inAnimation->setDuration(200);
			inAnimation->setStartValue(5);
			inAnimation->setEndValue(0);
			group->addAnimation(outAnimation);
			group->addAnimation(inAnimation);
			group->start(QSequentialAnimationGroup::DeleteWhenStopped);
		}
	});
	return widget;
}

PropertyBrowserView::PropertyBrowserView(core::SceneBackendInterface* sceneBackend, PropertyBrowserItem* item, PropertyBrowserModel* model, QWidget* parent)
	: sceneBackend_{sceneBackend}, QWidget{parent} {
	item->setParent(this);
	auto* layout = new PropertyBrowserGridLayout{this};
	auto* content = new QWidget{this};
	auto* contentLayout = new PropertyBrowserVBoxLayout{content};
	contentLayout->setContentsMargins(0, 0, 5, 0);
	content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

	layout->addWidget(content, 0, 0);

	auto* topNotificationWidget = createNotificationWidget<Qt::TopEdge>(model, this);
	layout->addWidget(topNotificationWidget, 0, 0);
	auto* bottomNotificationWidget = createNotificationWidget<Qt::BottomEdge>(model, this);
	layout->addWidget(bottomNotificationWidget, 2, 0);

	QObject::connect(model, &PropertyBrowserModel::beforeStructuralChange, this, [this, content](QWidget* toChange) {
		auto focusWidget = QApplication::focusWidget();

		if (!focusWidget || !content->isAncestorOf(focusWidget) || focusWidget->visibleRegion().isEmpty()) {
			focusWidget = nullptr;
			// Search for the first label visible in our content
			for (auto label : content->findChildren<QLabel*>()) {
				if (!label->visibleRegion().isEmpty()) {
					if (!focusWidget || focusWidget->mapToGlobal({0, 0}).y() > label->mapToGlobal({0, 0}).y()) {
						focusWidget = label;
					}
				}
			}
		}

		// if the focsed widget is inside the widget which will structurally change
		// then we can only keep that widget focused
		if (toChange->isAncestorOf(focusWidget)) {
			focusWidget = toChange;
		}
		// if we have more than one changed valueHandle in the property browser we need to find the common widget.
		// If we get more complex cases we may also need an afterStructuralChange to clean up the verticalPivotWidget.
		if (verticalPivotWidget_ && verticalPivotWidget_->isAncestorOf(focusWidget)) {
			// Currently we only need to check for hierarchy case
			// e.g. if we have an error the EditorObject and associated engine property table will change (parent -> child)
			focusWidget = verticalPivotWidget_;
		}

		if (focusWidget) {
			verticalPivot_ = mapFromGlobal(focusWidget->mapToGlobal({0, 0}));
			verticalPivotWidget_ = focusWidget;
		}
	});

	contentLayout->addWidget(new PropertySubtreeView{sceneBackend_, model, item, this});
}

PropertyBrowserWidget::PropertyBrowserWidget(
	SDataChangeDispatcher dispatcher,
	core::CommandInterface* commandInterface,
	core::SceneBackendInterface* sceneBackend,
	object_tree::view::ObjectTreeDockManager* treeDockManager,
	QWidget* parent)
	: QWidget{parent},
	  dispatcher_{dispatcher},
	  commandInterface_{commandInterface},
	  sceneBackend_{sceneBackend},
	  treeDockManager_(treeDockManager),
	  layout_{this},
	  emptyLabel_{new QLabel{"Empty", this}},
	  locked_{false},
	  model_{new PropertyBrowserModel(this)} {
	lockButton_ = new QPushButton{this};
	lockButton_->setContentsMargins(0, 0, 0, 0);
	lockButton_->setFlat(true);
	lockButton_->setIcon(Icons::instance().unlocked);
	lockButton_->connect(lockButton_, &QPushButton::clicked, this, [this]() {
		setLocked(!locked_);
	});

	layout_.addWidget(lockButton_, 0, 0, Qt::AlignLeft);
	layout_.addWidget(emptyLabel_, 1, 0, Qt::AlignCenter);
	layout_.setColumnStretch(0, 1);
	layout_.setRowStretch(1, 1);

	subscription_ = dispatcher_->registerOnObjectsLifeCycle([](auto) {}, [this](core::SEditorObject obj) {
		if (propertyBrowser_) {
			if (currentObjects_.find(obj) != currentObjects_.end()) {
				if (locked_) {
					setLocked(false);
				}
				clear();
			}
		} 
	});
}

void PropertyBrowserWidget::setLockable(bool lockable) {
	layout_.itemAtPosition(0, 0)->widget()->setVisible(lockable);
}

void PropertyBrowserWidget::clear() {
	if (!locked_) {
		propertyBrowser_.reset();
		currentObjects_.clear();
		emptyLabel_->setVisible(true);
	}
}

void PropertyBrowserWidget::setLocked(bool locked) {
	bool isLockChanged = locked_ != locked;
	locked_ = locked;
	lockButton_->setIcon(locked_ ? Icons::instance().locked : Icons::instance().unlocked);
	if (!locked_ && treeDockManager_) {
		auto selection = treeDockManager_->getSelection();
		if (!selection.empty()) {
			setObjectsImpl(core::SEditorObjectSet(selection.begin(), selection.end()), isLockChanged);
		} else {
			clear();
			rootItem_ = nullptr;
		}
	}

	if (rootItem_) {
		rootItem_->setLocked(locked_);
	}
}

void PropertyBrowserWidget::setObjectFromObjectId(const QString& objectID) {
	const auto handle = core::ValueHandle{commandInterface_->project()->getInstanceByID(objectID.toStdString())};
	setObjects({handle.rootObject()});
}

PropertyBrowserModel* PropertyBrowserWidget::model() const {
	return model_;
}

void PropertyBrowserWidget::setObjectsImpl(const core::SEditorObjectSet& objects, bool forceExpandStateUpdate) {
	if (propertyBrowser_ && currentObjects_ == objects) {
		// No need to update if we still are referencing to the same objects.
		// This happens for example when the display name changes, thus the tree view will update and then restore the selected items in the property browser.

		if (forceExpandStateUpdate && !locked_) {
			rootItem_->restoreDefaultExpandedRecursively();
		}

		return;
	}

	if (!locked_) {
		emptyLabel_->setVisible(false);

		std::set<core::ValueHandle> valueHandles(objects.begin(), objects.end());
		rootItem_ = new PropertyBrowserItem{valueHandles, dispatcher_, commandInterface_, model_};
		propertyBrowser_.reset(new PropertyBrowserView{sceneBackend_, rootItem_, model_, this});
		currentObjects_ = objects;
		layout_.addWidget(propertyBrowser_.get(), 1, 0);
	}
}

void PropertyBrowserWidget::setObjects(const core::SEditorObjectSet& objects) {
	setObjectsImpl(objects, false);
}



}  // namespace raco::property_browser
