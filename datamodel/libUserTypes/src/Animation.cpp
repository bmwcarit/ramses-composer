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
	// Only set default animation channel amount when animationChannels is empty (ie. created by user)
	// TODO: the initial creation of the channel should be in the constructor.
	// BUT: that doesn't work since the deserialization will not handle this case correctly.
	// The deserialization will only create but not remove properties in Tables.
	// Animation objects with less than ANIMATION_CHANNEL_AMOUNT channels created by BaseContext::insertAssetScenegraph
	// will then be loaded incorrectly.
	// We would need to fix the serialization if we want to move the setChannelAmount to the constructor.
	if (animationChannels_.asTable().size() == 0) {
		setChannelAmount(ANIMATION_CHANNEL_AMOUNT);
	}
	syncOutputInterface(context);
}

void Animation::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	syncOutputInterface(context);
}

void Animation::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	const auto &channelTable = animationChannels_.asTable();
	for (auto channelIndex = 0; channelIndex < channelTable.size(); ++channelIndex) {
		if (value == ValueHandle{shared_from_this(), {"animationChannels", fmt::format("Channel {}", channelIndex)}}) {
			syncOutputInterface(context);
			return;
		}
	}

	if (value == ValueHandle(shared_from_this(), &Animation::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

void Animation::syncOutputInterface(BaseContext& context) {
	PropertyInterfaceList outputs{};

	OutdatedPropertiesStore dummyCache{};

	auto &channelTable = animationChannels_.asTable();
	for (auto channelIndex = 0; channelIndex < channelTable.size(); ++channelIndex) {
		context.errors().removeError({shared_from_this(), {"animationChannels", fmt::format("Channel {}", channelIndex)}});
		auto channelRef = channelTable[channelIndex]->asRef();
		if (channelRef) {
			auto samp = channelRef->as<raco::user_types::AnimationChannel>();
			if (samp->currentSamplerData_) {
				auto prop = samp->getOutputProperty();
				prop.name = createAnimChannelOutputName(channelIndex, prop.name);
				outputs.emplace_back(prop);
			} else {
				context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, {shared_from_this(), {"animationChannels", fmt::format("Channel {}", channelIndex)}}, fmt::format("Invalid animation channel."));
			}
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
	animationChannels_->clear();
	for (auto i = 0; i < amount; ++i) {
		animationChannels_->addProperty(fmt::format("Channel {}", i), new Value<SAnimationChannel>());
	}
}

}  // namespace raco::user_types