/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/Gizmos.h"

#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_adaptor/BuildOptions.h"
#include "ramses_adaptor/DefaultRamsesObjects.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"

#include "user_types/PerspectiveCamera.h"

#include "core/MeshCacheInterface.h"

#include <glm/gtx/euler_angles.hpp>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

const char* frustumVertexShader =
	R"(
#version 300 es
precision highp float;

in vec3 a_Position;

uniform mat4 u_PMatrix;
uniform mat4 u_VMatrix;

uniform mat4 frustum_proj_matrix;
uniform mat4 frustum_model_matrix;

void main() {
    gl_Position = u_PMatrix * u_VMatrix * frustum_model_matrix * inverse(frustum_proj_matrix) * vec4(a_Position, 1.0);
}
)";

const char* frustumFragmentShader =
	R"(
#version 300 es
precision highp float;

out vec4 fragColor;

uniform vec3 color;

void main() {
     fragColor = vec4(color, 0.2);
}
)";


ramses_base::RamsesMeshNode GizmoTriad::createGizmoMeshnode(ramses::Scene* scene, RamsesGizmoMeshBuffers& buffers, glm::vec3 color, float alpha, bool invisible) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(flatColorVertexShader);
	effectDescription.setFragmentShader(flatColorFragmentShader);
	effectDescription.setUniformSemantic("u_MVPMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);

	auto effect = ramses_base::ramsesEffect(scene, effectDescription, {}, {0, 0});
	auto appearance = ramses_base::ramsesAppearance(scene, effect, {0, 0});
	(*appearance)->setDepthFunction(ramses::EDepthFunc::LessEqual);
	(*appearance)->setCullingMode(ramses::ECullMode::Disabled);

	if (invisible) {
		(*appearance)->setColorWriteMask(false, false, false, false);
	}

	ramsesSetUniform(**appearance, "u_color", color);
	ramsesSetUniform(**appearance, "u_alpha", alpha);

	auto geometry = ramsesGeometry(scene, effect, {0, 0});
	geometry->setIndices(buffers.indices);

	ramses::AttributeInput inputPosition = (*appearance)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_POSITION).value();
	geometry->addAttributeBuffer(inputPosition, buffers.vertices);

	auto meshNode = ramsesMeshNode(scene, {0, 0});
	meshNode->setGeometry(geometry);
	meshNode->setAppearance(appearance);

	return meshNode;
}

ramses_base::RamsesMeshNode GizmoTriad::createFrustumMeshnode(ramses::Scene* scene, RamsesGizmoMeshBuffers& buffers, glm::vec3 color, float alpha) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(frustumVertexShader);
	effectDescription.setFragmentShader(frustumFragmentShader);
	effectDescription.setUniformSemantic("u_VMatrix", ramses::EEffectUniformSemantic::ViewMatrix);
	effectDescription.setUniformSemantic("u_PMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix);

	auto effect = ramses_base::ramsesEffect(scene, effectDescription, {}, {0, 0});
	auto appearance = ramses_base::ramsesAppearance(scene, effect, {0, 0});

	(*appearance)->setDepthFunction(ramses::EDepthFunc::LessEqual);
	(*appearance)->setCullingMode(ramses::ECullMode::Disabled);

	(*appearance)->setDepthWrite(ramses::EDepthWrite::Disabled);
	(*appearance)->setBlendingOperations(ramses::EBlendOperation::Add, ramses::EBlendOperation::Add);
	(*appearance)->setBlendingFactors(ramses::EBlendFactor::SrcAlpha, ramses::EBlendFactor::OneMinusSrcAlpha, ramses::EBlendFactor::One, ramses::EBlendFactor::One);

	auto geometry = ramsesGeometry(scene, effect, {0, 0});
	geometry->setIndices(buffers.indices);

	ramses::AttributeInput inputPosition = (*appearance)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_POSITION).value();
	geometry->addAttributeBuffer(inputPosition, buffers.vertices);

	auto meshNode = ramsesMeshNode(scene, {0, 0});
	meshNode->setGeometry(geometry);
	meshNode->setAppearance(appearance);

	return meshNode;
}

GizmoTriad::GizmoTriad(AbstractSceneAdaptor* sceneAdaptor, ramses::Scene* scene, ramses_base::RamsesRenderGroup renderGroup, ramses_base::RamsesRenderGroup transparentRenderGroup,
	RamsesGizmoMeshBuffers& buffers, core::SEditorObject node)
	: sceneAdaptor_(sceneAdaptor), renderGroup_(renderGroup), renderGroupTransparent_(transparentRenderGroup), node_(node) {
	root_ = ramses_base::ramsesNode(scene, {0, 0});

	std::vector<glm::vec3> colors{{9.0, 0.2, 0.2}, {0.2, 0.9, 0.2}, {0.2, 0.2, 0.9}};

	for (int axis = 0; axis < 3; axis++) {
		arrow_[axis] = createGizmoMeshnode(scene, buffers, colors[axis]);
		(*root_)->addChild(**arrow_[axis]);
		renderGroup->addMeshNode(arrow_[axis], axis);

		pickId_[axis] = sceneAdaptor_->getPickId();
		pickObject_[axis] = ramses_base::ramsesPickableObject(sceneAdaptor_->scene(), buffers.triangles, pickId_[axis], sceneAdaptor_->camera());
		if (pickObject_[axis]) {
			(**arrow_[axis]).addChild((*pickObject_[axis].get()));
		}
	}

	if (node->isType<user_types::PerspectiveCamera>() || node->isType<user_types::OrthographicCamera>()) {
		RamsesGizmoMeshBuffers cubeBuffers = createGizmoCube(scene);
		frustum_ = createFrustumMeshnode(scene, cubeBuffers, {0.1, 0.1, 0.1}, 0.2);

		transparentRenderGroup->addMeshNode(frustum_, 0);
	}

	update();
}

GizmoTriad::~GizmoTriad() {
	renderGroup_->removeAllRenderables();
	renderGroupTransparent_->removeAllRenderables();
}

void GizmoTriad::update() {
	auto modelMatrix = sceneAdaptor_->modelMatrix(node_);
	glm::vec3 worldOrigin = modelMatrix * glm::vec4(0, 0, 0, 1);

	(*root_)->setTranslation(worldOrigin);

	// rotate individual axes
	std::vector<glm::vec3> axisVecs{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

	for (int axis = 0; axis < 3; axis++) {
		glm::vec3 axisWorld = modelMatrix * glm::vec4(axisVecs[axis], 0.0);

		// find rotate (0,0,1) -> axisWorld
		glm::quat rotationQuat(glm::vec3(0, 0, 1), axisWorld);
		(**arrow_[axis]).setRotation(rotationQuat);
	}

	// Find scaling to make gizmo size appear constant on screen
	auto viewMatrix = sceneAdaptor_->cameraController().viewMatrix();
	glm::vec3 originView = viewMatrix * modelMatrix * glm::vec4(0, 0, 0, 1);
	float dist = glm::length(originView);
	float scale = 0.05 * dist;
	(**root_).setScaling({scale, scale, scale});


	ramses::Camera* ramsesCamera = nullptr;
	if (auto cameraAdaptor = sceneAdaptor_->previewAdaptor()->lookup<PerspectiveCameraAdaptor>(node_)) {
		ramsesCamera = cameraAdaptor->getRamsesObjectPointer()->get();
	} else if (auto cameraAdaptor = sceneAdaptor_->previewAdaptor()->lookup<OrthographicCameraAdaptor>(node_)) {
		ramsesCamera = cameraAdaptor->getRamsesObjectPointer()->get();
	}
	if (ramsesCamera) {
		glm::mat4 camProjectionMatrix;
		glm::mat4 camModelMatrix;
		ramsesCamera->getModelMatrix(camModelMatrix);
		auto status = ramsesCamera->getProjectionMatrix(camProjectionMatrix);

		ramses::Appearance* appearance = (**frustum_).getAppearance();

		ramsesSetUniform<glm::vec3>(*appearance, "color", {0.2, 0.2, 0.2});

		ramsesSetUniform<glm::mat4>(*appearance, "frustum_model_matrix", camModelMatrix);
		ramsesSetUniform<glm::mat4>(*appearance, "frustum_proj_matrix", camProjectionMatrix);
	}
}

core::SEditorObject GizmoTriad::object() {
	return node_;
}

std::pair<int, GizmoTriad::PickElement> GizmoTriad::pickElement(ramses::pickableObjectId_t pickId) {
	for (int axis = 0; axis < 3; axis++) {
		if (pickId == pickId_[axis]) {
			return {axis, PickElement::Axis};
		}
	}
	return {-1, PickElement::None};
}

GizmoTransformation::GizmoTransformation(AbstractSceneAdaptor* sceneAdaptor, ramses::Scene* scene, ramses_base::RamsesRenderGroup renderGroup, ramses_base::RamsesRenderGroup transparentRenderGroup, RamsesGizmoMeshBuffers& axisBuffers, RamsesGizmoMeshBuffers& centralBuffers, core::SEditorObject node, bool enableCentralElement, bool enablePlanes, bool isRotation)
	: enablePlanes_(enablePlanes), 
	GizmoTriad(sceneAdaptor, scene, renderGroup, transparentRenderGroup, axisBuffers, node) {

	if (enableCentralElement) {
		central_ = createGizmoMeshnode(scene, centralBuffers, {0.4, 0.4, 0.4});
		(**central_).setScaling({0.2, 0.2, 0.2});
		(*root_)->addChild(**central_);
		renderGroup->addMeshNode(central_, 7);

		pickIdCentral_ = sceneAdaptor_->getPickId();
		pickObjectCentral_ = ramses_base::ramsesPickableObject(sceneAdaptor_->scene(), centralBuffers.triangles, pickIdCentral_, sceneAdaptor_->camera());
		if (pickObjectCentral_) {
			(**central_).addChild(*pickObjectCentral_.get());
		}
	}

	if (isRotation) {
		central_ = createGizmoMeshnode(scene, centralBuffers, {0.5, 0.5, 0.5}, 0.3, true);
		(**central_).setScaling({0.95, 0.95, 0.95});
		(*root_)->addChild(**central_);
		renderGroup->addMeshNode(central_, -1);
	}

	if (enablePlanes_) {
		std::vector<glm::vec3> colors{{9.0, 0.2, 0.2}, {0.2, 0.9, 0.2}, {0.2, 0.2, 0.9}};

		RamsesGizmoMeshBuffers planeBuffers = createGizmoQuad(scene);
		for (int axis = 0; axis < 3; axis++) {
			plane_[axis] = createGizmoMeshnode(scene, planeBuffers, colors[axis]);
			(**plane_[axis]).setScaling({0.25, 0.25, 0.25});
			(*root_)->addChild(**plane_[axis]);
			renderGroup->addMeshNode(plane_[axis], axis);

			pickIdPlane_[axis] = sceneAdaptor_->getPickId();
			pickObjectPlane_[axis] = ramses_base::ramsesPickableObject(sceneAdaptor_->scene(), planeBuffers.triangles, pickIdPlane_[axis], sceneAdaptor_->camera());
			if (pickObjectPlane_[axis]) {
				(**plane_[axis]).addChild((*pickObjectPlane_[axis].get()));
			}
		}
	}

	update();
}

void GizmoTransformation::update() {
	GizmoTriad::update();

	if (enablePlanes_) {
		auto modelMatrix = sceneAdaptor_->modelMatrix(node_);

		// rotate individual quads
		std::vector<glm::vec3> axisVecs{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

		for (int axis = 0; axis < 3; axis++) {
			glm::vec3 axisWorld1 = glm::normalize(modelMatrix * glm::vec4(axisVecs[(axis + 1) % 3], 0.0));
			glm::vec3 axisWorld2 = glm::normalize(modelMatrix * glm::vec4(axisVecs[(axis + 2) % 3], 0.0));
			glm::vec3 planeNormalWorld = glm::normalize(glm::cross(axisWorld1, axisWorld2));

			// Note: axisWorld1 and axisWorld2 may not be orthogonal; in that case extracting the euler angles from the matrix
			// is qustionable but seems to work anyway in simple cases.
			glm::mat3 rotationMat(axisWorld1, axisWorld2, planeNormalWorld);
			glm::mat4 m4(rotationMat);

			float rotX, rotY, rotZ;
			glm::extractEulerAngleXYZ(m4, rotX, rotY, rotZ);

			(**plane_[axis]).setRotation({glm::degrees(rotX), glm::degrees(rotY), glm::degrees(rotZ)}, ramses::ERotationType::Euler_ZYX);

			glm::vec3 translation = 0.5f * (axisWorld1 + axisWorld2);
			(**plane_[axis]).setTranslation(translation);
		}
	}
}

std::pair<int, GizmoTriad::PickElement> GizmoTransformation::pickElement(ramses::pickableObjectId_t pickId) {
	if (pickId == pickIdCentral_) {
		return {0, PickElement::Central};
	}

	auto result = GizmoTriad::pickElement(pickId);
	if (result.first != -1) {
		return result;
	}

	if (enablePlanes_) {
		for (int plane = 0; plane < 3; plane++) {
			if (pickId == pickIdPlane_[plane]) {
				return {plane, PickElement::Plane};
			}
		}
	}
	return {-1, PickElement::None};
}

}  // namespace raco::ramses_adaptor