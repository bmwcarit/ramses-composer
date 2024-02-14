/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AnimationChannelAdaptor.h"

#include "ramses_adaptor/MeshAdaptor.h"

namespace {

template <typename DataType>
void createRamsesDataArrays(const raco::ramses_base::RamsesAnimationChannelHandle& handle, ramses::LogicEngine* engine, const DataType& tangentInData, const DataType& outputData, const DataType& tangentOutData, const std::pair<uint64_t, uint64_t>& objectID) {
	auto outputName = handle->name + ".keyframes";
	auto tangentInName = handle->name + ".tangentIn";
	auto tangentOutName = handle->name + ".tangentOut";

	handle->animOutput = raco::ramses_base::ramsesDataArray(outputData, engine, outputName, objectID);
	if (!tangentInData.empty()) {
		handle->tangentIn = raco::ramses_base::ramsesDataArray(tangentInData, engine, tangentInName, objectID);
	}
	if (!tangentOutData.empty()) {
		handle->tangentOut = raco::ramses_base::ramsesDataArray(tangentOutData, engine, tangentOutName, objectID);
	}
}

}  // namespace

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

AnimationChannelAdaptor::AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, user_types::SAnimationChannel channel)
	: UserTypeObjectAdaptor{sceneAdaptor, channel},
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::animationIndex_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::uri_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::samplerIndex_}, [this]() { tagDirty(); })},
	  previewDirtySubscription_{sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })} {
}

std::vector<glm::vec3> convert_vec3(const std::vector<std::vector<float>>& data) {
	std::vector<glm::vec3> result;
	for (size_t index = 0; index < data.size(); index++) {
		const auto& v = data[index];
		result.emplace_back(glm::vec3(v[0], v[1], v[2]));
	}
	return result;
}

std::vector<glm::vec4> convert_vec4(const std::vector<std::vector<float>>& data) {
	std::vector<glm::vec4> result;
	for (size_t index = 0; index < data.size(); index++) {
		const auto& v = data[index];
		result.emplace_back(glm::vec4(v[0], v[1], v[2], v[3]));
	}
	return result;
}

bool AnimationChannelAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	handle_.reset();

	if (auto animSampler = editorObject_->currentSamplerData_) {
		auto objectID = editorObject_->objectIDAsRamsesLogicID();

		handle_.reset(new ramses_base::RamsesAnimationChannelData);
		handle_->name = editorObject_->objectName();
		handle_->keyframeTimes = ramsesDataArray(animSampler->timeStamps, &sceneAdaptor_->logicEngine(), handle_->name + ".timestamps", objectID);

		std::map<core::MeshAnimationInterpolation, ramses::EInterpolationType> interpolationTypeMap = {
			{core::MeshAnimationInterpolation::Linear, ramses::EInterpolationType::Linear},
			{core::MeshAnimationInterpolation::CubicSpline, ramses::EInterpolationType::Cubic},
			{core::MeshAnimationInterpolation::Step, ramses::EInterpolationType::Step},
			{core::MeshAnimationInterpolation::Linear_Quaternion, ramses::EInterpolationType::Linear_Quaternions},
			{core::MeshAnimationInterpolation::CubicSpline_Quaternion, ramses::EInterpolationType::Cubic_Quaternions}
		};

		handle_->interpolationType = interpolationTypeMap.at(animSampler->interpolation);

		switch (animSampler->componentType) {
			case core::EnginePrimitive::Array: {
				// Morph target weights
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), animSampler->tangentsIn, animSampler->keyFrames, animSampler->tangentsOut, objectID);
				break;
			}
			case core::EnginePrimitive::Vec3f: {
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), convert_vec3(animSampler->tangentsIn), convert_vec3(animSampler->keyFrames), convert_vec3(animSampler->tangentsOut), objectID);
				break;
			}
			case core::EnginePrimitive::Vec4f: {
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), convert_vec4(animSampler->tangentsIn), convert_vec4(animSampler->keyFrames), convert_vec4(animSampler->tangentsOut), objectID);
				break;
			}
			default:
				assert(false);
		}
	}

	tagDirty(false);
	return true;
}

ramses_base::RamsesAnimationChannelHandle AnimationChannelAdaptor::handle() const {
	return handle_;
}

std::vector<ExportInformation> AnimationChannelAdaptor::getExportInformation() const {
	if (handle_ == nullptr) {
		return {};
	}

	auto result = std::vector<ExportInformation>();

	if (handle_->keyframeTimes != nullptr) {
		result.emplace_back("DataArray", handle_->keyframeTimes->getName());
	}

	if (handle_->animOutput != nullptr) {
		result.emplace_back("DataArray", handle_->animOutput->getName());
	}

	if (handle_->tangentIn != nullptr) {
		result.emplace_back("DataArray", handle_->tangentIn->getName());
	}

	if (handle_->tangentOut != nullptr) {
		result.emplace_back("DataArray", handle_->tangentOut->getName());
	}

	return result;
}

};	// namespace raco::ramses_adaptor
