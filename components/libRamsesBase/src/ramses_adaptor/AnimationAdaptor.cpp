/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AnimationAdaptor.h"

#include "ramses_adaptor/AnimationChannelAdaptor.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

AnimationAdaptor::AnimationAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SAnimation animation)
	: UserTypeObjectAdaptor{sceneAdaptor, animation},
	  animNode_{sceneAdaptor_->defaultAnimation()},
	  progressSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::Animation::progress_}, [this]() { tagDirty(); })},
	  dirtySubscription_{
		  sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })},
	  nameSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::Animation::objectName_}, [this]() { tagDirty(); })}
 {
}

void AnimationAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	logicNodes.emplace_back(animNode_->get());
}

const rlogic::Property* AnimationAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector) {
	if (propertyNamesVector.size() > 1) {
		const rlogic::Property* prop{(*animNode_)->getOutputs()};
		// The first element in the names is the output container
		for (size_t i = 1; i < propertyNamesVector.size(); i++) {
			prop = prop->getChild(propertyNamesVector.at(i));
		}
		return prop;
	} else if (propertyNamesVector.size() == 1) {
		return (*animNode_)->getInputs()->getChild(propertyNamesVector.front());
	}
	return nullptr;
}

void AnimationAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR, level, valueHandle, message);
}

bool AnimationAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject_->shared_from_this()});

	std::vector<raco::ramses_base::RamsesAnimationChannelHandle> newChannelHandles;

	const auto &channelTable = editorObject_->animationChannels.asTable();
	for (auto channelIndex = 0; channelIndex < channelTable.size(); ++channelIndex) {
		auto channel = channelTable.get(channelIndex)->asRef();
		auto channelAdaptor = sceneAdaptor_->lookup<raco::ramses_adaptor::AnimationChannelAdaptor>(channel);
		if (channelAdaptor && channelAdaptor->handle()) {
			newChannelHandles.emplace_back(channelAdaptor->handle());
		} else {
			newChannelHandles.emplace_back(nullptr);
		}
	}

	if (animNode_->channels() != newChannelHandles || (**animNode_).getName() != editorObject_->objectName()) {
		rlogic::AnimationNodeConfig config;
		for (auto i = 0; i < newChannelHandles.size(); i++) {
			auto& newHandle = newChannelHandles[i];
			if (newHandle) {
				config.addChannel({editorObject_->createAnimChannelOutputName(i, newHandle->name),
					newHandle->keyframeTimes.get(),
					newHandle->animOutput.get(),
					newHandle->interpolationType,
					newHandle->tangentIn.get(),
					newHandle->tangentOut.get()});
			}
		}

		animNode_ = raco::ramses_base::ramsesAnimationNode(&sceneAdaptor_->logicEngine(), config, newChannelHandles, editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID());
		if (!animNode_) {
			animNode_ = sceneAdaptor_->defaultAnimation();
		}
		updateGlobalAnimationStats(errors);
	}

	updateGlobalAnimationSettings();

	tagDirty(false);
	return true;
}

void AnimationAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	core::ValueHandle animOutputs{editorObject_, &user_types::Animation::animationOutputs};
	getOutputFromEngine(*(*animNode_)->getOutputs(), animOutputs, recorder);
}

void AnimationAdaptor::updateGlobalAnimationSettings() {
	(*animNode_)->getInputs()->getChild("progress")->set<float>(*editorObject_->progress_);
}

void AnimationAdaptor::updateGlobalAnimationStats(core::Errors* errors) {
	auto duration = (*animNode_)->getOutputs()->getChild("duration")->get<float>().value();
	auto infoText = fmt::format("Total Duration: {:.2f} s", duration);
	errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::INFORMATION, {editorObject_->shared_from_this(), {"animationOutputs"}}, infoText);
}

};	// namespace raco::ramses_adaptor
