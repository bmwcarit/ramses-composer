/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/MeshNodeAdaptor.h"

#include "user_types/CubeMap.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/Texture.h"

#include "ramses_adaptor/SceneAdaptor.h"

namespace raco::ramses_adaptor {

MeshNodeAdaptor::MeshNodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SMeshNode node)
	: SpatialAdaptor{sceneAdaptor, node, raco::ramses_base::ramsesMeshNode(sceneAdaptor->scene())},
	  meshSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node}.get("mesh"), [this]() {
			  tagDirty();
		  })},

	  materialsSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node}.get("materials"), [this]() {
		  setupMaterialSubscription();
		  tagDirty();
	  })},

	  instanceCountSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node}.get("instanceCount"), [this] {
		  tagDirty();
	  })} {
	setupMaterialSubscription();
	setupUniformChildrenSubscription();
}

void MeshNodeAdaptor::setupMaterialSubscription() {
	if (editorObject()->numMaterialSlots() == 1) {
		materialSubscription_ = sceneAdaptor_->dispatcher()->registerOn(editorObject()->getMaterialHandle(0), [this]() {
			tagDirty();
		});

		matPrivateSubscription_ = sceneAdaptor_->dispatcher()->registerOn(editorObject()->getMaterialPrivateHandle(0), [this]() {
			tagDirty();
		});

		uniformSubscription_ = sceneAdaptor_->dispatcher()->registerOn(editorObject()->getUniformContainerHandle(0), [this]() {
			setupUniformChildrenSubscription();
			tagDirty();
		});

		optionsChildrenSubscription_ = sceneAdaptor_->dispatcher()->registerOnChildren(editorObject()->getMaterialOptionsHandle(0), [this](auto) {
			tagDirty();
		});

	} else {
		materialSubscription_ = components::Subscription{};
		matPrivateSubscription_ = components::Subscription{};
		uniformSubscription_ = components::Subscription{};
		uniformChildrenSubscription_ = components::Subscription{};
		optionsChildrenSubscription_ = components::Subscription{};
	}
}

void MeshNodeAdaptor::setupUniformChildrenSubscription() {
	if (editorObject()->numMaterialSlots() == 1) {
		uniformChildrenSubscription_ = sceneAdaptor_->dispatcher()->registerOnChildren(editorObject()->getUniformContainerHandle(0), [this](auto) {
			tagDirty();
		});
	}
}

MeshNodeAdaptor::~MeshNodeAdaptor() {
	resetRamsesObject();
	privateAppearance_.reset();
	geometryBinding_.reset();
	currentMeshIndices_.reset();
	currentMeshVertexData_.clear();
}

user_types::SMesh MeshNodeAdaptor::mesh() {
	return *editorObject()->mesh_;
}

MeshAdaptor* MeshNodeAdaptor::meshAdaptor() {
	if (auto m = mesh()) {
		auto adaptor = sceneAdaptor_->lookup<MeshAdaptor>(m);
		if (adaptor->isValid()) {
			return adaptor;
		}
	}
	return nullptr;
}

user_types::SMaterial MeshNodeAdaptor::material(size_t index) {
	return editorObject()->getMaterial(index);
}

MaterialAdaptor* MeshNodeAdaptor::materialAdaptor(size_t index) {
	if (auto m = material(index)) {
		auto adaptor = sceneAdaptor_->lookup<MaterialAdaptor>(m);
		if (adaptor->isValid()) {
			return adaptor;
		}
	}
	return nullptr;
}

const raco::ramses_base::RamsesAppearance& MeshNodeAdaptor::privateAppearance() const {
	return privateAppearance_;
}


bool MeshNodeAdaptor::sync(core::Errors* errors) {
	BaseAdaptor::sync(errors);
	if (*editorObject()->instanceCount_ >= 1) {
		ramsesObject().setInstanceCount(*editorObject()->instanceCount_);
	}

	syncMaterials();
	syncMeshObject();
	tagDirty(false);
	return true;
}

void MeshNodeAdaptor::syncMaterials() {
	syncMaterial(0);
}

void MeshNodeAdaptor::syncMaterial(size_t index) {
	assert((index == 0) && "We only support one material for now.");
	// we need to reset the geometry in case of the new appearance not being compatible with the current geometry
	ramsesObject().removeAppearanceAndGeometry();

	bool haveMeshNormals = false;
	if (MeshAdaptor* meshAdapt = meshAdaptor()) {
		auto vertexData = meshAdapt->vertexData();
		if (vertexData.find("a_Normal") != vertexData.end()) {
			haveMeshNormals = true;
		}
	}

	appearanceBinding_.reset();
	privateAppearance_.reset();

	if (auto materialAdapt = materialAdaptor(index)) {
		LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using materialAdaptor (valid)");
		if (editorObject()->materialPrivate(index)) {
			privateAppearance_ = raco::ramses_base::ramsesAppearance(sceneAdaptor_->scene(), materialAdapt->getRamsesObjectPointer());
			currentAppearance_ = privateAppearance_;

			core::ValueHandle optionsHandle = editorObject()->getMaterialOptionsHandle(index);
			core::ValueHandle uniformsHandle = editorObject()->getUniformContainerHandle(index);
			updateAppearance(sceneAdaptor_, privateAppearance_, optionsHandle, uniformsHandle);

			(*privateAppearance_)->setName(std::string(this->editorObject()->objectName() + "_Appearance").c_str());

			appearanceBinding_ = raco::ramses_base::ramsesAppearanceBinding(&sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_AppearanceBinding");
			appearanceBinding_->setRamsesAppearance(privateAppearance_->get());

		} else {
			currentAppearance_ = materialAdapt->appearance();
		}
	} else {
		LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using materialAdaptor (invalid)");
		currentAppearance_ = sceneAdaptor_->defaultAppearance(haveMeshNormals);
	}

	ramsesObject().setAppearance(*currentAppearance_->get());
}

void MeshNodeAdaptor::syncMeshObject() {
	const auto& effect = (*currentAppearance_)->getEffect();
	geometryBinding_ = raco::ramses_base::ramsesGeometryBinding(sceneAdaptor_->scene(), effect);
	geometryBinding_->setName(std::string(this->editorObject()->objectName() + "_GeometryBinding").c_str());

	currentMeshVertexData_.clear();
	currentMeshIndices_.reset();

	if (MeshAdaptor* meshAdapt = meshAdaptor()) {
		LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using meshAdaptor");

		auto vertexData = meshAdapt->vertexData();
		for (uint32_t i = 0; i < effect.getAttributeInputCount(); i++) {
			ramses::AttributeInput attribInput;
			effect.getAttributeInput(i, attribInput);
			std::string attribName = attribInput.getName();

			auto it = vertexData.find(attribName);
			if (it != vertexData.end()) {
				auto status = geometryBinding_->setInputBuffer(attribInput, *it->second);
				if (status == ramses::StatusOK) {
					currentMeshVertexData_.emplace_back(it->second);
				} else {
					LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, geometryBinding_->getStatusMessage(status));
				}
			} else {
				LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "Attrribute mismatch in MeshNode '{}': attribute '{}' required by Material '{}' not found in Mesh '{}'.", editorObject()->objectName(), attribName, material(0)->objectName(), mesh()->objectName());
			}
		}

		auto status = geometryBinding_->setIndices(*meshAdapt->indicesPtr());
		if (status == ramses::StatusOK) {
			currentMeshIndices_ = meshAdapt->indicesPtr();
		} else {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, geometryBinding_->getStatusMessage(status));
		}

	} else {
		LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using defaultMesh");
		ramses::AttributeInput input;
		(*currentAppearance_)->getEffect().findAttributeInput("a_Position", input);
		currentMeshVertexData_.emplace_back(sceneAdaptor_->defaultVertices());
		currentMeshIndices_ = sceneAdaptor_->defaultIndices();

		geometryBinding_->setInputBuffer(input, *currentMeshVertexData_.front());
		geometryBinding_->setIndices(*currentMeshIndices_);
	}

	ramsesObject().setGeometryBinding(*geometryBinding_.get());
}

void MeshNodeAdaptor::addObjectToRenderGroup(ramses::RenderGroup& renderGroup, int orderWithinGroup) {
	// We can use mesh node orders such as (0 2 3 6) in a render group
	// Ramses stores the object handle and the order as a singular vector entry and lazily sorts the vector in-place according to the order
	auto renderGroupAddStatus = renderGroup.addMeshNode(this->ramsesObject(), orderWithinGroup);
	assert(renderGroupAddStatus == ramses::StatusOK);
}

void MeshNodeAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	if (appearanceBinding_) {
		logicNodes.push_back(appearanceBinding_.get());
	}
}

const rlogic::Property* MeshNodeAdaptor::getProperty(const std::vector<std::string>& names) {
	if (names.size() > 1) {
		if (appearanceBinding_) {
			const rlogic::Property* prop{appearanceBinding_->getInputs()};
			// Remove the first 3 nesting levels of the handle: materials/slot #/uniforms container:
			for (size_t i{3}; i < names.size(); i++) {
				prop = prop->getChild(names.at(i));
			}
			return prop;
		}
		return nullptr;
	}
	return SpatialAdaptor::getProperty(names);
}

};	// namespace raco::ramses_adaptor
