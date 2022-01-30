/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#include "object_tree_view_model/ObjectTreeViewTopLevelSortProxyModel.h"
#include "object_tree_view_model/ObjectTreeNode.h"

namespace raco::object_tree::model {

bool ObjectTreeViewTopLevelSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {

	// The external reference grouping element should always be on top of the list
	if (static_cast<ObjectTreeNode*>(source_left.internalPointer())->getType() == ObjectTreeNodeType::ExtRefGroup) {
		return true;	
	} else if (static_cast<ObjectTreeNode*>(source_right.internalPointer())->getType() == ObjectTreeNodeType::ExtRefGroup) {
		return false;
	}

	// Only compare items that are on the same level and only sort items that are below the root node or the ExtRefGroup
	if (source_left.parent() == source_right.parent() && (!source_left.parent().isValid() || static_cast<ObjectTreeNode*>(source_left.parent().internalPointer())->getType() == ObjectTreeNodeType::ExtRefGroup)) {
		return QSortFilterProxyModel::lessThan(source_left, source_right);
	}

	return false;
}

}  // namespace raco::object_tree::model