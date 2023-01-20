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
void createRamsesDataArrays(const raco::ramses_base::RamsesAnimationChannelHandle& handle, rlogic::LogicEngine* engine, const DataType& tangentInData, const DataType& outputData, const DataType& tangentOutData, const std::pair<uint64_t, uint64_t>& objectID) {
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

AnimationChannelAdaptor::AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SAnimationChannel channel)
	: UserTypeObjectAdaptor{sceneAdaptor, channel},
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::animationIndex_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::uri_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::samplerIndex_}, [this]() { tagDirty(); })},
	  previewDirtySubscription_{sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })} {
}

bool AnimationChannelAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	handle_.reset();

	if (auto animSampler = editorObject_->currentSamplerData_) {
		auto objectID = editorObject_->objectIDAsRamsesLogicID();

		handle_.reset(new raco::ramses_base::RamsesAnimationChannelData);
		handle_->name = editorObject_->objectName();
		handle_->keyframeTimes = ramsesDataArray(animSampler->input, &sceneAdaptor_->logicEngine(), handle_->name + ".timestamps", objectID);

		std::map<raco::core::MeshAnimationInterpolation, rlogic::EInterpolationType> interpolationTypeMap = {
			{raco::core::MeshAnimationInterpolation::Linear, rlogic::EInterpolationType::Linear},
			{raco::core::MeshAnimationInterpolation::CubicSpline, rlogic::EInterpolationType::Cubic},
			{raco::core::MeshAnimationInterpolation::Step, rlogic::EInterpolationType::Step},
			{raco::core::MeshAnimationInterpolation::Linear_Quaternion, rlogic::EInterpolationType::Linear_Quaternions},
			{raco::core::MeshAnimationInterpolation::CubicSpline_Quaternion, rlogic::EInterpolationType::Cubic_Quaternions}
		};

		handle_->interpolationType = interpolationTypeMap.at(animSampler->interpolation);

		switch (animSampler->getOutputComponentType()) {
			case raco::core::EnginePrimitive::Array: {
				// Morph target weights
				// We can create DataArrays with std::vector<float> only with feature level >= 4, so we
				// switch this off at smaller feature levels.
				// Strictly this breaks backwards compatibility but morphing was not supported anyway so this should be OK.

				if (sceneAdaptor_->featureLevel() >= 4) {
					const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<std::vector<float>>();
					createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
				} else {
					handle_.reset();
				}
				break;
			}
			case raco::core::EnginePrimitive::Vec3f: {
				const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<rlogic::vec3f>();
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
				break;
			}
			case raco::core::EnginePrimitive::Vec4f: {
				const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<rlogic::vec4f>();
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
				break;
			}
			default:
				assert(false);
		}
	}

	tagDirty(false);
	return true;
}

raco::ramses_base::RamsesAnimationChannelHandle AnimationChannelAdaptor::handle() const {
	return handle_;
}

std::vector<ExportInformation> AnimationChannelAdaptor::getExportInformation() const {
	if (handle_ == nullptr) {
		return {};
	}

	auto result = std::vector<ExportInformation>();

	if (handle_->keyframeTimes != nullptr) {
		result.emplace_back("DataArray", handle_->keyframeTimes->getName().data());
	}

	if (handle_->animOutput != nullptr) {
		result.emplace_back("DataArray", handle_->animOutput->getName().data());
	}

	if (handle_->tangentIn != nullptr) {
		result.emplace_back("DataArray", handle_->tangentIn->getName().data());
	}

	if (handle_->tangentOut != nullptr) {
		result.emplace_back("DataArray", handle_->tangentOut->getName().data());
	}

	return result;
}

};	// namespace raco::ramses_adaptor
