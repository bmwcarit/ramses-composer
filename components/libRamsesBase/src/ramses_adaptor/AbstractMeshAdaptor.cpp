/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AbstractMeshAdaptor.h"

#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Mesh.h"
#include <unordered_map>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

AbstractMeshAdaptor::AbstractMeshAdaptor(AbstractSceneAdaptor* sceneAdaptor, user_types::SMesh mesh)
	: AbstractUserTypeObjectAdaptor{sceneAdaptor, mesh},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  tagDirty();
	  })} {
}

ramses_base::RamsesArrayResource AbstractMeshAdaptor::indicesPtr() {
	return indices_;
}

const VertexDataMap& AbstractMeshAdaptor::vertexData() const {
	return vertexDataMap_;
}

const ramses_base::RamsesArrayBuffer AbstractMeshAdaptor::triangleBuffer() const {
	return triangleBuffer_;
}

bool AbstractMeshAdaptor::isValid() {
	auto mesh = editorObject_->meshData();
	return mesh.get() != nullptr;
}

bool AbstractMeshAdaptor::sync() {
	AbstractObjectAdaptor::sync();
	triangleBuffer_.reset();
	if (isValid()) {
		auto mesh = editorObject_->meshData();
		auto indices = mesh->getIndices();
		indices_ = ramsesArrayResource(sceneAdaptor_->scene(), indices, std::string(this->editorObject_->objectName() + "_MeshIndexData").c_str());

		for (uint32_t i{0}; i < mesh->numAttributes(); i++) {
			auto name = mesh->attribName(i);
			std::string attribName = this->editorObject_->objectName() + "_MeshVertexData_" + name;
			vertexDataMap_[name] = arrayResourceFromAttribute(sceneAdaptor_->scene(), mesh, i, attribName); 
		}

		const auto& triangleData = mesh->triangleBuffer();
		triangleBuffer_ = ramsesArrayBuffer(sceneAdaptor_->scene(), ramses::EDataType::Vector3F, triangleData.size(), triangleData.data());
	} else {
		vertexDataMap_.clear();
		indices_.reset();
	}
	tagDirty(false);
	return true;
}

};	// namespace raco::ramses_adaptor
