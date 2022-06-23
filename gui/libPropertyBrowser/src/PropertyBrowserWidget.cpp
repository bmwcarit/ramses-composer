/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserWidget.h"

#include "common_widgets/QtGuiFormatter.h"
#include "core/CoreFormatter.h"
#include "log_system/log.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeView.h"
#include "NodeData/nodeManager.h"
#include "node_logic/NodeLogic.h"
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
using namespace raco::guiData;
using namespace raco::node_logic;

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

bool getObjId(raco::core::ValueHandle valueHandle, std::string& objId, std::string& nodeName) {
    for (int i{0}; i < valueHandle.size(); i++) {
        if (!valueHandle[i].isObject()) {
            if (valueHandle[i].getPropName().compare("objectID") == 0) {
                objId = valueHandle[i].asString();
                nodeName = valueHandle[i].getPropertyPath();
                return true;
            }
        }
        return getObjId(valueHandle, objId, nodeName);
    }
    return false;
}

PropertyBrowserView::PropertyBrowserView(PropertyBrowserItem* item, PropertyBrowserModel* model, QWidget* parent)
	: currentObjectID_(item->valueHandle().rootObject()->objectID()), QWidget{parent} {
	item->setParent(this);
	auto* layout = new PropertyBrowserGridLayout{this};
	layout->setColumnStretch(0, 1);
	auto* content = new QWidget{this};
	auto* contentLayout = new PropertyBrowserVBoxLayout{content};
	contentLayout->setAlignment(Qt::AlignTop);
	contentLayout->setContentsMargins(0, 0, 5, 0);
	content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

	auto* scrollArea = new QScrollArea{this};
	scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
	layout->addWidget(scrollArea, 0, 0, 3, Qt::AlignTop);
	scrollArea->setWidget(content);
	scrollArea->setWidgetResizable(true);

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

	QObject::connect(scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this, scrollArea](int min, int max) {
		if (verticalPivotWidget_) {
			auto newPosition = mapFromGlobal(verticalPivotWidget_->mapToGlobal({0, 0}));
			scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + (newPosition.y() - verticalPivot_.y()));
		}
		verticalPivotWidget_ = nullptr;
		verticalPivot_ = {0, 0};
	});

	contentLayout->addWidget(new PropertySubtreeView{model, item, this});
}

std::string PropertyBrowserView::getCurrentObjectID() const {
	return currentObjectID_;
}

PropertyBrowserWidget::PropertyBrowserWidget(
	SDataChangeDispatcher dispatcher,
	raco::core::CommandInterface* commandInterface,
	QWidget* parent)
	: QWidget{parent},
	  dispatcher_{dispatcher},
	  commandInterface_{commandInterface},
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
    initPropertyBrowserWidget();

    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateActiveAnimation_From_AnimationLogic, this, &PropertyBrowserWidget::slotRefreshPropertyBrowser);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInsertCurveBinding_From_NodeUI, this, &PropertyBrowserWidget::slotInsertCurveBinding);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &PropertyBrowserWidget::slotResetPropertyBrowser);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitPropertyBrowserView, this, &PropertyBrowserWidget::slotRefreshPropertyBrowser);
}

void PropertyBrowserWidget::setLockable(bool lockable) {
	layout_.itemAtPosition(0, 0)->widget()->setVisible(lockable);
}

void PropertyBrowserWidget::slotInsertCurveBinding(QString property, QString curve) {
    std::string stdstrCurve = curve.toStdString();
    std::string stdstrProperty = property.toStdString();

    std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
    if (sampleProperty == std::string()) {
        return;
    }
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().insertBindingDataItem(sampleProperty, stdstrProperty, stdstrCurve);
    curveBindingWidget_->insertCurveBinding(QString::fromStdString(sampleProperty), property, curve);
}

void PropertyBrowserWidget::slotTreeMenu(const QPoint &pos) {
    QMenu menu;
    QAction* action = new QAction("insert KeyFrame");
    menu.addAction(action);

    menu.exec(QCursor::pos());
}

void PropertyBrowserWidget::slotRefreshPropertyBrowser() {
    if (curveBindingWidget_) {
        curveBindingWidget_->initCurveBindingWidget();
    }
    if (meshWidget_) {
        meshWidget_->initPropertyBrowserMeshWidget();
    }
    if (customWidget_) {
        customWidget_->initPropertyBrowserCustomWidget();
    }
}

void PropertyBrowserWidget::slotResetPropertyBrowser() {
    if (curveBindingWidget_) {
        curveBindingWidget_->clearCurveBinding();
    }
    if (meshWidget_) {
        meshWidget_->clearMesh();
    }
    if (customWidget_) {
        customWidget_->clearPropertyBrowserCustomWidget();
    }
}

void PropertyBrowserWidget::switchNode(std::string objectID) {
	NodeData* nodeData = NodeDataManager::GetInstance().searchNodeByID(objectID);
	NodeDataManager::GetInstance().setActiveNode(nodeData);
    if (curveBindingWidget_) {
        curveBindingWidget_->initCurveBindingWidget();
    }
    if (meshWidget_) {
		meshWidget_->initPropertyBrowserMeshWidget();
    }
    if (customWidget_) {
		customWidget_->initPropertyBrowserCustomWidget();
    }
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
				if (locked_) {
					setLocked(false);
				}
				clearValueHandle(true);
			}
		});
		propertyBrowser_.reset(new PropertyBrowserView{new PropertyBrowserItem{valueHandle, dispatcher_, commandInterface_, model_}, model_, this});
        layout_.addWidget(propertyBrowser_.get(), 1, 0);

        std::string objectID;
        std::string nodeName;
        if (getObjId(valueHandle, objectID, nodeName)) {
            switchNode(objectID);
        }
	} else {
		LOG_DEBUG(log_system::PROPERTY_BROWSER, "locked! ignore value handle set {}", valueHandle);
	}
}

PropertyBrowserModel* PropertyBrowserWidget::model() const {
	return model_;
}

void PropertyBrowserWidget::initPropertyBrowserWidget() {
    meshNodeWidget_ = new PropertyBrowserNodeWidget(QString("Mesh"), this);
    meshWidget_ = new PropertyBrowserMeshWidget(meshNodeWidget_);
    meshNodeWidget_->setShowWidget(meshWidget_);
    layout_.addWidget(meshNodeWidget_, 2, 0, Qt::AlignTop);

    customNodeWidget_ = new PropertyBrowserNodeWidget(QString("CustomProperty"), this, true);
    customWidget_ = new PropertyBrowserCustomWidget(customNodeWidget_);
    QObject::connect(customNodeWidget_, &PropertyBrowserNodeWidget::insertData, customWidget_, &PropertyBrowserCustomWidget::insertData);
    QObject::connect(customNodeWidget_, &PropertyBrowserNodeWidget::removeData, customWidget_, &PropertyBrowserCustomWidget::removeData);
    customNodeWidget_->setShowWidget(customWidget_);
    layout_.addWidget(customNodeWidget_, 3, 0, Qt::AlignTop);

    curveBindingNodeWidget_ = new PropertyBrowserNodeWidget(QString("CurveBinding"), this, true);
    curveBindingWidget_ = new PropertyBrowserCurveBindingWidget(curveBindingNodeWidget_);
    QObject::connect(curveBindingNodeWidget_, &PropertyBrowserNodeWidget::insertData, curveBindingWidget_, &PropertyBrowserCurveBindingWidget::insertData);
    QObject::connect(curveBindingNodeWidget_, &PropertyBrowserNodeWidget::removeData, curveBindingWidget_, &PropertyBrowserCurveBindingWidget::removeData);
    curveBindingNodeWidget_->setShowWidget(curveBindingWidget_);
    layout_.addWidget(curveBindingNodeWidget_, 4, 0, Qt::AlignTop);
}

void PropertyBrowserWidget::setValueHandles(const std::set<raco::core::ValueHandle>& valueHandles) {

    if (valueHandles.size() > 1) {
        clearValueHandle(false);
    } else {
        auto handle = *valueHandles.begin();
        setValueHandle(*valueHandles.begin());
    }
}

}  // namespace raco::property_browser
