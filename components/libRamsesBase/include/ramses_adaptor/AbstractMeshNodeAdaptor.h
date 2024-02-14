/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_adaptor/AbstractMeshAdaptor.h"
#include "ramses_adaptor/AbstractNodeAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include <array>
#include <ramses/client/MeshNode.h>


namespace raco::ramses_adaptor {

class SceneAdaptor;

class AbstractMeshNodeAdaptor final : public AbstractSpatialAdaptor<user_types::MeshNode, ramses_base::RamsesMeshNodeHandle> {
public:
	using BaseAdaptor = AbstractSpatialAdaptor<user_types::MeshNode, ramses::MeshNode>;

	explicit AbstractMeshNodeAdaptor(AbstractSceneAdaptor* sceneAdaptor, user_types::SMeshNode node);
	~AbstractMeshNodeAdaptor();

	user_types::SMesh mesh();
	/**
	 * @returns the associated MeshAdaptor if it exists and is valid (e.g. a mesh is set)
	 */
	AbstractMeshAdaptor* meshAdaptor();

	bool sync() override;
	void syncMaterials();
	void syncMeshObject();

	RamsesHandle<ramses::Node> sceneObject() override {
		auto handlePtr = getRamsesObjectPointer();
		return std::shared_ptr<ramses::Node>(handlePtr, handlePtr->get());
	}

	/**
	 * @brief Get the axis-aligned bounding box of the current MeshNode.
	 *
	 * @param worldCoordinates If true the bounding is calculated in world space, i.e. the bounding box
	 * of the vertices transformed to world space is calculated. Otherrwise the bounding box is calculated
	 * in object space, i.e. the mesh coordinates without any transformation are used.
	 * @return The bounding box.
	 */
	BoundingBox getBoundingBox(bool worldCoordinates);

	bool highlighted() const;
	void setHighlighted(bool highlight);

private:
	void syncMaterial(size_t index);

	ramses_base::RamsesAppearance currentAppearance_;
	ramses_base::RamsesPickableObject pickObject_;
	ramses_base::RamsesArrayBuffer triangles_;

	// Subscriptions
	components::Subscription meshSubscription_;
	components::Subscription instanceCountSubscription_;
	components::Subscription visibilitySubscription_;

	bool highlight_ = false;
};

};	// namespace raco::ramses_adaptor
