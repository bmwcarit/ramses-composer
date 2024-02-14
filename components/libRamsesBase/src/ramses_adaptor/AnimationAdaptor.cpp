/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/AnimationAdaptor.h"

#include "ramses_adaptor/AnimationChannelAdaptor.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

AnimationAdaptor::AnimationAdaptor(SceneAdaptor* sceneAdaptor, user_types::SAnimation animation)
	: UserTypeObjectAdaptor{sceneAdaptor, animation},
	  animNode_{},
	  progressSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::Animation::progress_}, [this]() { tagDirty(); })},
	  dirtySubscription_{
		  sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })},
	  nameSubscription_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::Animation::objectName_}, [this]() { tagDirty(); })} {
}

void AnimationAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	if (animNode_) {
		logicNodes.emplace_back(animNode_->get());
	}
}

ramses::Property* AnimationAdaptor::getProperty(const std::vector<std::string_view>& propertyNamesVector) {
	if (animNode_) {
		if (propertyNamesVector.size() > 1 && propertyNamesVector[0] == "outputs") {
			return ILogicPropertyProvider::getPropertyRecursive((*animNode_)->getOutputs(), propertyNamesVector, 1);
		} else if (propertyNamesVector.size() == 1) {
			return (*animNode_)->getInputs()->getChild(propertyNamesVector.front());
		}
	}
	return nullptr;
}

void AnimationAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

bool AnimationAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject_->shared_from_this()});

	std::vector<ramses_base::RamsesAnimationChannelHandle> newChannelHandles;

	for (auto channelIndex = 0; channelIndex < editorObject_->animationChannels_->size(); ++channelIndex) {
		auto channel = **editorObject_->animationChannels_->get(channelIndex);
		auto channelAdaptor = sceneAdaptor_->lookup<ramses_adaptor::AnimationChannelAdaptor>(channel);
		if (channelAdaptor && channelAdaptor->handle()) {
			newChannelHandles.emplace_back(channelAdaptor->handle());
		} else {
			newChannelHandles.emplace_back(nullptr);
		}
	}

	if (!animNode_ || animNode_->channels() != newChannelHandles || (**animNode_).getName() != editorObject_->objectName()) {
		ramses::AnimationNodeConfig config;
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
		if (!config.getChannels().empty()) {
			animNode_ = ramses_base::ramsesAnimationNode(&sceneAdaptor_->logicEngine(), config, newChannelHandles, editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID());

			if (animNode_) {
				updateGlobalAnimationStats(errors);
				errors->removeError({editorObject()});
			} else {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()},
					fmt::format("RamsesLogic Error: {}", sceneAdaptor_->scene()->getRamsesClient().getRamsesFramework().getLastError().value().message));
			}
		} else {
			animNode_.reset();
			errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()},
				fmt::format("Can't create RamsesLogic AnimationNode: Animation '{}' contains no valid AnimationChannels.", editorObject()->objectName()));
		}
	}

	updateGlobalAnimationSettings();

	tagDirty(false);
	return true;
}

void AnimationAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	if (animNode_) {
		core::ValueHandle animOutputs{editorObject_, &user_types::Animation::outputs_};
		getOutputFromEngine(*(*animNode_)->getOutputs(), animOutputs, recorder);
	}
}

void AnimationAdaptor::updateGlobalAnimationSettings() {
	if (animNode_) {
		(*animNode_)->getInputs()->getChild("progress")->set<float>(*editorObject_->progress_);
	}
}

void AnimationAdaptor::updateGlobalAnimationStats(core::Errors* errors) {
	if (animNode_) {
		auto duration = (*animNode_)->getOutputs()->getChild("duration")->get<float>().value();
		auto infoText = fmt::format("Total Duration: {:.2f} s", duration);
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION,
			{editorObject_->shared_from_this(), &user_types::Animation::outputs_}, infoText);
	}
}

std::vector<ExportInformation> AnimationAdaptor::getExportInformation() const {
	if (animNode_ == nullptr) {
		return {};
	}

	return {ExportInformation{"Animation", animNode_->get()->getName()}};
}

};	// namespace raco::ramses_adaptor
