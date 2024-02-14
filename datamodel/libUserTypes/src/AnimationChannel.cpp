/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/AnimationChannel.h"

#include "core/Errors.h"
#include "core/CoreFormatter.h"

#include "Validation.h"

namespace raco::user_types {

void AnimationChannel::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&AnimationChannel::animationIndex_) || value.isRefToProp(&AnimationChannel::samplerIndex_)) {
		updateFromExternalFile(context);
	}
}

core::PropertyInterface AnimationChannel::getOutputProperty() const {
	if (currentSamplerData_->componentType == EnginePrimitive::Array) {
		return PropertyInterface::makeArrayOf(objectName(), EnginePrimitive::Double, currentSamplerData_->getOutputComponentSize());
	}
	return {objectName(), currentSamplerData_->componentType};
}

void AnimationChannel::updateFromExternalFile(BaseContext& context) {
	// TODO(error) try to avoid unnecessary error item updates
	context.errors().removeError({shared_from_this()});

	ValueHandle uriHandle{shared_from_this(), &AnimationChannel::uri_};
	context.errors().removeError({shared_from_this(), &AnimationChannel::animationIndex_});
	context.errors().removeError({shared_from_this(), &AnimationChannel::samplerIndex_});

	currentSamplerData_.reset();
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	if (!validateURI(context, uriHandle)) {
		return;
	}

	auto uriAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), uriHandle);
	auto scenegraph = context.meshCache()->getMeshScenegraph(uriAbsPath);
	if (!scenegraph) {
		auto fileErrorText = context.meshCache()->getMeshError(uriAbsPath);
		auto errorText = fileErrorText.empty() ? "Selected Animation Source file is not valid."
											   : fmt::format("Error while loading Animation Source:\n\n{}", fileErrorText);
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, uriHandle, errorText);
		return;
	}

	auto animationAmount = scenegraph->animations.size();
	if (animationAmount == 0) {
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::WARNING, uriHandle, "Selected Animation Source does not contain animations.");		
		return;
	}

	auto animIndex = *animationIndex_;
	if (animIndex < 0 || animIndex >= animationAmount) {
		auto errorText = fmt::format("Selected animation index is outside of valid range [{}, {}]", 0, animationAmount - 1);
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{shared_from_this(), &AnimationChannel::animationIndex_}, errorText);
		return;
	}

	auto samplerAmount = scenegraph->animationSamplers[animIndex].size();
	auto animSamplerIndex = *samplerIndex_;
	if (animSamplerIndex < 0 || animSamplerIndex >= samplerAmount) {
		auto errorText = fmt::format("Selected animation sampler index is outside of valid range [{}, {}]", 0, samplerAmount - 1);
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{shared_from_this(), &AnimationChannel::samplerIndex_}, errorText);
		return;
	}

	currentSamplerData_ = context.meshCache()->getAnimationSamplerData(uriAbsPath, animIndex, animSamplerIndex);
	if (!currentSamplerData_) {
		auto fileErrorText = context.meshCache()->getMeshError(uriAbsPath);
		auto errorText = fileErrorText.empty() ? "Selected Animation Source does not contain valid animation samplers."
											   : fmt::format("Error while loading Animation Source:\n\n{}", fileErrorText);

		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, uriHandle, errorText);
		currentSamplerData_.reset();
		return;
	}

	if (currentSamplerData_->timeStamps.empty()) {
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), &AnimationChannel::samplerIndex_}, "Selected animation sampler does not contain valid input data.");
		currentSamplerData_.reset();
		return;
	}

	if (currentSamplerData_->keyFrames.empty()) {
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), &AnimationChannel::samplerIndex_}, "Selected animation sampler does not contain valid output data.");
		currentSamplerData_.reset();
		return;
	}

	createSamplerInfoBox(context, animationAmount, samplerAmount);
}

void AnimationChannel::createSamplerInfoBox(BaseContext& context, int animationAmount, int samplerAmount) {
	std::string samplerInfoText;
	samplerInfoText.append(fmt::format("Current Animation: {} of {}\n", *animationIndex_ + 1, animationAmount));
	samplerInfoText.append(fmt::format("Current Sampler: {} of {}\n\n", *samplerIndex_ + 1, samplerAmount));

	samplerInfoText.append(fmt::format("Keyframes (non-interpolated): {}\n", currentSamplerData_->timeStamps.size()));
	auto type = currentSamplerData_->componentType;
	samplerInfoText.append(fmt::format("Component Type: {}\n", type));
	if (type == EnginePrimitive::Array) {
		samplerInfoText.append(fmt::format("Component Size: {}\n", currentSamplerData_->getOutputComponentSize()));
	}
	samplerInfoText.append(fmt::format("Interpolation: {}\n", currentSamplerData_->interpolation));
	samplerInfoText.append(fmt::format("Start: {:.2f} s\n", currentSamplerData_->timeStamps.front()));
	samplerInfoText.append(fmt::format("End: {:.2f} s\n", currentSamplerData_->timeStamps.back()));
	samplerInfoText.append(fmt::format("Duration: {:.2f} s", currentSamplerData_->timeStamps.back() - currentSamplerData_->timeStamps.front()));

	context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {shared_from_this()}, samplerInfoText);
}

}  // namespace raco::user_types