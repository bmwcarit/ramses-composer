/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include "object_tree_view/ObjectTreeView.h"
#include "components/DebugInstanceCounter.h"
#include "core/ErrorItem.h"

#include <QComboBox>
#include <DockWidget.h>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <memory>
#include <QLineEdit>

namespace raco::object_tree::view {

class ObjectTreeDock : public ads::CDockWidget {
	Q_OBJECT
	DEBUG_INSTANCE_COUNTER(ObjectTreeDock);
public:
	explicit ObjectTreeDock(const char *dockTitle, QWidget *parent = nullptr);
	~ObjectTreeDock();

	void setTreeView(ObjectTreeView *treeView);
	ObjectTreeView *getActiveTreeView() const;

public Q_SLOTS:
	void resetSelection();

Q_SIGNALS:
	void externalObjectSelected(ObjectTreeDock *srcDock);
	void newObjectTreeItemsSelected(const core::SEditorObjectSet &objects, ObjectTreeDock *srcDock, const QString &property);
	void dockClosed(ObjectTreeDock *closedDock);
	void dockSelectionFocusRequested(ObjectTreeDock *focusDock);

private:
	void filterTreeViewObjects();
	void setLineEditErrorLevel(FilterResult filterResult);

	QWidget *treeDockContent_;
	QVBoxLayout *treeDockLayout_;

	QLineEdit *filterLineEdit_;
	QComboBox *filterByComboBox_;
	QHBoxLayout *treeDockSettingsLayout_;

	QStackedWidget *treeViewStack_;

	std::shared_ptr<core::BaseContext> currentContext_;
	
};

}