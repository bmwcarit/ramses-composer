/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AbstractMeshNodeAdaptor.h"

namespace raco::ramses_adaptor {

AbstractMeshNodeAdaptor::AbstractMeshNodeAdaptor(AbstractSceneAdaptor* sceneAdaptor, user_types::SMeshNode node)
	: AbstractSpatialAdaptor{sceneAdaptor, node, ramses_base::ramsesMeshNode(sceneAdaptor->scene(), node->objectIDAsRamsesLogicID())},
	  meshSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::mesh_}, [this]() {
			  tagDirty();
		  })},
	  instanceCountSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::instanceCount_}, [this] {
		  tagDirty();
	  })},
	  visibilitySubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{node, &user_types::MeshNode::editorVisibility_}, [this] {
		  tagDirty();
	  })} {
}

AbstractMeshNodeAdaptor::~AbstractMeshNodeAdaptor() {
	resetRamsesObject();
}

user_types::SMesh AbstractMeshNodeAdaptor::mesh() {
	return *editorObject()->mesh_;
}

AbstractMeshAdaptor* AbstractMeshNodeAdaptor::meshAdaptor() {
	if (auto m = mesh()) {
		auto adaptor = sceneAdaptor_->lookup<AbstractMeshAdaptor>(m);
		if (adaptor->isValid()) {
			return adaptor;
		}
	}
	return nullptr;
}

bool AbstractMeshNodeAdaptor::sync() {
	AbstractSpatialAdaptor::sync();
	if (*editorObject()->instanceCount_ >= 1) {
		(*ramsesObject()).setInstanceCount(*editorObject()->instanceCount_);
	}

	syncMaterials();
	syncMeshObject();

	(*ramsesObject()).setVisibility(*editorObject()->editorVisibility_ ? ramses::EVisibilityMode::Visible : ramses::EVisibilityMode::Invisible);

	tagDirty(false);
	return true;
}

void AbstractMeshNodeAdaptor::syncMaterials() {
	syncMaterial(0);
}

BoundingBox AbstractMeshNodeAdaptor::getBoundingBox(bool worldCoordinates) {
	if (getRamsesObjectPointer() != nullptr) {
		glm::mat4x4 trafoMatrix = glm::identity<glm::mat4x4>();
		if (worldCoordinates) {
			(*ramsesObject()).getModelMatrix(trafoMatrix);
		}

		if (triangles_) {
			auto numElements = triangles_->getUsedNumberOfElements();
			std::vector<glm::vec3> data(numElements);
			triangles_->getData(data.data(), numElements);

			BoundingBox bbox;
			for (auto index = 0; index < numElements; index++) {
				auto v = glm::vec3(trafoMatrix * glm::vec4(data[index], 1.0));
				bbox.merge(v);
			}
			return bbox;
		}
	}
	return {};
}

bool AbstractMeshNodeAdaptor::highlighted() const {
	return highlight_;
}

void AbstractMeshNodeAdaptor::setHighlighted(bool highlight) {
	highlight_ = highlight;
	tagDirty();
}

void AbstractMeshNodeAdaptor::syncMaterial(size_t index) {
	assert((index == 0) && "We only support one material for now.");
	// we need to reset the geometry in case of the new appearance not being compatible with the current geometry
	ramsesObject().removeAppearanceAndGeometry();

	bool haveMeshNormals = false;
	if (AbstractMeshAdaptor* meshAdapt = meshAdaptor()) {
		auto vertexData = meshAdapt->vertexData();
		if (vertexData.find("a_Normal") != vertexData.end()) {
			haveMeshNormals = true;
		}
	} else {
		haveMeshNormals = true;
	}

	currentAppearance_ = sceneAdaptor_->defaultAppearance(haveMeshNormals, highlight_);
	ramsesObject().setAppearance(currentAppearance_);
}

void AbstractMeshNodeAdaptor::syncMeshObject() {
	pickObject_.reset();
	triangles_.reset();
	auto geometry = ramses_base::ramsesGeometry(sceneAdaptor_->scene(), currentAppearance_->effect(), editorObject()->objectIDAsRamsesLogicID());
	(*geometry)->setName(std::string(this->editorObject()->objectName() + "_Geometry").c_str());

	if (AbstractMeshAdaptor* meshAdapt = meshAdaptor()) {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using meshAdaptor");

		auto vertexData = meshAdapt->vertexData();
		for (uint32_t i = 0; i < currentAppearance_->effect()->getAttributeInputCount(); i++) {
			ramses::AttributeInput attribInput = currentAppearance_->effect()->getAttributeInput(i).value();
			std::string attribName = attribInput.getName();

			auto it = vertexData.find(attribName);
			if (it != vertexData.end()) {
				geometry->addAttributeBuffer(attribInput, it->second);
			} else {
				LOG_ERROR(log_system::RAMSES_ADAPTOR, "Attrribute mismatch in MeshNode '{}': attribute '{}' not found in Mesh '{}'.", editorObject()->objectName(), attribName, mesh()->objectName());
			}
		}

		geometry->setIndices(meshAdapt->indicesPtr());

		triangles_ = meshAdapt->triangleBuffer();
	} else {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "using defaultMesh");
		int index = *editorObject()->instanceCount_ == -1 ? 1 : 0;
		ramses::AttributeInput inputPosition = (*currentAppearance_)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_POSITION).value();
		geometry->addAttributeBuffer(inputPosition, sceneAdaptor_->defaultVertices(index));

		ramses::AttributeInput inputNormals = (*currentAppearance_)->getEffect().findAttributeInput(core::MeshData::ATTRIBUTE_NORMAL).value();
		geometry->addAttributeBuffer(inputNormals, sceneAdaptor_->defaultNormals(index));
		
		geometry->setIndices(sceneAdaptor_->defaultIndices(index));

		triangles_ = sceneAdaptor_->defaultTriangles(index);
	}

	if (*editorObject()->editorVisibility_) {
		pickObject_ = ramses_base::ramsesPickableObject(sceneAdaptor_->scene(), triangles_, sceneAdaptor_->getPickId(editorObject()), sceneAdaptor_->camera());
		if (pickObject_) {
			ramsesObject().get()->addChild(*(pickObject_.get()));
		} else {
			LOG_ERROR(log_system::RAMSES_ADAPTOR, "Ramses pickable object creation for '{}' failed", editorObject()->objectName());
		}
	}

	ramsesObject().setGeometry(geometry);
}

};	// namespace raco::ramses_adaptor
