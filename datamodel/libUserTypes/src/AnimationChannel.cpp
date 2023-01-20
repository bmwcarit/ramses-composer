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

raco::core::PropertyInterface AnimationChannel::getOutputProperty() const {
	if (currentSamplerData_->getOutputComponentType() == EnginePrimitive::Array) {
		return PropertyInterface::makeArrayOf(objectName(), EnginePrimitive::Double, currentSamplerData_->getOutputComponentSize());
	}
	return {objectName(), currentSamplerData_->getOutputComponentType()};
}

void AnimationChannel::updateFromExternalFile(BaseContext& context) {
	context.errors().removeError({shared_from_this()});
	context.errors().removeError({shared_from_this(), &AnimationChannel::uri_});
	context.errors().removeError({shared_from_this(), &AnimationChannel::animationIndex_});
	context.errors().removeError({shared_from_this(), &AnimationChannel::samplerIndex_});

	currentSamplerData_.reset();
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	auto uriAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uri"}});
	if (!(validateURI(context, {shared_from_this(), &AnimationChannel::uri_})) || uriAbsPath.empty()) {
		return;
	}

	auto scenegraph = context.meshCache()->getMeshScenegraph(uriAbsPath);
	if (!scenegraph) {
		auto fileErrorText = context.meshCache()->getMeshError(uriAbsPath);
		auto errorText = fileErrorText.empty() ? "Selected Animation Source file is not valid."
											   : fmt::format("Error while loading Animation Source:\n\n{}", fileErrorText);
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), {"uri"}}, errorText);
		return;
	}

	auto animationAmount = scenegraph->animations.size();
	if (animationAmount == 0) {
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::WARNING, ValueHandle{shared_from_this(), {"uri"}}, "Selected Animation Source does not contain animations.");		
		return;
	}

	auto animIndex = ValueHandle{shared_from_this(), &AnimationChannel::animationIndex_}.asInt();
	if (animIndex < 0 || animIndex >= animationAmount) {
		auto errorText = fmt::format("Selected animation index is outside of valid range [{}, {}]", 0, animationAmount - 1);
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{shared_from_this(), &AnimationChannel::animationIndex_}, errorText);
		return;
	}

	auto samplerAmount = scenegraph->animationSamplers[animIndex].size();
	auto animSamplerIndex = ValueHandle{shared_from_this(), &AnimationChannel::samplerIndex_}.asInt();
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

		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), {"uri"}}, errorText);
		currentSamplerData_.reset();
		return;
	}

	if (currentSamplerData_->input.empty()) {
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), &AnimationChannel::samplerIndex_}, "Selected animation sampler does not contain valid input data.");
		currentSamplerData_.reset();
		return;
	}

	if (currentSamplerData_->output.empty()) {
		context.errors().addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {shared_from_this(), &AnimationChannel::samplerIndex_}, "Selected animation sampler does not contain valid output data.");
		currentSamplerData_.reset();
		return;
	}

	bool unsupportedArray = currentSamplerData_->getOutputComponentType() == EnginePrimitive::Array && context.project()->featureLevel() < 4;

	createSamplerInfoBox(context, animationAmount, samplerAmount, unsupportedArray);
}

void AnimationChannel::createSamplerInfoBox(BaseContext& context, int animationAmount, int samplerAmount, bool unsupportedArray) {
	std::string samplerInfoText;
	if (unsupportedArray) {
		samplerInfoText.append(fmt::format("Can't create RamsesLogic DataArrays for AnimationChannel '{}': array samplers are only supported at feature level >= 4.\n\n", objectName()));
	}
	samplerInfoText.append(fmt::format("Current Animation: {} of {}\n", *animationIndex_ + 1, animationAmount));
	samplerInfoText.append(fmt::format("Current Sampler: {} of {}\n\n", *samplerIndex_ + 1, samplerAmount));

	samplerInfoText.append(fmt::format("Keyframes (non-interpolated): {}\n", currentSamplerData_->input.size()));
	auto type = currentSamplerData_->getOutputComponentType();
	samplerInfoText.append(fmt::format("Component Type: {}\n", type));
	if (type == EnginePrimitive::Array) {
		samplerInfoText.append(fmt::format("Component Size: {}\n", currentSamplerData_->getOutputComponentSize()));
	}
	samplerInfoText.append(fmt::format("Interpolation: {}\n", currentSamplerData_->interpolation));
	samplerInfoText.append(fmt::format("Start: {:.2f} s\n", currentSamplerData_->input.front()));
	samplerInfoText.append(fmt::format("End: {:.2f} s\n", currentSamplerData_->input.back()));
	samplerInfoText.append(fmt::format("Duration: {:.2f} s", currentSamplerData_->input.back() - currentSamplerData_->input.front()));

	context.errors().addError(raco::core::ErrorCategory::GENERAL, unsupportedArray ? raco::core::ErrorLevel::ERROR : raco::core::ErrorLevel::INFORMATION, 
		{shared_from_this()}, samplerInfoText);
}

}  // namespace raco::user_types