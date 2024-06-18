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

#include "user_types/AnchorPoint.h"
#include "user_types/AnimationChannel.h"
#include "user_types/AnimationChannelRaco.h"
#include "user_types/BlitPass.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"
#include "user_types/RenderTarget.h"
#include "user_types/Texture.h"
#include "user_types/TextureExternal.h"
#include "user_types/Timer.h"

namespace raco::object_tree::model {

using namespace user_types;

ObjectTreeViewResourceModel::ObjectTreeViewResourceModel(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectStore,
		  std::vector<std::string>{
			  AnchorPoint::typeDescription.typeName,
			  AnimationChannel::typeDescription.typeName,
			  AnimationChannelRaco::typeDescription.typeName,
			  BlitPass::typeDescription.typeName,
			  CubeMap::typeDescription.typeName,
			  LuaScriptModule::typeDescription.typeName,
			  Material::typeDescription.typeName,
			  Mesh::typeDescription.typeName,
			  RenderBuffer::typeDescription.typeName,
			  RenderBufferMS::typeDescription.typeName,
			  RenderLayer::typeDescription.typeName,
			  RenderPass::typeDescription.typeName,
			  RenderTarget::typeDescription.typeName,
			  RenderTargetMS::typeDescription.typeName,
			  Texture::typeDescription.typeName,
			  TextureExternal::typeDescription.typeName,
			  Timer::typeDescription.typeName},
		  true, true) {
	setAcceptableFileExtensions(QStringList{"gltf", "glb", "ctm", "png", "vert", "frag", "geom", "def", "glsl", "lua"});
	setAcceptLuaModules(true);
}

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

std::vector<ObjectTreeViewDefaultModel::ColumnIndex> ObjectTreeViewResourceModel::hiddenColumns() const {
	return {COLUMNINDEX_PREVIEW_VISIBILITY, COLUMNINDEX_ABSTRACT_VIEW_VISIBILITY, COLUMNINDEX_RENDER_ORDER, COLUMNINDEX_INPUT_BUFFERS, COLUMNINDEX_OUTPUT_BUFFERS};
}

ObjectTreeViewDefaultModel::ColumnIndex ObjectTreeViewResourceModel::defaultSortColumn() const {
	return COLUMNINDEX_NAME;
}

}  // namespace raco::object_tree::model