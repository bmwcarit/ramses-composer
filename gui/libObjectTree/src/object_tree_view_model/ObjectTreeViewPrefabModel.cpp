/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

namespace raco::object_tree::model {

ObjectTreeViewPrefabModel::ObjectTreeViewPrefabModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore, allowedCreatableUserTypes) {
}

std::vector<std::string> ObjectTreeViewPrefabModel::typesAllowedIntoIndex(const QModelIndex& index) const {

	auto prefabType = raco::user_types::Prefab::typeDescription.typeName;
	if (index.isValid()) {
		auto types = ObjectTreeViewDefaultModel::typesAllowedIntoIndex(index);

		auto prefabIndex = std::find(types.begin(), types.end(), prefabType);
		if (prefabIndex != types.end()) {
			types.erase(std::find(types.begin(), types.end(), prefabType));
		}

		return types;
	} else {
		return {prefabType};	
	}
}

bool ObjectTreeViewPrefabModel::isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const {
	if (index.isValid()) {

		if (auto parentObj = indexToSEditorObject(index)) {
			if (parentObj->query<core::ExternalReferenceAnnotation>() || raco::core::PrefabOperations::findContainingPrefabInstance(parentObj)) {
				return false;
			}
		}

		return ObjectTreeViewDefaultModel::isObjectAllowedIntoIndex(index, obj);
	} else {
		if (obj->as<user_types::Prefab>()) {
			return true;
		} else {
			return false;
		}
	}
}



}  // namespace raco::object_tree::model