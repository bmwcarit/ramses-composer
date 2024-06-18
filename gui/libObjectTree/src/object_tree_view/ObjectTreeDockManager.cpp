/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeDockManager.h"

#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"

namespace raco::object_tree::view {

void ObjectTreeDockManager::addTreeDock(ObjectTreeDock* newDock) {
	connectTreeDockSignals(newDock);

	docks_.emplace_back(newDock);

	Q_EMIT treeDockListChanged();
}

void ObjectTreeDockManager::eraseTreeDock(ObjectTreeDock* dockToErase) {
	auto dockPosition = std::find(docks_.begin(), docks_.end(), dockToErase);
	if (dockPosition == docks_.end()) {
		return;
	}

	docks_.erase(dockPosition);
	if (focusedDock_ == dockToErase) {
		focusedDock_ = nullptr;
		Q_EMIT newObjectTreeItemsSelected({}, {});
	}

	Q_EMIT treeDockListChanged();
}

void ObjectTreeDockManager::setFocusedDock(ObjectTreeDock* dock) {
	for (auto savedDock : docks_) {
		if (savedDock != dock) {
			savedDock->resetSelection();
		}
	}
	focusedDock_ = dock;
}

void ObjectTreeDockManager::selectObjectAndPropertyAcrossAllTreeDocks(core::SEditorObject object, const QString& propertyPath) {
	ObjectTreeDock* focusDock = nullptr;
	auto objectID = QString::fromStdString(object->objectID());
	for (auto* dock : getDocks()) {
		if (auto activeTreeView = dock->getActiveTreeView()) {
			if (activeTreeView->canProgrammaticallyGoToObject() && dock->getActiveTreeView()->containsObject(objectID)) {
				activeTreeView->selectObject(objectID);
				activeTreeView->expandAllParentsOfObject(objectID);
				focusDock = dock;
				break;
			}
		}
	}
	setFocusedDock(focusDock);
	Q_EMIT newObjectTreeItemsSelected({object}, propertyPath);
}

size_t ObjectTreeDockManager::getTreeDockAmount() const {
	return docks_.size();
}

std::vector<ObjectTreeDock*> ObjectTreeDockManager::getDocks() const {
	return docks_;
}

ObjectTreeDock* ObjectTreeDockManager::getActiveDockWithSelection() const {
	// only return the active dock when it actually contains a selection
	if (focusedDock_ && !focusedDock_->getActiveTreeView()->selectionModel()->hasSelection()) {
		return nullptr;
	}
	return focusedDock_;
}

bool ObjectTreeDockManager::docksContainObject(const QString& objID) const {
	for (auto* dock : docks_) {
		if (dock->getActiveTreeView()->containsObject(objID)) {
			return true;
		}
	}

	return false;
}

bool ObjectTreeDockManager::isExternalProjectDock(ObjectTreeDock* dock) const {
	return dynamic_cast<model::ObjectTreeViewExternalProjectModel*>(dock->getActiveTreeView()->treeModel()) != nullptr;
}

std::vector<core::SEditorObject> ObjectTreeDockManager::getSelection() const {
	auto activeDockWhichHasSelection = getActiveDockWithSelection();
	if (activeDockWhichHasSelection == nullptr || isExternalProjectDock(activeDockWhichHasSelection)) {
		return std::vector<core::SEditorObject>();
	}

	return activeDockWhichHasSelection->getActiveTreeView()->getSortedSelectedEditorObjects();
}

void ObjectTreeDockManager::connectTreeDockSignals(ObjectTreeDock* dock) {
	QObject::connect(dock, &ObjectTreeDock::newObjectTreeItemsSelected, [this, dock](const core::SEditorObjectSet& objects) {
		setFocusedDock(dock);
		if (objects.empty() || isExternalProjectDock(dock)) {
			Q_EMIT newObjectTreeItemsSelected({}, {});
		} else {
			Q_EMIT newObjectTreeItemsSelected(objects, {});
		}
	});
	QObject::connect(dock, &ObjectTreeDock::dockClosed, this, [this, dock]() {
		eraseTreeDock(dock);
	});
}

}  // namespace raco::object_tree::view