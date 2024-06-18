/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewSceneGraphModel.h"

#include "core/ExternalReferenceAnnotation.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "style/Icons.h"

#include "user_types/Animation.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Skin.h"

namespace raco::object_tree::model {

using namespace user_types;

ObjectTreeViewSceneGraphModel::ObjectTreeViewSceneGraphModel(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore,
		  std::vector<std::string>{
			  Animation::typeDescription.typeName,
			  LuaInterface::typeDescription.typeName,
			  LuaScript::typeDescription.typeName,
			  MeshNode::typeDescription.typeName,
			  Node::typeDescription.typeName,
			  OrthographicCamera::typeDescription.typeName,
			  PerspectiveCamera::typeDescription.typeName,
			  PrefabInstance::typeDescription.typeName,
			  Skin::typeDescription.typeName},
		  true) {
	setAcceptableFileExtensions(QStringList{"lua", "gltf", "glb"});
	setAcceptLuaScripts(true);
	setAcceptLuaInterfaces(true);
	setDropGltfOpensAssetImportDialog(true);
}

}  // namespace raco::object_tree::model