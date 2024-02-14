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
#include "core/Errors.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/SceneBackendInterface.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/Utilities.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Texture.h"

#include "style/Icons.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>

namespace raco::property_browser {

using namespace style;

using SDataChangeDispatcher = components::SDataChangeDispatcher;

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
	lockButton_->setToolTip("Lock current property browser");
	lockButton_->connect(lockButton_, &QPushButton::clicked, this, [this]() {
		setLocked(!locked_);
	});

	refButton_ = new QPushButton{this};
	refButton_->setContentsMargins(0, 0, 0, 0);
	refButton_->setFlat(true);
	refButton_->setIcon(Icons::instance().goToLeft);
	refButton_->setToolTip("Show objects referencing current object");
	connect(refButton_, &QPushButton::clicked, this, &PropertyBrowserWidget::showRefToThis);

	prefabLookupButton_ = new QPushButton{this};
	prefabLookupButton_->setContentsMargins(0, 0, 0, 0);
	prefabLookupButton_->setFlat(true);
	prefabLookupButton_->setIcon(Icons::instance().prefabLookup);
	prefabLookupButton_->setToolTip("Show object in prefab");
	prefabLookupButton_->setEnabled(false);
	connect(prefabLookupButton_, &QPushButton::clicked, this, [this]() {
		rootItem_->model()->Q_EMIT selectionRequested(QString::fromStdString(getObjectIdInPrefab()));
	});

	const auto buttonsLayout = new PropertyBrowserHBoxLayout(this);
	buttonsLayout->addWidget(lockButton_, 0, Qt::AlignLeft);
	buttonsLayout->addWidget(refButton_, 0, Qt::AlignLeft);
	buttonsLayout->addWidget(prefabLookupButton_, 0, Qt::AlignLeft);

	layout_.addLayout(buttonsLayout, 0, 0, Qt::AlignLeft);
	layout_.addWidget(emptyLabel_, 1, 0, Qt::AlignCenter);
	layout_.setColumnStretch(0, 1);
	layout_.setRowStretch(1, 1);
	layout_.setContentsMargins(0, 0, 5, 0);

	lifecycleSubs_ = dispatcher_->registerOnObjectsLifeCycle([](auto) {}, [this](core::SEditorObject obj) {
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

void PropertyBrowserWidget::showRefToThis() {
	QMenu refMenu;
	if (!currentObjects_.empty() && *currentObjects_.begin() && rootItem_ && currentObjects_.size() == 1) {
		// get list of ref objects sorted by name
		std::vector<std::pair<QString, QString>> refObjects;

		for (const auto& refObject : (*currentObjects_.begin())->referencesToThis()) {
			const auto refObjectName = QString::fromStdString(core::Queries::getFullObjectHierarchyPath(refObject.lock()));
			const auto refObjectID = QString::fromStdString(refObject.lock()->objectID());
			refObjects.emplace_back(refObjectName, refObjectID);
		}
		std::sort(refObjects.begin(), refObjects.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.first < rhs.first;
		});

		// add ref menu items
		for (const auto& [refObjectName, refObjectID] : refObjects) {
			const auto id = refObjectID;
			refMenu.addAction(refObjectName, [this, id]() {
				rootItem_->model()->Q_EMIT selectionRequested(id);
			});
		}
	}

	if (refMenu.isEmpty()) {
		refMenu.addAction("No references", []() {});
	}

	refMenu.exec(mapToGlobal(refButton_->pos() + QPoint(refButton_->width(), 0)));
}

std::string PropertyBrowserWidget::getObjectIdInPrefab() const {
	if (rootItem_ && currentObjects_.size() == 1) {
		const auto selectedObject = *currentObjects_.begin();
		if (const auto prefabInstance = core::PrefabOperations::findContainingPrefabInstance(selectedObject)) {
			if(const auto prefab = *prefabInstance->template_) {
				return user_types::PrefabInstance::mapObjectIDFromInstance(selectedObject, prefab, prefabInstance);
			}
		}
	}
	return std::string{};
}

void PropertyBrowserWidget::setLockable(bool lockable) const {
	lockButton_->setVisible(lockable);
	refButton_->setVisible(lockable);
}

void PropertyBrowserWidget::clear() {
	if (!locked_) {
		propertyBrowser_.reset();
		currentObjects_.clear();
		emptyLabel_->setVisible(true);
		showScrollBar(false);
	}
}

void PropertyBrowserWidget::setLocked(bool locked) {
	const bool isLockChanged = locked_ != locked;
	locked_ = locked;
	lockButton_->setIcon(locked_ ? Icons::instance().locked : Icons::instance().unlocked);
	lockButton_->setToolTip(locked_ ? "Unlock current property browser" : "Lock current property browser");
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

void PropertyBrowserWidget::setObjectFromObjectId(const QString& objectID, const QString& objectProperty) {
	const auto handle = core::ValueHandle{commandInterface_->project()->getInstanceByID(objectID.toStdString())};
	if (handle) {
		setObjects({handle.rootObject()}, objectProperty);
	}
}

PropertyBrowserModel* PropertyBrowserWidget::model() const {
	return model_;
}


void PropertyBrowserWidget::showScrollBar(bool isAlwaysOn) {
	auto scrollArea = findAncestor<QAbstractScrollArea>(parentWidget());
	if (scrollArea) {
		scrollArea->setVerticalScrollBarPolicy(isAlwaysOn ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded);
	}
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
		rootItem_ = new PropertyBrowserItem{valueHandles, dispatcher_, commandInterface_, model_, sceneBackend_};

		showScrollBar(objects.size() == 1 && (*objects.begin())->isType<user_types::Texture>());

		propertyBrowser_.reset(new PropertySubtreeView{sceneBackend_, model_, rootItem_, this, nullptr});
		rootItem_->setParent(propertyBrowser_.get());
		propertyBrowser_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

		currentObjects_ = objects;
		layout_.addWidget(propertyBrowser_.get(), 1, 0);
		prefabLookupButton_->setEnabled(!getObjectIdInPrefab().empty());
	}
}

void PropertyBrowserWidget::setObjects(const core::SEditorObjectSet& objects, const QString& property) {
	setObjectsImpl(objects, false);

	if ((locked_ && currentObjects_ == objects) || !locked_) {
		highlightProperty(property);
	}
}

void PropertyBrowserWidget::highlightProperty(const QString& property) {
	if (!property.isEmpty()) {
		// We need to call updateGeometry otherwise the scrolling does not work correctly
		updateGeometry();

		rootItem_->highlightProperty(property);
	}
}


}  // namespace raco::property_browser
