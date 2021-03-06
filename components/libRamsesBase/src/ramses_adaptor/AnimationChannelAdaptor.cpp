/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AnimationChannelAdaptor.h"

#include "ramses_adaptor/MeshAdaptor.h"

namespace {
template <typename DataType>
void createRamsesDataArrays(const raco::ramses_base::RamsesAnimationChannelHandle& handle, rlogic::LogicEngine *engine, const DataType& tangentInData, const DataType& outputData, const DataType& tangentOutData, const std::pair<uint64_t, uint64_t> &objectID) {
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

}

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

AnimationChannelAdaptor::AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SAnimationChannel channel)
	: UserTypeObjectAdaptor{sceneAdaptor, channel},
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::animationIndex_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::uri_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::samplerIndex_}, [this]() { tagDirty(); })},
	  previewDirtySubscription_ {sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })}
{}


bool AnimationChannelAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	handle_.reset();

	if (auto animSampler = editorObject_->currentSamplerData_) {
		auto objectID = editorObject_->objectIDAsRamsesLogicID();

		handle_.reset(new raco::ramses_base::RamsesAnimationChannelData);
		handle_->name = editorObject_->objectName();
		handle_->keyframeTimes = ramsesDataArray(animSampler->input, &sceneAdaptor_->logicEngine(), handle_->name + ".timestamps", objectID);
		auto componentSize = animSampler->getOutputComponentSize();

		switch (animSampler->interpolation) {
			case raco::core::MeshAnimationInterpolation::Linear: {
				handle_->interpolationType = (componentSize == 4) ? rlogic::EInterpolationType::Linear_Quaternions : rlogic::EInterpolationType::Linear;
				break;
			}
			case raco::core::MeshAnimationInterpolation::CubicSpline: {
				handle_->interpolationType = (componentSize == 4) ? rlogic::EInterpolationType::Cubic_Quaternions : rlogic::EInterpolationType::Cubic;
				break;
			}
			case raco::core::MeshAnimationInterpolation::Step: {
				handle_->interpolationType = rlogic::EInterpolationType::Step;
				break;
			}
		}

		if (componentSize == 1) {
			// edge case: weights
			// see https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_018_MorphTargets.md
			const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<rlogic::vec2f>();

			if (outputData.size() > animSampler->input.size()) {
				// edge case of edge case: Multiple morph targets in single animation sampler
				// ramses-logic only allows output data to be the same size as input, so for now we only load the first morph target data
				// - see README of glTF example "MorphStressTest"
				auto morphTargetAmount = outputData.size() / animSampler->input.size();
				std::vector<rlogic::vec2f> singularTangentInData;
				std::vector<rlogic::vec2f> singularOutputData(animSampler->input.size());
				std::vector<rlogic::vec2f> singularTangentOutData;

				for (auto i = 0; i < animSampler->input.size(); ++i) {
					singularOutputData[i] = outputData[i * morphTargetAmount];
				}
				if (!tangentInData.empty()) {
					for (auto i = 0; i < animSampler->input.size(); ++i) {
						singularTangentInData[i] = tangentInData[i * morphTargetAmount];
					}
				}
				if (!tangentOutData.empty()) {
					for (auto i = 0; i < animSampler->input.size(); ++i) {
						singularTangentOutData[i] = tangentOutData[i * morphTargetAmount];
					}
				}

				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), singularTangentInData, singularOutputData, singularTangentOutData, objectID);
			} else {
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
			}
		} else if (componentSize == 3) {
			const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<rlogic::vec3f>();
			createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
		} else if (componentSize == 4) {
			const auto& [tangentInData, outputData, tangentOutData] = animSampler->getOutputData<rlogic::vec4f>();
			createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), tangentInData, outputData, tangentOutData, objectID);
		}
	}

	tagDirty(false);
	return true;
}

raco::ramses_base::RamsesAnimationChannelHandle AnimationChannelAdaptor::handle() const {
	return handle_;
}

};	// namespace raco::ramses_adaptor
