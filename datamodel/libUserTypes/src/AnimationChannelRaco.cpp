/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/AnimationChannelRaco.h"

#include "core/CoreFormatter.h"
#include "core/Errors.h"

#include "Validation.h"

namespace raco::user_types {

template <typename T>
struct RamsesToPropertyTypeTraits {};

template <>
struct RamsesToPropertyTypeTraits<float> {
	using PropType = double;
};

template <>
struct RamsesToPropertyTypeTraits<int32_t> {
	using PropType = int;
};


template <typename RamsesElementType, int N>
void convertDataAsArray(BaseContext& context, const ValueHandle& handle, const std::vector<glm::vec<N, RamsesElementType, glm::defaultp>>& data, const std::string& propName) {
	auto outputsProp = std::make_unique<Value<Array<Array<typename RamsesToPropertyTypeTraits<RamsesElementType>::PropType>>>>();
	for (size_t index = 0; index < data.size(); index++) {
		auto& v = data[index];
		auto& valueProp = (*outputsProp)->addProperty()->asArray();
		for (size_t component = 0; component < N; component++) {
			auto compProp = valueProp.addProperty();
			*compProp = v[component];
		}
	}
	context.addProperty(handle, propName, std::move(outputsProp));
}

void convertDataAsArray(BaseContext& context, const ValueHandle& handle, const std::vector<std::vector<float>>& data, const std::string& propName) {
	auto outputsProp = std::make_unique<Value<Array<Array<double>>>>();
	for (size_t index = 0; index < data.size(); index++) {
		auto& v = data[index];
		auto& valueProp = (*outputsProp)->addProperty()->asArray();
		for (size_t component = 0; component < v.size(); component++) {
			auto compProp = valueProp.addProperty();
			*compProp = v[component];
		}
	}
	context.addProperty(handle, propName, std::move(outputsProp));
}

template <typename RamsesElementType>
void convertDataAsArray(BaseContext& context, const ValueHandle& handle, const std::vector<RamsesElementType>& data, const std::string& propName) {
	auto inputs = std::make_unique<Value<Array<typename RamsesToPropertyTypeTraits<RamsesElementType>::PropType>>>();
	for (size_t index = 0; index < data.size(); index++) {
		auto elem = (*inputs)->addProperty();
		*elem = data[index];
	}
	context.addProperty(handle, propName, std::move(inputs));
}

template <typename ElementType>
void createCurveDataProperties(BaseContext& context, ValueHandle dataHandle, const core::AnimationOutputData<ElementType>& samplerData) {
	convertDataAsArray(context, dataHandle, samplerData.keyFrames, AnimationChannelRaco::PROPNAME_KEYFRAMES);
	if (!samplerData.tangentsIn.empty()) {
		convertDataAsArray(context, dataHandle, samplerData.tangentsIn, AnimationChannelRaco::PROPNAME_TANGENTS_IN);
	}
	if (!samplerData.tangentsOut.empty()) {
		convertDataAsArray(context, dataHandle, samplerData.tangentsOut, AnimationChannelRaco::PROPNAME_TANGENTS_OUT);
	}
}

void AnimationChannelRaco::createPropertiesFromSamplerData(BaseContext& context, core::SharedAnimationSamplerData samplerData) {
	ValueHandle dataHandle({shared_from_this(), &AnimationChannelRaco::data_});

	context.removeAllProperties(dataHandle);

	context.set({shared_from_this(), &AnimationChannelRaco::interpolationType_}, static_cast<int>(samplerData->interpolation));
	context.set({shared_from_this(), &AnimationChannelRaco::componentType_}, static_cast<int>(samplerData->componentType));
	if (samplerData->componentType == core::EnginePrimitive::Array) {
		const auto& keyFrames = std::get<AnimationOutputData<std::vector<float>>>(samplerData->output).keyFrames;
		int compSize = keyFrames.empty() ? 1 : keyFrames.begin()->size();
		if (compSize > 0) {
			context.set({shared_from_this(), &AnimationChannelRaco::componentArraySize_}, compSize);
		}
	}

	convertDataAsArray(context, dataHandle, samplerData->timeStamps, PROPNAME_TIME_STAMPS);

	std::visit(
		[this, dataHandle, &context](const auto& data) {
			createCurveDataProperties(context, dataHandle, data);
		},
		samplerData->output);

	// Need to update the sampler data for the engine since the BaseContext::addProperty calls will not invoke onAfterValueChanged
	updateSamplerData(context);
}

void AnimationChannelRaco::setAnimationData(BaseContext& context, const std::vector<float>& timeStamps, const core::AnimationSamplerData::OutputDataVariant& outputData) {
	ValueHandle dataHandle({shared_from_this(), &AnimationChannelRaco::data_});

	context.removeAllProperties(dataHandle);
	convertDataAsArray(context, dataHandle, timeStamps, PROPNAME_TIME_STAMPS);

	std::visit(
		[this, dataHandle, &context](const auto& data) {
			createCurveDataProperties(context, dataHandle, data);
		},
		outputData);

	// Need to update the sampler data for the engine since the BaseContext::addProperty calls will not invoke onAfterValueChanged
	updateSamplerData(context);
}


template <typename RamsesElementType, int N>
std::vector<glm::vec<N, RamsesElementType, glm::defaultp>> convertVectorToGlm(const std::vector<std::vector<RamsesElementType>>& data) {
	using ComponentType = glm::vec<N, RamsesElementType, glm::defaultp>;
	std::vector<ComponentType> result;
	for (size_t index = 0; index < data.size(); index++) {
		ComponentType v;
		for (size_t compIndex = 0; compIndex < N; compIndex++) {
			v[compIndex] = data[index][compIndex];
		}
		result.emplace_back(v);
	}
	return result;
}

core::AnimationSamplerData::OutputDataVariant AnimationChannelRaco::makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<float>& keyFrames, const std::vector<float>& tangentsIn, const std::vector<float>& tangentsOut) {
	return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<float>{keyFrames, tangentsIn, tangentsOut});
}

core::AnimationSamplerData::OutputDataVariant AnimationChannelRaco::makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<int>& keyFrames, const std::vector<int>& tangentsIn, const std::vector<int>& tangentsOut) {
	return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<int32_t>{keyFrames, tangentsIn, tangentsOut});
}


core::AnimationSamplerData::OutputDataVariant AnimationChannelRaco::makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<std::vector<float>>& keyFrames, const std::vector<std::vector<float>>& tangentsIn, const std::vector<std::vector<float>>& tangentsOut) {
	switch (static_cast<core::EnginePrimitive>(componentType)) {
		case core::EnginePrimitive::Array:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<std::vector<float>>{keyFrames, tangentsIn, tangentsOut});
			break;
		case core::EnginePrimitive::Vec2f:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::vec2>{convertVectorToGlm<float, 2>(keyFrames), convertVectorToGlm<float, 2>(tangentsIn), convertVectorToGlm<float, 2>(tangentsOut)});
			break;
		case core::EnginePrimitive::Vec3f:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::vec3>{convertVectorToGlm<float, 3>(keyFrames), convertVectorToGlm<float, 3>(tangentsIn), convertVectorToGlm<float, 3>(tangentsOut)});
			break;
		case core::EnginePrimitive::Vec4f:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::vec4>{convertVectorToGlm<float, 4>(keyFrames), convertVectorToGlm<float, 4>(tangentsIn), convertVectorToGlm<float, 4>(tangentsOut)});
			break;
		default:
			assert(false);
	}
	return {};
}

core::AnimationSamplerData::OutputDataVariant AnimationChannelRaco::makeAnimationOutputData(core::EnginePrimitive componentType, const std::vector<std::vector<int>>& keyFrames, const std::vector<std::vector<int>>& tangentsIn, const std::vector<std::vector<int>>& tangentsOut) {
	switch (static_cast<core::EnginePrimitive>(componentType)) {
		case core::EnginePrimitive::Vec2i:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::ivec2>{convertVectorToGlm<int, 2>(keyFrames), convertVectorToGlm<int, 2>(tangentsIn), convertVectorToGlm<int, 2>(tangentsOut)});
			break;
		case core::EnginePrimitive::Vec3i:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::ivec3>{convertVectorToGlm<int, 3>(keyFrames), convertVectorToGlm<int, 3>(tangentsIn), convertVectorToGlm<int, 3>(tangentsOut)});
			break;
		case core::EnginePrimitive::Vec4i:
			return core::AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<glm::ivec4>{convertVectorToGlm<int, 4>(keyFrames), convertVectorToGlm<int, 4>(tangentsIn), convertVectorToGlm<int, 4>(tangentsOut)});
			break;
		default:
			assert(false);
	}
	return {};
}


template<typename RamsesElementType>
std::vector<RamsesElementType> getSamplerDataArray(data_storage::Table& cont, const char* propName) {
	using PropType = typename RamsesToPropertyTypeTraits<RamsesElementType>::PropType;
	std::vector<RamsesElementType> data;
	if (cont.hasProperty(propName)) {
		const ValueBase* prop = cont.get(propName);
		const ArrayBase& array = prop->asArray();
		for (size_t index = 0; index < array.size(); index++) {
			data.emplace_back(array.get(index)->as<PropType>());
		}
	}
	return data;
}

std::vector<std::vector<float>> getSamplerDataArrayVec(data_storage::Table& cont, const char* propName) {
	std::vector<std::vector<float>> data;
	if (cont.hasProperty(propName)) {
		const ValueBase* prop = cont.get(propName);
		const ArrayBase& array = prop->asArray();
		for (size_t index = 0; index < array.size(); index++) {
			const auto& component = array.get(index)->asArray();
			std::vector<float> v;
			for (size_t compIndex = 0; compIndex < component.size(); compIndex++) {
				v.emplace_back(component.get(compIndex)->asDouble());
			}
			data.emplace_back(v);
		}
	}
	return data;
}

template <typename RamsesElementType, int N>
std::vector<glm::vec<N, RamsesElementType, glm::defaultp>> getSamplerDataArrayGlm(data_storage::Table& data, const char* propName) {
	if (data.hasProperty(propName)) {
		const ValueBase* prop = data.get(propName);
		const ArrayBase& array = prop->asArray();
		using ComponentType = glm::vec<N, RamsesElementType, glm::defaultp>;
		using PropType = typename RamsesToPropertyTypeTraits<RamsesElementType>::PropType;
		std::vector<ComponentType> data;
		for (size_t index = 0; index < array.size(); index++) {
			const auto& component = array.get(index)->asArray();
			ComponentType v;
			for (size_t compIndex = 0; compIndex < N; compIndex++) {
				v[compIndex] = component.get(compIndex)->as<PropType>();
			}
			data.emplace_back(v);
		}
		return data;
	}
	return {};
}

template <typename RamsesElementType, int N>
AnimationSamplerData::OutputDataVariant makeOutputData(data_storage::Table& data) {
	using ComponentType = glm::vec<N, RamsesElementType, glm::defaultp>;

	return AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<ComponentType>{
		getSamplerDataArrayGlm<RamsesElementType, N>(data, AnimationChannelRaco::PROPNAME_KEYFRAMES),
		getSamplerDataArrayGlm<RamsesElementType, N>(data, AnimationChannelRaco::PROPNAME_TANGENTS_IN),
		getSamplerDataArrayGlm<RamsesElementType, N>(data, AnimationChannelRaco::PROPNAME_TANGENTS_OUT)});
}


void AnimationChannelRaco::updateSamplerData(BaseContext& context) {
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	if (data_->size() == 0) {
		currentSamplerData_.reset();
		return;
	}

	std::vector<float> timeStamps = getSamplerDataArray<float>(*data_, PROPNAME_TIME_STAMPS);

	AnimationSamplerData::OutputDataVariant outputData;

	switch (static_cast<core::EnginePrimitive>(*componentType_)) {
		case core::EnginePrimitive::Double:
			outputData = AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<float>{
				getSamplerDataArray<float>(*data_, PROPNAME_KEYFRAMES), getSamplerDataArray<float>(*data_, PROPNAME_TANGENTS_IN), getSamplerDataArray<float>(*data_, PROPNAME_TANGENTS_OUT)});
			break;

		case core::EnginePrimitive::Array: {
			outputData = AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<std::vector<float>>{
				getSamplerDataArrayVec(*data_, PROPNAME_KEYFRAMES), getSamplerDataArrayVec(*data_, PROPNAME_TANGENTS_IN), getSamplerDataArrayVec(*data_, PROPNAME_TANGENTS_OUT)});
			break;
		}

		case core::EnginePrimitive::Vec2f:
			outputData = makeOutputData<float, 2>(*data_);
			break;

		case core::EnginePrimitive::Vec3f:
			outputData = makeOutputData<float, 3>(*data_);
			break;

		case core::EnginePrimitive::Vec4f:
			outputData = makeOutputData<float, 4>(*data_);
			break;

		case core::EnginePrimitive::Int32:
			outputData = AnimationSamplerData::OutputDataVariant(core::AnimationOutputData<int>{
				getSamplerDataArray<int>(*data_, PROPNAME_KEYFRAMES), getSamplerDataArray<int>(*data_, PROPNAME_TANGENTS_IN), getSamplerDataArray<int>(*data_, PROPNAME_TANGENTS_OUT)});
			break;

		case core::EnginePrimitive::Vec2i:
			outputData = makeOutputData<int, 2>(*data_);
			break;

		case core::EnginePrimitive::Vec3i:
			outputData = makeOutputData<int, 3>(*data_);
			break;

		case core::EnginePrimitive::Vec4i:
			outputData = makeOutputData<int, 4>(*data_);
			break;

		default:
			assert(false);
	}

	currentSamplerData_ = std::make_shared<core::AnimationSamplerData>(core::AnimationSamplerData{
		static_cast<core::MeshAnimationInterpolation>(*interpolationType_),
		static_cast<core::EnginePrimitive>(*componentType_),
		static_cast<size_t>(*componentArraySize_),
		timeStamps, outputData});
}

int AnimationChannelRaco::getOuputComponentSize() const {
	switch (static_cast<EnginePrimitive>(*componentType_)) {
		case EnginePrimitive::Double:
			return 1;
			break;
		case EnginePrimitive::Vec2f:
			return 2;
			break;
		case EnginePrimitive::Vec3f:
			return 3;
			break;
		case EnginePrimitive::Vec4f:
			return 4;
			break;
		case EnginePrimitive::Int32:
			return 1;
			break;
		case EnginePrimitive::Vec2i:
			return 2;
			break;
		case EnginePrimitive::Vec3i:
			return 3;
			break;
		case EnginePrimitive::Vec4i:
			return 4;
			break;
		case EnginePrimitive::Array:
			return *componentArraySize_;
			break;
	}
	return 0;
}

void AnimationChannelRaco::onAfterContextActivated(BaseContext& context) {
	validate(context);
	updateSamplerData(context);
}

void AnimationChannelRaco::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&AnimationChannelRaco::componentType_) || value.isRefToProp(&AnimationChannelRaco::interpolationType_) || value.isRefToProp(&AnimationChannelRaco::componentArraySize_)) {
		validate(context);
		adjustDataArray(context);
		updateSamplerData(context);
	} else if (ValueHandle(shared_from_this(), &AnimationChannelRaco::data_).contains(value)) {
		updateSamplerData(context);
	}
}

void AnimationChannelRaco::validate(BaseContext& context) {
	bool quaternionInterpolation = (*interpolationType_ == static_cast<int>(core::MeshAnimationInterpolation::Linear_Quaternion)) || (*interpolationType_ == static_cast<int>(core::MeshAnimationInterpolation::CubicSpline_Quaternion));

	ValueHandle interpolationHandle(shared_from_this(), &AnimationChannelRaco::interpolationType_);

	if (quaternionInterpolation && *componentType_ != static_cast<int>(core::EnginePrimitive::Vec4f)) {
		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, interpolationHandle, "Primitive type must be Vec4f if quaternion interpolation is used.");
	} else {
		context.errors().removeError(interpolationHandle);
	}
}

void AnimationChannelRaco::adjustDataArray(BaseContext& context) {
	ValueHandle dataHandle({shared_from_this(), &AnimationChannelRaco::data_});
	context.removeAllProperties(dataHandle);
}

}  // namespace raco::user_types