/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <QMap>
#include <QObject>
#include <unordered_set>

#include "core/EditorObject.h"

namespace raco::object_tree::view {

class ObjectTreeDock;
class ObjectTreeView;

class ObjectTreeDockManager : public QObject {
	Q_OBJECT
public:
	void addTreeDock(ObjectTreeDock* newDock);

	size_t getTreeDockAmount() const;
	std::vector<ObjectTreeDock*> getDocks() const;
	ObjectTreeDock* getActiveDockWithSelection() const;
	bool docksContainObject(const QString& objID) const;

Q_SIGNALS:
	void treeDockListChanged();
	void newObjectTreeItemsSelected(const std::set<raco::core::ValueHandle> &objects);
	void selectionCleared();

public Q_SLOTS:
	void eraseTreeDock(ObjectTreeDock* dockToErase);
	void setFocusedDock(ObjectTreeDock* dockToFocus);
	void selectObjectAcrossAllTreeDocks(const QString& objectID);

private:
	std::vector<ObjectTreeDock*> docks_;
	ObjectTreeDock* focusedDock_{nullptr};

	void connectTreeDockSignals(ObjectTreeDock* dock);
};

}  // namespace raco::object_tree::view
