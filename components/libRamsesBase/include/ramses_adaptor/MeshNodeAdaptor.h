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

#include "ramses_adaptor/MaterialAdaptor.h"
#include "ramses_adaptor/MeshAdaptor.h"
#include "ramses_adaptor/NodeAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include <array>
#include <ramses-client-api/MeshNode.h>
#include <ramses-logic/RamsesAppearanceBinding.h>

namespace raco::ramses_adaptor {

class SceneAdaptor;

class MeshNodeAdaptor final : public SpatialAdaptor<user_types::MeshNode, ramses::MeshNode>, public IRenderGroupObject {
public:
	using UniqueRamsesAppearanceBinding = std::unique_ptr<rlogic::RamsesAppearanceBinding, std::function<void(rlogic::RamsesAppearanceBinding*)>>;
	using BaseAdaptor = SpatialAdaptor<user_types::MeshNode, ramses::MeshNode>;

	explicit MeshNodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SMeshNode node);
	~MeshNodeAdaptor();

	user_types::SMesh mesh();
	/**
	 * @returns the associated MeshAdaptor if it exists and is valid (e.g. a mesh is set)
	 */
	MeshAdaptor* meshAdaptor();
	user_types::SMaterial material(size_t index);
	MaterialAdaptor* materialAdaptor(size_t index);

	bool sync(core::Errors* errors) override;
	void syncMaterials();
	void syncMeshObject();

	void addObjectToRenderGroup(ramses::RenderGroup& renderGroup, int orderWithinGroup) override;

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property& getProperty(const std::vector<std::string>& propertyNamesVector) override;

private:
	void syncMaterial(size_t index);

	void setupMaterialSubscription();
	void setupUniformChildrenSubscription();

	raco::ramses_base::RamsesAppearance currentAppearance_;
	UniqueRamsesAppearanceBinding appearanceBinding_;
	raco::ramses_base::RamsesEffect currentEffect_;
	std::vector<raco::ramses_base::RamsesArrayResource> currentMeshVertexData_;
	raco::ramses_base::RamsesArrayResource currentMeshIndices_;
	raco::ramses_base::RamsesGeometryBinding geometryBinding_;
	std::vector<raco::ramses_base::RamsesTextureSampler> currentSamplers_;

	// Subscriptions
	components::Subscription meshSubscription_;
	components::Subscription materialsSubscription_;
	components::Subscription materialSubscription_;
	components::Subscription optionsChildrenSubscription_;
	components::Subscription instanceCountSubscription_;
	components::Subscription uniformSubscription_;
	components::Subscription uniformChildrenSubscription_;
};

};	// namespace raco::ramses_adaptor
