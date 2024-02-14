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

#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

class AbstractSceneAdaptor;
struct RamsesGizmoMeshBuffers;

class GizmoTriad {
public:
	GizmoTriad(AbstractSceneAdaptor* sceneAdaptor, ramses::Scene* scene, ramses_base::RamsesRenderGroup renderGroup, ramses_base::RamsesRenderGroup transparentRenderGroup, RamsesGizmoMeshBuffers& buffers, core::SEditorObject node);

	~GizmoTriad();

	virtual void update();
	core::SEditorObject object();

	enum class PickElement {
		None,
		Axis,
		Plane,
		Central
	};

	virtual std::pair<int, PickElement> pickElement(ramses::pickableObjectId_t pickId);

protected:
	ramses_base::RamsesMeshNode createGizmoMeshnode(ramses::Scene* scene, RamsesGizmoMeshBuffers& buffers, glm::vec3 color, float alpha = 1.0, bool invisible = false);
	ramses_base::RamsesMeshNode createFrustumMeshnode(ramses::Scene* scene, RamsesGizmoMeshBuffers& buffers, glm::vec3 color, float alpha);

	AbstractSceneAdaptor* sceneAdaptor_;
	ramses::Scene* scene_;
	ramses_base::RamsesRenderGroup renderGroup_;
	ramses_base::RamsesRenderGroup renderGroupTransparent_;
	core::SEditorObject node_;

	ramses_base::RamsesNode root_;
	std::array<ramses_base::RamsesMeshNode, 3> arrow_;
	std::array<ramses_base::RamsesPickableObject, 3> pickObject_;
	std::array<ramses::pickableObjectId_t, 3> pickId_;

	ramses_base::RamsesMeshNode frustum_;
};

class GizmoTransformation : public GizmoTriad {
public:
	GizmoTransformation(AbstractSceneAdaptor* sceneAdaptor, ramses::Scene* scene, ramses_base::RamsesRenderGroup renderGroup, ramses_base::RamsesRenderGroup transparentRenderGroup, RamsesGizmoMeshBuffers& axisBuffers, RamsesGizmoMeshBuffers& centralBuffers, core::SEditorObject node, bool enableCentralElement, bool enablePlanes, bool isRotation);

	virtual void update() override;

	virtual std::pair<int, PickElement> pickElement(ramses::pickableObjectId_t pickId);

private:
	ramses_base::RamsesMeshNode central_;
	ramses_base::RamsesPickableObject pickObjectCentral_;
	ramses::pickableObjectId_t pickIdCentral_;

	bool enablePlanes_;
	std::array<ramses_base::RamsesMeshNode, 3> plane_;
	std::array<ramses_base::RamsesPickableObject, 3> pickObjectPlane_;
	std::array<ramses::pickableObjectId_t, 3> pickIdPlane_;
};

}  // namespace raco::ramses_adaptor
