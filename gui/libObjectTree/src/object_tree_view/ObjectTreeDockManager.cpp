/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeDockManager.h"

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
	if (dockWithSelection_ == dockToErase) {
		dockWithSelection_ = nullptr;
		Q_EMIT selectionCleared();
	}

	Q_EMIT treeDockListChanged();
}

void ObjectTreeDockManager::setFocusedDock(ObjectTreeDock* dock) {
	for (auto savedDock : docks_) {
		if (savedDock != dock) {
			savedDock->resetSelection();
		} else {
			dockWithSelection_ = dock;
		}
	}
}

void ObjectTreeDockManager::selectObjectAcrossAllTreeDocks(const QString& objectID) {
	// always favor the first created active tree view of any type (e.g. the first "Scene Graph", the first "Resources")
	std::set<QString> viewTitles;

	for (const auto* dock : getDocks()) {
		if (auto activeTreeView = dock->getCurrentlyActiveTreeView()) {
			auto viewTitle = activeTreeView->getViewTitle();

			if (viewTitles.count(viewTitle) == 0) {
				activeTreeView->selectObject(objectID);
				activeTreeView->expandAllParentsOfObject(objectID);
				viewTitles.insert(viewTitle);
			}
		}
	}
}

size_t ObjectTreeDockManager::getTreeDockAmount() const {
	return docks_.size();
}

std::vector<ObjectTreeDock*> ObjectTreeDockManager::getDocks() const {
	return docks_;
}

void ObjectTreeDockManager::connectTreeDockSignals(ObjectTreeDock* dock) {
	QObject::connect(dock, &ObjectTreeDock::newObjectTreeItemsSelected, [this](auto& objects, auto* selectionSrcDock) {
		setFocusedDock(selectionSrcDock);
		if (objects.empty()) {
			Q_EMIT selectionCleared();
		} else {
			Q_EMIT newObjectTreeItemsSelected(objects);
		}
	});
	QObject::connect(dock, &ObjectTreeDock::dockSelectionFocusRequested, this, &ObjectTreeDockManager::setFocusedDock);

	QObject::connect(dock, &ObjectTreeDock::dockClosed, this, &ObjectTreeDockManager::eraseTreeDock);
}

}  // namespace raco::object_tree::view