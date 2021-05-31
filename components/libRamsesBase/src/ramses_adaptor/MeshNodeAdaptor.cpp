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
#include "user_types/Texture.h"
#include "user_types/EngineTypeAnnotation.h" 

#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_adaptor/CubeMapAdaptor.h"

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

		uniformSubscription_ = sceneAdaptor_->dispatcher()->registerOn(editorObject()->getUniformContainerHandle(0), [this]() {
			setupUniformChildrenSubscription();
			tagDirty();
		});

		optionsChildrenSubscription_ = sceneAdaptor_->dispatcher()->registerOnChildren(editorObject()->getMaterialOptionsHandle(0), [this](auto) {
			tagDirty();
		});

	} else {
		materialSubscription_ = components::Subscription{};
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
	currentAppearance_.reset();
	currentEffect_.reset();
	geometryBinding_.reset();
	currentMeshIndices_.reset();
	currentMeshVertexData_.clear();
	currentSamplers_.clear();
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

	if (auto mat = material(index)) {
		// current Appearance must be replaced before the current Effect to successfully delete last Ramses effect pointer
		if (auto materialAdapt = materialAdaptor(index)) {
			LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using materialAdaptor (valid)");
			currentAppearance_ = raco::ramses_base::ramsesAppearance(sceneAdaptor_->scene(), *materialAdapt->getRamsesObjectPointer());
			currentEffect_ = materialAdapt->getRamsesObjectPointer();
		} else {
			LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using materialAdaptor (invalid)");
			currentAppearance_ = raco::ramses_base::ramsesAppearance(sceneAdaptor_->scene(), *sceneAdaptor_->defaultEffect(haveMeshNormals));
			currentEffect_ = sceneAdaptor_->defaultEffect(haveMeshNormals);
		}

		core::ValueHandle optionsHandle = editorObject()->getMaterialOptionsHandle(index);

		setDepthWrite(currentAppearance_.get(), optionsHandle.get("depthwrite"));
		setDepthFunction(currentAppearance_.get(), optionsHandle.get("depthfunction"));
		setBlendMode(currentAppearance_.get(), optionsHandle);
		setBlendColor(currentAppearance_.get(), optionsHandle.get("blendColor"));
		setCullMode(currentAppearance_.get(), optionsHandle.get("cullmode"));
		
		std::vector<raco::ramses_base::RamsesTextureSampler> newSamplers;

		core::ValueHandle uniformsHandle = editorObject()->getUniformContainerHandle(index);
		for (size_t i{0}; i < uniformsHandle.size(); i++) {
			setUniform(currentAppearance_.get(), uniformsHandle[i]);

			if (uniformsHandle[i].type() == core::PrimitiveType::Ref) {
				auto anno = uniformsHandle[i].query<user_types::EngineTypeAnnotation>();
				auto engineType = anno->type();

				raco::ramses_base::RamsesTextureSampler sampler = nullptr;
				if (engineType == raco::core::EnginePrimitive::TextureSampler2D) {
					if (auto texture = uniformsHandle[i].asTypedRef<user_types::Texture>()) {
						if (auto adaptor = sceneAdaptor_->lookup<TextureSamplerAdaptor>(texture)) {
							sampler = adaptor->getRamsesObjectPointer();
						}
					}
				} else if (engineType == raco::core::EnginePrimitive::TextureSamplerCube) {
					if (auto texture = uniformsHandle[i].asTypedRef<user_types::CubeMap>()) {
						if (auto adaptor = sceneAdaptor_->lookup<CubeMapAdaptor>(texture)) {
							sampler = adaptor->getRamsesObjectPointer();
						}
					}
				}

				if (sampler) {
					ramses::UniformInput input;
					currentAppearance_->getEffect().findUniformInput(uniformsHandle[i].getPropName().c_str(), input);
					currentAppearance_->setInputTexture(input, *sampler);
					newSamplers.emplace_back(sampler);
				}
			}
		}
		currentSamplers_ = newSamplers;
	} else {
		LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "using defaultEffect");
		// current Appearance must be replaced before the current Effect to successfully delete last Ramses effect pointer
		currentAppearance_ = raco::ramses_base::ramsesAppearance(sceneAdaptor_->scene(), *sceneAdaptor_->defaultEffect(haveMeshNormals));
		currentEffect_ = sceneAdaptor_->defaultEffect(haveMeshNormals);
		currentSamplers_.clear();
	}
	currentAppearance_->setName(std::string(this->editorObject()->objectName() + "_Appearance").c_str());

	appearanceBinding_.reset();
	appearanceBinding_ = {sceneAdaptor_->logicEngine().createRamsesAppearanceBinding(this->editorObject()->objectName() + "_AppearanceBinding"), [this](rlogic::RamsesAppearanceBinding* ptr) {
							  sceneAdaptor_->logicEngine().destroy(*ptr);
						  }};
	appearanceBinding_->setRamsesAppearance(currentAppearance_.get());
	ramsesObject().setAppearance(*currentAppearance_.get());
}

void MeshNodeAdaptor::syncMeshObject() {
	const auto& effect = currentAppearance_->getEffect();
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
		currentAppearance_->getEffect().findAttributeInput("a_Position", input);
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
	logicNodes.push_back(appearanceBinding_.get());
}

const rlogic::Property& MeshNodeAdaptor::getProperty(const std::vector<std::string>& names) {
	if (names.size() > 1) {
		const rlogic::Property* prop{appearanceBinding_->getInputs()};
		// Remove the first 3 nesting levels of the handle: materials/slot #/uniforms container:
		for (size_t i{3}; i < names.size(); i++) {
			prop = prop->getChild(names.at(i));
		}
		return *prop;
	} else {
		return SpatialAdaptor::getProperty(names);
	}
}

};	// namespace raco::ramses_adaptor
