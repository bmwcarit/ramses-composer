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

#include "user_types/BaseObject.h"

#include "core/BasicAnnotations.h"

#include "core/MeshCacheInterface.h"


namespace raco::user_types {

class Mesh : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "Mesh", true };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Mesh(Mesh const& other) : BaseObject(other), uri_(other.uri_), meshIndex_(other.meshIndex_), bakeMeshes_(other.bakeMeshes_), materialNames_(other.materialNames_)
	{
		fillPropertyDescription();
	}

	Mesh(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("meshIndex", &meshIndex_);
		properties_.emplace_back("bakeMeshes", &bakeMeshes_);
		properties_.emplace_back("materialNames", &materialNames_);
	}

	void updateFromExternalFile(BaseContext& context) override;
	void onAfterValueChanged(BaseContext &context, ValueHandle const& value) override;
		
	std::vector<std::string> materialNames();
	
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string(), {"All Meshes (*.ctm *.glTF *.glb);;CTM files (*.ctm);;glTF files (*.glTF *.glb);;All Files (*.*)", core::PathManager::FolderTypeKeys::Mesh}, DisplayNameAnnotation("URI")};

	Property<int, DisplayNameAnnotation> meshIndex_{0, DisplayNameAnnotation("Mesh Index")};
	Property<bool, DisplayNameAnnotation> bakeMeshes_{true, DisplayNameAnnotation("Bake All Meshes")};
	
	Property<Table, ArraySemanticAnnotation, HiddenProperty> materialNames_{{}, {}, {}};
	
	SharedMeshData meshData() const {
		return mesh_;
	}

	std::map<std::string, std::string> metaData_;

private:
	
	SharedMeshData mesh_;

	mutable FileChangeMonitor::UniqueListener uriListener_;
};

using SMesh = std::shared_ptr<Mesh>;

}
