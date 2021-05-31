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

namespace raco::object_tree::model {

bool ObjectTreeViewTopLevelSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
	if (!source_left.parent().isValid() || !source_right.parent().isValid()) {
		return QSortFilterProxyModel::lessThan(source_left, source_right);
	}

	return false;
}

}  // namespace raco::object_tree::model