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

AnimationChannelAdaptor::AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, user_types::SAnimationChannelBase channel)
	: UserTypeObjectAdaptor{sceneAdaptor, channel},
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnimationChannel::objectName_}, [this]() { tagDirty(); })},
	  previewDirtySubscription_{sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })} {
}

bool AnimationChannelAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	handle_.reset();

	if (auto animSampler = editorObject_->currentSamplerData_) {
		auto objectID = editorObject_->objectIDAsRamsesLogicID();

		handle_.reset(new ramses_base::RamsesAnimationChannelData);
		handle_->name = editorObject_->objectName();
		handle_->keyframeTimes = ramsesDataArray(animSampler->timeStamps, &sceneAdaptor_->logicEngine(), handle_->name + ".timestamps", objectID);

		handle_->interpolationType = ramses_base::enumerationTranslationAnimationInterpolationType.at(animSampler->interpolation);

		std::visit(
			[this, objectID](const auto& data) {
				createRamsesDataArrays(handle_, &sceneAdaptor_->logicEngine(), data.tangentsIn, data.keyFrames, data.tangentsOut, objectID);
			},
			animSampler->output);
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
