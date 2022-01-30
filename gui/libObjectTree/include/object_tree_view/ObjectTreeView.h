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

#include "core/EditorObject.h"

#include <QAbstractItemModel>
#include <QTreeView>
#include <unordered_set>

class QSortFilterProxyModel;

namespace raco::object_tree::model {
class ObjectTreeViewDefaultModel;
}

namespace raco::object_tree::view {

class ObjectTreeView : public QTreeView {
	Q_OBJECT

	using ValueHandle = core::ValueHandle;
	using EditorObject = core::EditorObject;
	using SEditorObject = core::SEditorObject;

public:
	ObjectTreeView(const QString &viewTitle, raco::object_tree::model::ObjectTreeViewDefaultModel *viewModel, QSortFilterProxyModel *sortFilterProxyModel = nullptr, QWidget *parent = nullptr);

	std::set<ValueHandle> getSelectedHandles() const;
	QString getViewTitle() const;

	void requestNewNode(EditorObject::TypeDescriptor nodeType, const std::string &nodeName, const QModelIndex &parent);
	void showContextMenu(const QPoint &p);
	bool canCopyAtIndices(const QModelIndexList &indices);
	bool canPasteIntoIndex(const QModelIndex &index, bool asExtref);

	QSortFilterProxyModel *proxyModel() const;

	QModelIndexList getSelectedIndices(bool sorted = false) const;
	QModelIndex getSelectedInsertionTargetIndex() const;
	void collapseRecusively(const QModelIndex &index);

Q_SIGNALS:
	void dockSelectionFocusRequested(ObjectTreeView *focusTree);
	void newNodeRequested(EditorObject::TypeDescriptor nodeType, const std::string &nodeName, const QModelIndex &parent);
	void newObjectTreeItemsSelected(const std::set<ValueHandle> &handles);
	void externalObjectSelected();

public Q_SLOTS:
	void resetSelection();
	void globalCopyCallback();
	void cut();
	void globalPasteCallback(const QModelIndex &index, bool asExtRef = false);
	void shortcutDelete();
	void selectObject(const QString &objectID);
	void expandAllParentsOfObject(const QString &objectID);
	void expanded(const QModelIndex &index);
	void collapsed(const QModelIndex &index);
	
protected:
	static inline auto SELECTION_MODE = QItemSelectionModel::Select | QItemSelectionModel::Rows;

	raco::object_tree::model::ObjectTreeViewDefaultModel *treeModel_;
	QSortFilterProxyModel *proxyModel_;
	QString viewTitle_;
	std::unordered_set<std::string> expandedItemIDs_;
	std::unordered_set<std::string> selectedItemIDs_;

	virtual QMenu* createCustomContextMenu(const QPoint &p);

	void dragMoveEvent(QDragMoveEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

	std::vector<SEditorObject> indicesToSEditorObjects(const QModelIndexList &index) const;
	std::string indexToTreeNodeID(const QModelIndex &index) const;
	QModelIndex indexFromTreeNodeID(const std::string &id) const;

protected Q_SLOTS:
	void restoreItemExpansionStates();
	void restoreItemSelectionStates();
	void expandAllParentsOfObject(const QModelIndex &index);
};

}