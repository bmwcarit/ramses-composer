/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
	: SpatialAdaptor{sceneAdaptor, node, ramses_base::ramsesMeshNode(sceneAdaptor->scene(), node->objectIDAsRamsesLogicID())},
	  meshSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::mesh_}, [this]() {
			  tagDirty();
		  })},

	  materialsSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::materials_}, [this]() {
		  setupMaterialSubscription();
		  tagDirty();
	  })},

	  instanceCountSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::instanceCount_}, [this] {
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

const ramses_base::RamsesAppearance& MeshNodeAdaptor::privateAppearance() const {
	return privateAppearance_;
}

const ramses_base::RamsesAppearanceBinding& MeshNodeAdaptor::appearanceBinding() const {
	return appearanceBinding_;
}


bool MeshNodeAdaptor::sync(core::Errors* errors) {
	errors->removeIf([this](core::ErrorItem const& error) {
		auto handle = error.valueHandle();
		auto materialsHandle = core::ValueHandle(editorObject(), &user_types::MeshNode::materials_);
		return materialsHandle.contains(handle);
	});

	SpatialAdaptor::sync(errors);
	if (*editorObject()->instanceCount_ >= 1) {
		(*ramsesObject()).setInstanceCount(*editorObject()->instanceCount_);
	}

	syncMaterials(errors);
	syncMeshObject();

	meshNodeBinding_ = ramses_base::ramsesMeshNodeBinding(getRamsesObjectPointer(), &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_MeshNodeBinding" , editorObject()->objectIDAsRamsesLogicID());

	tagDirty(false);
	return true;
}

void MeshNodeAdaptor::syncMaterials(core::Errors* errors) {
	syncMaterial(errors, 0);
}

void MeshNodeAdaptor::syncMaterial(core::Errors* errors, size_t index) {
	assert((index == 0) && "We only support one material for now.");
	// we need to reset the geometry in case of the new appearance not being compatible with the current geometry
	ramsesObject().removeAppearanceAndGeometry();

	bool haveMeshNormals = false;
	if (MeshAdaptor* meshAdapt = meshAdaptor()) {
		auto vertexData = meshAdapt->vertexData();
		if (vertexData.find("a_Normal") != vertexData.end()) {
			haveMeshNormals = true;
		}
	} else {
		haveMeshNormals = true;
	}

	appearanceBinding_.reset();
	privateAppearance_.reset();

	if (auto materialAdapt = materialAdaptor(index)) {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using materialAdaptor (valid)");
		if (editorObject()->materialPrivate(index)) {
			privateAppearance_ = ramses_base::ramsesAppearance(sceneAdaptor_->scene(), materialAdapt->getRamsesObjectPointer(), editorObject_->objectIDAsRamsesLogicID());
			currentAppearance_ = privateAppearance_;

			appearanceBinding_ = ramses_base::ramsesAppearanceBinding(*privateAppearance_->get(), &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_AppearanceBinding", editorObject_->objectIDAsRamsesLogicID());

			core::ValueHandle optionsHandle = editorObject()->getMaterialOptionsHandle(index);
			core::ValueHandle uniformsHandle = editorObject()->getUniformContainerHandle(index);
			updateAppearance(errors, sceneAdaptor_, privateAppearance_, *editorObject()->getOptions(index), optionsHandle, uniformsHandle);

			(*privateAppearance_)->setName(std::string(this->editorObject()->objectName() + "_Appearance").c_str());
		} else {
			currentAppearance_ = materialAdapt->appearance();
		}
	} else {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using materialAdaptor (invalid)");
		currentAppearance_ = sceneAdaptor_->defaultAppearance(haveMeshNormals);
	}

	ramsesObject().setAppearance(currentAppearance_);
}

void MeshNodeAdaptor::syncMeshObject() {
	auto geometry = ramses_base::ramsesGeometry(sceneAdaptor_->scene(), currentAppearance_->effect(), editorObject()->objectIDAsRamsesLogicID());
	(*geometry)->setName(std::string(this->editorObject()->objectName() + "_Geometry").c_str());

	if (MeshAdaptor* meshAdapt = meshAdaptor()) {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using meshAdaptor");

		auto vertexData = meshAdapt->vertexData();
		for (uint32_t i = 0; i < currentAppearance_->effect()->getAttributeInputCount(); i++) {
			ramses::AttributeInput attribInput = currentAppearance_->effect()->getAttributeInput(i).value();
			std::string attribName = attribInput.getName();

			auto it = vertexData.find(attribName);
			if (it != vertexData.end()) {
				geometry->addAttributeBuffer(attribInput, it->second);
			} else {
				LOG_ERROR(log_system::RAMSES_ADAPTOR, "Attrribute mismatch in MeshNode '{}': attribute '{}' required by Material '{}' not found in Mesh '{}'.", editorObject()->objectName(), attribName, material(0)->objectName(), mesh()->objectName());
			}
		}

		geometry->setIndices(meshAdapt->indicesPtr());

	} else {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using defaultMesh");
		int index = *editorObject()->instanceCount_ == -1 ? 1 : 0;
		ramses::AttributeInput inputVertices = (*currentAppearance_)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_POSITION).value();
		geometry->addAttributeBuffer(inputVertices, sceneAdaptor_->defaultVertices(index));

		ramses::AttributeInput inputNormals = (*currentAppearance_)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_NORMAL).value();
		geometry->addAttributeBuffer(inputNormals, sceneAdaptor_->defaultNormals(index));

		geometry->setIndices(sceneAdaptor_->defaultIndices(index));
	}

	ramsesObject().setGeometry(geometry);
}

void MeshNodeAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	if (appearanceBinding_) {
		logicNodes.push_back(appearanceBinding_.get());
	}

	if (meshNodeBinding_) {
		logicNodes.push_back(meshNodeBinding_.get());
	}
}

ramses::Property* MeshNodeAdaptor::getProperty(const std::vector<std::string_view>& names) {
	if (names.size() > 1) {
		if (appearanceBinding_) {
			// Remove the first 3 nesting levels of the handle: materials/slot #/uniforms container:
			core::ValueHandle uniformContainerHandle = editorObject()->getUniformContainerHandle(0);
			auto ramsesPropNames = ramses_base::getRamsesUniformPropertyNames(uniformContainerHandle, names, 3);
			return ILogicPropertyProvider::getPropertyRecursive(appearanceBinding_->getInputs(), {ramsesPropNames.begin(), ramsesPropNames.end()}, 0);
		}
		return nullptr;
	} else if (names.size() == 1 && names[0] == "instanceCount") {
		if (meshNodeBinding_) {
			return meshNodeBinding_->getInputs()->getChild("instanceCount");
		}
	}
	return SpatialAdaptor::getProperty(names);
}

std::vector<ExportInformation> MeshNodeAdaptor::getExportInformation() const {
	auto result = std::vector<ExportInformation>();
	if (getRamsesObjectPointer() != nullptr) {
		result.emplace_back(ramses::ERamsesObjectType::MeshNode, ramsesObject().getName());
		result.emplace_back(ramses::ERamsesObjectType::Geometry, fmt::format("{}_Geometry", ramsesObject().getName()));
	}

	if (nodeBinding() != nullptr) {
		result.emplace_back("NodeBinding", nodeBinding()->getName());
	}

	if (privateAppearance_ != nullptr) {
		result.emplace_back(ramses::ERamsesObjectType::Appearance, privateAppearance_->get()->getName());
	}

	if (appearanceBinding_ != nullptr) {
		result.emplace_back("AppearanceBinding", appearanceBinding_->getName());
	}

	if (meshNodeBinding_ != nullptr) {
		result.emplace_back("MeshNodeBinding", meshNodeBinding_->getName());
	}

	return result;
}

};	// namespace raco::ramses_adaptor
