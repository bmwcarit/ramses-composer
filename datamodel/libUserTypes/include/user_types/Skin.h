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
#include "user_types/MeshNode.h"
#include "core/CoreAnnotations.h"

namespace raco::user_types {

class Skin : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"Skin", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Skin(Skin const& other)
		: BaseObject(other),
		  targets_(other.targets_),
		  joints_(other.joints_),
		  uri_(other.uri_),
		  skinIndex_(other.skinIndex_) {
		fillPropertyDescription();
	}

	Skin(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
		setupTargetProperties(1);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("targets", &targets_);
		properties_.emplace_back("joints", &joints_);
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("skinIndex", &skinIndex_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void updateFromExternalFile(BaseContext& context) override;


	// To account for multi-material meshnodes in gltf files (multiple primitives in single mesh in gltf)
	// the Skin can contain target multiple meshnodes.
	Property<Array<SMeshNode>, DisplayNameAnnotation, ResizableArray> targets_{{}, {"Target MeshNodes"}, {}};

	// Joints are not user-resizable since the joint set is read from the gltf-file
	Property<Array<SNode>, DisplayNameAnnotation> joints_{{}, {"Joint Nodes"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string(), {"glTF files (*.glTF *.glb);;All Files (*.*)", core::PathManager::FolderTypeKeys::Mesh}, DisplayNameAnnotation("URI")};
	Property<int, DisplayNameAnnotation> skinIndex_{0, {"Skin Index"}};

	SharedSkinData skinData() const;

	void setupTargetProperties(size_t numTargets);

private:
	size_t numJoints() const;

	void syncJointProperties(BaseContext & context);

	SharedSkinData skinData_;
};

using SSkin = std::shared_ptr<Skin>;

}  // namespace raco::user_types
