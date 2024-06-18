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

using namespace user_types;

ObjectTreeViewPrefabModel::ObjectTreeViewPrefabModel(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore,
		  std::vector<std::string>{
			  Prefab::typeDescription.typeName},
		  true) {
}

QVariant ObjectTreeViewPrefabModel::data(const QModelIndex& index, int role) const {
	auto treeNode = indexToTreeNode(index);

	if (role == Qt::ItemDataRole::DecorationRole && index.column() == COLUMNINDEX_NAME) {
		auto editorObj = indexToSEditorObject(index);
		if (editorObj && editorObj->query<core::ExternalReferenceAnnotation>() && editorObj->as<user_types::Prefab>()) {
			return QVariant(typeIconMap.at("ExtrefPrefab"));
		}
	}

	return ObjectTreeViewDefaultModel::data(index, role);
}

std::vector<ObjectTreeViewDefaultModel::ColumnIndex> ObjectTreeViewPrefabModel::hiddenColumns() const {
	return {COLUMNINDEX_ABSTRACT_VIEW_VISIBILITY, COLUMNINDEX_RENDER_ORDER, COLUMNINDEX_INPUT_BUFFERS, COLUMNINDEX_OUTPUT_BUFFERS};
}

}  // namespace raco::object_tree::model