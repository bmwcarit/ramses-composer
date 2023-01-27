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
#include "user_types/DefaultValues.h"
#include "core/EngineInterface.h"

namespace raco::user_types {

class TextureExternal : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"TextureExternal", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	TextureExternal(TextureExternal const& other)
		: BaseObject(other),
		  minSamplingMethod_(other.minSamplingMethod_),
		  magSamplingMethod_(other.magSamplingMethod_) {
		fillPropertyDescription();
	}

	TextureExternal(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("minSamplingMethod", &minSamplingMethod_);
		properties_.emplace_back("magSamplingMethod", &magSamplingMethod_);
	}

	// Using TextureMagSamplingMethod enumeration for min sampler is weird, but for external textures
	// only nearest and linear are allowed by ramses so instead of creating a new enumeration with the same
	// content we just reuse the mag sampling method one.
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> minSamplingMethod_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MAG_SAMPLING_METHOD_NEAREST, DisplayNameAnnotation("Min Sampling Method"), EnumerationAnnotation{TextureMagSamplingMethod}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> magSamplingMethod_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MAG_SAMPLING_METHOD_NEAREST, DisplayNameAnnotation("Mag Sampling Method"), EnumerationAnnotation{TextureMagSamplingMethod}};
};

using STextureExternal = std::shared_ptr<TextureExternal>;

}  // namespace raco::user_types
