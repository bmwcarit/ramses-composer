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
#include "core/Queries.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"


namespace raco::object_tree::model {

ObjectTreeViewPrefabModel::ObjectTreeViewPrefabModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore, allowedCreatableUserTypes) {
}

std::vector<std::string> ObjectTreeViewPrefabModel::allowedCreatableUserTypes(const QModelIndexList& selectedIndexes) const {
	if (!selectedIndexes.isEmpty()) {
		auto selectedIndex = selectedIndexes.front();
		if (selectedIndex.isValid()) {
			return allowedUserCreatableUserTypes_;
		}
	}

	return {raco::user_types::Prefab::typeDescription.typeName};
}

}  // namespace raco::object_tree::model