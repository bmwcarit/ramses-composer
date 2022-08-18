/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/MeshAdaptor.h"

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Mesh.h"
#include <unordered_map>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

MeshAdaptor::MeshAdaptor(SceneAdaptor* sceneAdaptor, user_types::SMesh mesh)
	: UserTypeObjectAdaptor{sceneAdaptor, mesh},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  tagDirty();
	  })},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::Mesh::objectName_}, [this]() {
		  tagDirty();
	  })} {
}

raco::ramses_base::RamsesArrayResource MeshAdaptor::indicesPtr() {
	return indices_;
}

const VertexDataMap& MeshAdaptor::vertexData() const {
	return vertexDataMap_;
}

bool MeshAdaptor::isValid() {
	auto mesh = editorObject_->meshData();
	return mesh.get() != nullptr;
}

bool MeshAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	if (isValid()) {
		auto mesh = editorObject_->meshData();
		auto indices = mesh->getIndices();
		indices_ = ramsesArrayResource(sceneAdaptor_->scene(), ramses::EDataType::UInt32, static_cast<uint32_t>(indices.size()), indices.data());
		indices_->setName(std::string(this->editorObject_->objectName() + "_MeshIndexData").c_str());


		for (uint32_t i{0}; i < mesh->numAttributes(); i++) {
			auto name = mesh->attribName(i);
			auto type = mesh->attribDataType(i);
			auto buffer = mesh->attribBuffer(i);
			auto elementCount = mesh->attribElementCount(i);
			vertexDataMap_[name] = ramsesArrayResource(sceneAdaptor_->scene(), convert(type), elementCount, buffer);
			vertexDataMap_[name]->setName(std::string(this->editorObject_->objectName() + "_MeshVertexData_" + name).c_str());
		}
	} else {
		vertexDataMap_.clear();
		indices_.reset();
	}
	tagDirty(false);
	return true;
}

};	// namespace raco::ramses_adaptor
