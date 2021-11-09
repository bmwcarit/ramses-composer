/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewResourceModel.h"

#include "core/CommandInterface.h"
#include "core/Queries.h"
#include "core/Serialization.h"
#include "user_types/UserObjectFactory.h"

namespace raco::object_tree::model {

ObjectTreeViewResourceModel::ObjectTreeViewResourceModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectStore, allowedCreatableUserTypes){}

bool ObjectTreeViewResourceModel::pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects) {
	// ignore index: resources always get pasted at top level.
	return ObjectTreeViewDefaultModel::pasteObjectAtIndex(getInvisibleRootIndex(), pasteAsExtref, outError, serializedObjects);
}

bool ObjectTreeViewResourceModel::objectsAreAllowedInModel(const std::vector<core::SEditorObject>& objs, const QModelIndex& parentIndex) const {
	for (const auto& obj : objs) {
		if (!obj || std::find(allowedUserCreatableUserTypes_.begin(), allowedUserCreatableUserTypes_.end(), obj->getTypeDescription().typeName) == allowedUserCreatableUserTypes_.end()) {
			return false;
		}
	}
	return true;
}

bool ObjectTreeViewResourceModel::canInsertMeshAssets(const QModelIndex& index) const {
	return false;
}

bool ObjectTreeViewResourceModel::canPasteInto(const QModelIndex& index, const std::string& serializedObjs, bool asExtRef) const {
	auto deserialization{raco::serialization::deserializeObjects(serializedObjs,
		raco::user_types::UserObjectFactoryInterface::deserializationFactory(commandInterface_->objectFactory()))};
	auto topLevelObjects = core::BaseContext::getTopLevelObjectsFromDeserializedObjects(deserialization, commandInterface_->objectFactory(), project());

	if (asExtRef) {
		for (const auto& topLevelObj : topLevelObjects) {
			if (!core::Queries::canPasteObjectAsExternalReference(topLevelObj, deserialization.rootObjectIDs.find(topLevelObj->objectID()) != deserialization.rootObjectIDs.end())) {
				return false;
			}
		}
	}

	return objectsAreAllowedInModel(topLevelObjects, index);
}

}  // namespace raco::object_tree::model