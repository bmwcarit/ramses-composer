/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "user_types/BaseObject.h"
#include "user_types/DefaultValues.h"
#include "core/EngineInterface.h"

namespace raco::user_types {

class BaseTexture : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"BaseTexture", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

    BaseTexture(BaseTexture const& other)
		: BaseObject(other)
        , wrapUMode_(other.wrapUMode_)
        , wrapVMode_(other.wrapVMode_)
        , minSamplingMethod_(other.minSamplingMethod_)
        , magSamplingMethod_(other.magSamplingMethod_)
        , anisotropy_(other.anisotropy_) {
		fillPropertyDescription();
	}

    BaseTexture(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
    }

    void fillPropertyDescription() {
		properties_.emplace_back("wrapUMode", &wrapUMode_);
		properties_.emplace_back("wrapVMode", &wrapVMode_);
		properties_.emplace_back("minSamplingMethod", &minSamplingMethod_);
		properties_.emplace_back("magSamplingMethod", &magSamplingMethod_);
		properties_.emplace_back("anisotropy", &anisotropy_);
	}

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> wrapUMode_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP, DisplayNameAnnotation("Wrap U Mode"), EnumerationAnnotation{TextureAddressMode}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> wrapVMode_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP, DisplayNameAnnotation("Wrap V Mode"), EnumerationAnnotation{TextureAddressMode}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> minSamplingMethod_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MIN_SAMPLING_METHOD_NEAREST, DisplayNameAnnotation("Min Sampling Method"), EnumerationAnnotation{TextureMinSamplingMethod}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> magSamplingMethod_{DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MAG_SAMPLING_METHOD_NEAREST, DisplayNameAnnotation("Mag Sampling Method"), EnumerationAnnotation{TextureMagSamplingMethod}};
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> anisotropy_{1, DisplayNameAnnotation{"Anisotropy Level"}, RangeAnnotation<int>(1, 32000) };
};

}