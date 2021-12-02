/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "user_types/BaseObject.h"

#include "user_types/AnimationChannel.h"

namespace raco::user_types {

class Animation : public BaseObject {
public:
	static inline const auto ANIMATION_CHANNEL_AMOUNT = 8;

	static inline const TypeDescriptor typeDescription = { "Animation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Animation(Animation const& other)
		: BaseObject(other),
		  play_(other.play_),
		  loop_(other.loop_),
		  rewindOnStop_(other.rewindOnStop_),
		  animationChannels(other.animationChannels),
		  animationOutputs(other.animationOutputs)
	{
		fillPropertyDescription();
	}

	Animation(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("play", &play_);
		properties_.emplace_back("loop", &loop_);
		properties_.emplace_back("rewindOnStop", &rewindOnStop_);
		properties_.emplace_back("animationChannels", &animationChannels);
		properties_.emplace_back("animationOutputs", &animationOutputs);
	}

	void onBeforeDeleteObject(Errors& errors) const override;

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void syncOutputInterface(BaseContext& context);
	std::string createAnimChannelOutputName(int channelIndex, const std::string& channelName);
	void setChannelAmount(int amount);

	// This property stays hidden until we figure out a proper Ramses-Logic-based time concept.
	//Property<double, DisplayNameAnnotation> speed_{1.0, {}, {"Speed Multiplier"}};

	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> play_{false, {"Play"}, {}};
	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> loop_{false, {"Loop"}, {}};
	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> rewindOnStop_{false, {"Rewind on Pause"}, {}};
	Property<Table, DisplayNameAnnotation> animationChannels{{}, {"Animation Channels"}};
	Property<Table, DisplayNameAnnotation> animationOutputs{{}, {"Outputs"}};

private:
};

using SAnimation = std::shared_ptr<Animation>;

}
