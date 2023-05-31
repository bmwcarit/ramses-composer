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

#include "user_types/Mesh.h"

#include "core/EngineInterface.h"

namespace raco::user_types {

class AnimationChannel : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "AnimationChannel", true };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	AnimationChannel(const AnimationChannel& other)
		: BaseObject(other),
		  uri_(other.uri_),
		  animationIndex_(other.animationIndex_),
		  samplerIndex_(other.samplerIndex_) {
		fillPropertyDescription();
	}

	AnimationChannel(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("animationIndex", &animationIndex_);
		properties_.emplace_back("samplerIndex", &samplerIndex_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void updateFromExternalFile(BaseContext& context) override;

	PropertyInterface getOutputProperty() const;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string(), {"glTF files (*.glTF *.glb);;All Files (*.*)", core::PathManager::FolderTypeKeys::Mesh}, DisplayNameAnnotation("Animation Source")};

	Property<int, DisplayNameAnnotation> animationIndex_{{}, DisplayNameAnnotation("Animation Index")};
	Property<int, DisplayNameAnnotation> samplerIndex_{{}, DisplayNameAnnotation("Sampler Index")};

	raco::core::SharedAnimationSamplerData currentSamplerData_;

private:
	void createSamplerInfoBox(BaseContext& context, int animationAmount, int samplerAmount, bool unsupportedArray);
};

using SAnimationChannel = std::shared_ptr<AnimationChannel>;

}
