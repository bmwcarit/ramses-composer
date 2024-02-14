/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/Animation.h"

#include "core/Errors.h"
#include "user_types/AnimationChannel.h"
#include "user_types/SyncTableWithEngineInterface.h"

namespace raco::user_types {

void Animation::onAfterContextActivated(BaseContext& context) {
	syncOutputInterface(context);
}

void Animation::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	syncOutputInterface(context);
}

void Animation::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (ValueHandle(shared_from_this(), &Animation::animationChannels_).contains(value)) {
		syncOutputInterface(context);
	}

	if (value == ValueHandle(shared_from_this(), &Animation::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

void Animation::syncOutputInterface(BaseContext& context) {
	PropertyInterfaceList outputs{};

	OutdatedPropertiesStore dummyCache{};

	for (auto channelIndex = 0; channelIndex < animationChannels_->size(); ++channelIndex) {
		ValueHandle channelHandle = ValueHandle(shared_from_this(), &Animation::animationChannels_)[channelIndex];
		auto channel = **animationChannels_->get(channelIndex);
		if (channel) {
			if (channel->currentSamplerData_) {
				auto prop = channel->getOutputProperty();
				prop.name = createAnimChannelOutputName(channelIndex, prop.name);
				outputs.emplace_back(prop);
				context.errors().removeError(channelHandle);
			} else {
				context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, channelHandle, "Invalid animation channel.");
			}
		} else {
			context.errors().removeError(channelHandle);
		}
	}

	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), &Animation::outputs_), dummyCache, true, false);
	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

std::string Animation::createAnimChannelOutputName(int channelIndex, const std::string& channelName) {
	return fmt::format("Ch{}.{}", channelIndex, channelName);
}

void Animation::setChannelAmount(int amount) {
	animationChannels_->resize(amount);
}

}  // namespace raco::user_types