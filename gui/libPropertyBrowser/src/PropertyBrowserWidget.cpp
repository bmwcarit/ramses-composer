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
#include "core/SceneBackendInterface.h"
#include "log_system/log.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeView.h"
#include "core/ProjectSettings.h"
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

using namespace raco::style;

using SDataChangeDispatcher = raco::components::SDataChangeDispatcher;

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

PropertyBrowserView::PropertyBrowserView(raco::core::SceneBackendInterface* sceneBackend, PropertyBrowserItem* item, PropertyBrowserModel* model, QWidget* parent)
	: currentObjectID_(item->valueHandle().rootObject()->objectID()), sceneBackend_{sceneBackend}, QWidget{parent} {
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

std::string PropertyBrowserView::getCurrentObjectID() const {
	return currentObjectID_;
}

PropertyBrowserWidget::PropertyBrowserWidget(
	SDataChangeDispatcher dispatcher,
	raco::core::CommandInterface* commandInterface,
	raco::core::SceneBackendInterface* sceneBackend,
	QWidget* parent)
	: QWidget{parent},
	  dispatcher_{dispatcher},
	  commandInterface_{commandInterface},
	  sceneBackend_{sceneBackend},
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
}

void PropertyBrowserWidget::setLockable(bool lockable) {
	layout_.itemAtPosition(0, 0)->widget()->setVisible(lockable);
}

void PropertyBrowserWidget::clear() {
	clearValueHandle(false);
}

void PropertyBrowserWidget::setLocked(bool locked) {
	locked_ = locked;
	lockButton_->setIcon(locked_ ? Icons::instance().locked : Icons::instance().unlocked);
}

void PropertyBrowserWidget::clearValueHandle(bool restorable) {
	if (!locked_) {
		if (restorable && propertyBrowser_) {
			subscription_ = dispatcher_->registerOnObjectsLifeCycle([this](core::SEditorObject obj) { 
				if (restorableObjectId_ == obj->objectID()) {
					setValueHandle({ obj });
				}}, [](auto) {});

		} else {
			restorableObjectId_ = "";
			subscription_ = raco::components::Subscription{};
		}
		propertyBrowser_.reset();
		emptyLabel_->setVisible(true);
	}
}

void PropertyBrowserWidget::setValueHandleFromObjectId(const QString& objectID) {
	setValueHandle(commandInterface_->project()->getInstanceByID(objectID.toStdString()));
}

void PropertyBrowserWidget::setValueHandle(core::ValueHandle valueHandle) {
	if (propertyBrowser_ && propertyBrowser_->getCurrentObjectID() == valueHandle.rootObject()->objectID()) {
		// No need to update the Value Handle if we still are referencing to the same object.
		// This happens for example when the display name changes, thus the tree view will update and then restore the selected item in the property browser.
		return;	
	}

	if (!locked_) {
		emptyLabel_->setVisible(false);
		restorableObjectId_ = valueHandle.rootObject()->objectID();
		subscription_ = dispatcher_->registerOnObjectsLifeCycle([](auto) {}, [this, valueHandle](core::SEditorObject obj) {
			if (valueHandle.rootObject() == obj) {
				// SaveAs with new project ID will delete the ProjecSettings object and create a new one in order to change the object ID.
				// We want to move a ProjectSettings property browser to the new ProjectSettings object automatically,
				// so we detect this case and instead of clearing we find the new settings object and set a new ValueHandle with it.
				if (obj->isType<raco::core::ProjectSettings>()) {
					setValueHandle({commandInterface_->project()->settings()});
				} else {
					if (locked_) {
						setLocked(false);
					}
					clearValueHandle(true);
				}
			}
		});
		propertyBrowser_.reset(new PropertyBrowserView{sceneBackend_, new PropertyBrowserItem{valueHandle, dispatcher_, commandInterface_, sceneBackend_, model_}, model_, this});
		layout_.addWidget(propertyBrowser_.get(), 1, 0);
	} else {
		LOG_DEBUG(log_system::PROPERTY_BROWSER, "locked! ignore value handle set {}", valueHandle);
	}
}

PropertyBrowserModel* PropertyBrowserWidget::model() const {
	return model_;
}

void PropertyBrowserWidget::setValueHandles(const std::set<raco::core::ValueHandle>& valueHandles) {
	//QTime t;
	//t.start();
	if (valueHandles.size() > 1) {
		clearValueHandle(false);
	} else {
		setValueHandle(*valueHandles.begin());
	}
	//LOG_DEBUG(log_system::PROPERTY_BROWSER, "ms for setting values handles: {}", t.elapsed() );
}

}  // namespace raco::property_browser
