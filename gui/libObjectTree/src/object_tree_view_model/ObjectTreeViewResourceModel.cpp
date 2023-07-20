/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewResourceModel.h"

#include "core/CommandInterface.h"
#include "core/Queries.h"
#include "core/Serialization.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/ExternalReferenceAnnotation.h"

namespace raco::object_tree::model {

ObjectTreeViewResourceModel::ObjectTreeViewResourceModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string>& allowedCreatableUserTypes)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectStore, allowedCreatableUserTypes, true, true) {}


bool ObjectTreeViewResourceModel::pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects) {
	// ignore index: resources always get pasted at top level.
	return ObjectTreeViewDefaultModel::pasteObjectAtIndex({}, pasteAsExtref, outError, serializedObjects);
}

bool ObjectTreeViewResourceModel::isObjectAllowedIntoIndex(const QModelIndex& index, const core::SEditorObject& obj) const {
	// Only allow root level pasting here, thus only invalid indices are ok.
	return !indexToSEditorObject(index) && ObjectTreeViewDefaultModel::isObjectAllowedIntoIndex(index, obj);
}

std::vector<std::string> ObjectTreeViewResourceModel::typesAllowedIntoIndex(const QModelIndex& index) const {
	auto topLevel = QModelIndex();
	// Always assume user wants to create item on top level.
	return ObjectTreeViewDefaultModel::typesAllowedIntoIndex(topLevel);
}

}  // namespace raco::object_tree::model