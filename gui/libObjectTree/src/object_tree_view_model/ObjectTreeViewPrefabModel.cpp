/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "user_types/Prefab.h"
#include "style/Icons.h"

namespace raco::object_tree::model {

ObjectTreeViewPrefabModel::ObjectTreeViewPrefabModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore, allowedCreatableUserTypes, true) {
}

QVariant ObjectTreeViewPrefabModel::data(const QModelIndex& index, int role) const {
	auto treeNode = indexToTreeNode(index);

	if (role == Qt::ItemDataRole::DecorationRole && index.column() == COLUMNINDEX_NAME) {
		auto editorObj = indexToSEditorObject(index);
		if (editorObj && editorObj->query<raco::core::ExternalReferenceAnnotation>() && editorObj->as<user_types::Prefab>()) {
			return QVariant(typeIconMap.at("ExtrefPrefab"));
		}
	}

	return ObjectTreeViewDefaultModel::data(index, role);
}

}  // namespace raco::object_tree::model