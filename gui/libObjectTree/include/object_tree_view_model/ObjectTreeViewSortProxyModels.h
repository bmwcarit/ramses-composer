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

#include "ObjectTreeNode.h"
#include <QSortFilterProxyModel>

namespace raco::object_tree::model {

class ObjectTreeViewDefaultSortFilterProxyModel : public QSortFilterProxyModel {
	std::function<bool(const ObjectTreeNode&)> customFilter = nullptr;

public:
	ObjectTreeViewDefaultSortFilterProxyModel(QObject *parent = nullptr, bool enableSorting = true);

	using FilterFunction = std::function<bool(const QAbstractItemModel*, int, const QModelIndex&)>;

	bool sortingEnabled() const;
	
	void setCustomFilter(std::function<bool(const ObjectTreeNode&)> filterFunc);
	void removeCustomFilter();

	QString getDataAtIndex(const QModelIndex &index) const;

protected:

	QVariant data(const QModelIndex &index, int role) const override;
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

	bool sortingEnabled_;
};

// This class sorts External References always to the top. TypeParent nodes will always be sorted alphabetically in descending order. All other nodes will be sorted regularly.
class ObjectTreeViewResourceSortFilterProxyModel : public ObjectTreeViewDefaultSortFilterProxyModel {
public:
	ObjectTreeViewResourceSortFilterProxyModel(QObject *parent = nullptr) : ObjectTreeViewDefaultSortFilterProxyModel(parent, true) {}

protected:
	bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

};

// This class only sorts top-level nodes (nodes under the invisible root node in ObjectTreeView models) while keeping the scenegraph structure of child nodes.
class ObjectTreeViewTopLevelSortFilterProxyModel : public ObjectTreeViewDefaultSortFilterProxyModel {
public:
	ObjectTreeViewTopLevelSortFilterProxyModel(QObject *parent = nullptr) : ObjectTreeViewDefaultSortFilterProxyModel(parent, true) {}

protected:
  
	bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

};

}  // namespace raco::object_tree::model