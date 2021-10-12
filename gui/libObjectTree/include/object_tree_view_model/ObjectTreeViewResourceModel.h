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

#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"

namespace raco::object_tree::model {

class ObjectTreeViewResourceModel : public ObjectTreeViewDefaultModel {
	DEBUG_INSTANCE_COUNTER(ObjectTreeViewResourceModel);
	Q_OBJECT

public:
	ObjectTreeViewResourceModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore, const std::vector<std::string>& allowedCreatableUserTypes = {});

	bool pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref, std::string* outError, const std::string& serializedObjects = RaCoClipboard::get()) override;
	bool objectsAreAllowedInModel(const std::vector<core::SEditorObject>& objs, const QModelIndex& parentIndex) const override;
	bool canInsertMeshAssets(const QModelIndex& index) const override;
	bool canPasteInto(const QModelIndex& index, const std::string& serializedObjs, bool asExtRef) const override;

};

}  // namespace raco::object_tree::model