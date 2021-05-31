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

#include <QSortFilterProxyModel>

namespace raco::object_tree::model {

// This class only sorts top-level nodes (nodes under the invisible root node in ObjectTreeView models) while keeping the scenegraph structure of child nodes.
class ObjectTreeViewTopLevelSortFilterProxyModel : public QSortFilterProxyModel {
protected:
  
	bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};

}  // namespace raco::object_tree::model