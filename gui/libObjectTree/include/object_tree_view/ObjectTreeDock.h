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
#include "object_tree_view/ObjectTreeView.h"
#include "components/DebugInstanceCounter.h"

#include <QComboBox>
#include <DockWidget.h>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <memory>
#include <unordered_set>

namespace raco::object_tree::view {

class ObjectTreeDock : public ads::CDockWidget {
	Q_OBJECT
	DEBUG_INSTANCE_COUNTER(ObjectTreeDock);
public:
	explicit ObjectTreeDock(const char *dockTitle, QWidget *parent = nullptr);
	~ObjectTreeDock();

	void addTreeView(ObjectTreeView *treeView);
	ObjectTreeView *getCurrentlyActiveTreeView() const;
    ObjectTreeView *getSceneTreeView() const;

public Q_SLOTS:
	void selectTreeView(const QString &treeViewTitle);
    void resetSelection();

Q_SIGNALS:
	void externalObjectSelected(ObjectTreeDock *srcDock);
	void newObjectTreeItemsSelected(const std::set<raco::core::ValueHandle> &objects, ObjectTreeDock *srcDock);
	void dockClosed(ObjectTreeDock *closedDock);
	void dockSelectionFocusRequested(ObjectTreeDock *focusDock);
    void selectObject(const QString& objectId);

private:
	QWidget *treeDockContent_;
	QVBoxLayout *treeDockLayout_;
	
	QComboBox *availableTreesComboBox_;
	QVBoxLayout *treeDockSettingsLayout_;

	QHash<QString, ObjectTreeView*> savedTreeViews_;
	QStackedWidget *treeViewStack_;

	std::shared_ptr<raco::core::BaseContext> currentContext_;
	
};

}
