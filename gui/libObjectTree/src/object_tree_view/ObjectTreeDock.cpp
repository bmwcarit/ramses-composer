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

#include "common_widgets/NoContentMarginsLayout.h"
#include "core/Context.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view/ObjectTreeView.h"

#include <QModelIndexList>

namespace raco::object_tree::view {

ObjectTreeDock::ObjectTreeDock(const char *dockTitle, QWidget *parent)
	: CDockWidget(dockTitle, parent), treeViewStack_(nullptr) {
	treeDockContent_ = new QWidget(parent);
	setWidget(treeDockContent_);
	setAttribute(Qt::WA_DeleteOnClose);
	setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);

	treeDockLayout_ = new raco::common_widgets::NoContentMarginsLayout<QVBoxLayout>(treeDockContent_);
	treeDockContent_->setLayout(treeDockLayout_);

	availableTreesComboBox_ = new QComboBox(treeDockContent_);
	availableTreesComboBox_->setVisible(false);
	treeDockLayout_->addWidget(availableTreesComboBox_);
	treeDockSettingsLayout_ = new raco::common_widgets::NoContentMarginsLayout<QVBoxLayout>(treeDockContent_);
	treeDockLayout_->addLayout(treeDockSettingsLayout_);

	treeViewStack_ = new QStackedWidget(treeDockContent_);
	treeDockLayout_->addWidget(treeViewStack_);

	connect(availableTreesComboBox_, &QComboBox::currentTextChanged, this, &ObjectTreeDock::selectTreeView);
}

raco::object_tree::view::ObjectTreeDock::~ObjectTreeDock() {
	Q_EMIT dockClosed(this);
}

void ObjectTreeDock::addTreeView(ObjectTreeView *treeView) {
	const auto treeViewTitle = treeView->getViewTitle();

	assert(!savedTreeViews_.contains(treeViewTitle));

	connect(treeView, &ObjectTreeView::newObjectTreeItemsSelected, [this](const auto &handles) {
		Q_EMIT newObjectTreeItemsSelected(handles, this);
	});
	connect(treeView, &ObjectTreeView::dockSelectionFocusRequested, [this]() {
		Q_EMIT dockSelectionFocusRequested(this);
	});

	savedTreeViews_.insert(treeViewTitle, treeView);
	treeViewStack_->addWidget(savedTreeViews_[treeViewTitle]);
	availableTreesComboBox_->setVisible(savedTreeViews_.size() > 1);

	availableTreesComboBox_->addItem(treeViewTitle);
}

ObjectTreeView *raco::object_tree::view::ObjectTreeDock::getCurrentlyActiveTreeView() const {
	return dynamic_cast<ObjectTreeView *>(treeViewStack_->currentWidget());
}

void raco::object_tree::view::ObjectTreeDock::resetSelection() {
	for (auto tree : savedTreeViews_) {
		tree->resetSelection();
	}
}

void ObjectTreeDock::selectTreeView(const QString &treeViewTitle) {
	treeViewStack_->setCurrentWidget(savedTreeViews_[treeViewTitle]);

	auto currentTreeView = static_cast<ObjectTreeView *>(treeViewStack_->currentWidget());
	if (currentTreeView && currentTreeView->selectionModel()->hasSelection()) {
		Q_EMIT newObjectTreeItemsSelected(currentTreeView->getSelectedHandles(), this);
	}
}

}  // namespace raco::object_tree::view
