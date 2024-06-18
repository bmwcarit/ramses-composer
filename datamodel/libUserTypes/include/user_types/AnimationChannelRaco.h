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

#include "user_types/AnimationChannel.h"

namespace raco::user_types {

class AnimationChannelRaco : public AnimationChannelBase {
public:
	static inline const char* PROPNAME_TIME_STAMPS{"timeStamps"};
	static inline const char* PROPNAME_KEYFRAMES{"keyframes"};
	static inline const char* PROPNAME_TANGENTS_IN{"tangentsIn"};
	static inline const char* PROPNAME_TANGENTS_OUT{"tangentsOut"};

	static inline const TypeDescriptor typeDescription = {"AnimationChannelRaco", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	AnimationChannelRaco(const AnimationChannelRaco& other)
		: AnimationChannelBase(other),
		  componentType_(other.componentType_),
		  interpolationType_(other.interpolationType_),
		  componentArraySize_(other.componentArraySize_),
		  data_(other.data_) {
		fillPropertyDescription();
	}

	AnimationChannelRaco(std::string name = std::string(), std::string id = std::string())
		: AnimationChannelBase(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("componentType", &componentType_);
		properties_.emplace_back("interpolationType", &interpolationType_);
		properties_.emplace_back("componentArraySize", &componentArraySize_);
		properties_.emplace_back("data", &data_);
	}

	int getOuputComponentSize() const;

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void createPropertiesFromSamplerData(BaseContext& context, core::SharedAnimationSamplerData currentSamplerData);

	 void setAnimationData(BaseContext& context, const std::vector<float>& timeStamps, const core::AnimationSamplerData::OutputDataVariant& outputData);


	static core::AnimationSamplerData::OutputDataVariant makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<float>& keyFrames, const std::vector<float>& tangentsIn, const std::vector<float>& tangentsOut);

	static core::AnimationSamplerData::OutputDataVariant makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<int>& keyFrames, const std::vector<int>& tangentsIn, const std::vector<int>& tangentsOut);

	static core::AnimationSamplerData::OutputDataVariant makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<std::vector<float>>& keyFrames, const std::vector<std::vector<float>>& tangentsIn, const std::vector<std::vector<float>>& tangentsOut);

	static core::AnimationSamplerData::OutputDataVariant makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<std::vector<int>>& keyFrames, const std::vector<std::vector<int>>& tangentsIn, const std::vector<std::vector<int>>& tangentsOut);


	void validate(BaseContext& context);
	void adjustDataArray(BaseContext& context);
	void updateSamplerData(BaseContext& context);

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> componentType_{static_cast<int>(core::EnginePrimitive::Double), {"Component Type"}, {EUserTypeEnumerations::AnimationComponentType}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> interpolationType_{static_cast<int>(core::MeshAnimationInterpolation::Step), {"Interpolation Type"}, {EUserTypeEnumerations::AnimationInterpolationType}};
	Property<int, DisplayNameAnnotation> componentArraySize_{1, {"Component Array Size"}};
	
	Property<Table, HiddenProperty> data_{{}, {}};
};

}  // namespace raco::user_types
