/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
	static inline const TypeDescriptor typeDescription = { "Animation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Animation(Animation const& other)
		: BaseObject(other),
		  progress_(other.progress_),
		  animationChannels_(other.animationChannels_),
		  outputs_(other.outputs_)
	{
		fillPropertyDescription();
	}

	Animation(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
		setChannelAmount(1);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("progress", &progress_);
		properties_.emplace_back("animationChannels", &animationChannels_);
		properties_.emplace_back("outputs", &outputs_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void syncOutputInterface(BaseContext& context);
	std::string createAnimChannelOutputName(int channelIndex, const std::string& channelName);
	void setChannelAmount(int amount);

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> progress_{0.0, {"Progress"}, {0.0, 1.0}, {}};

	Property<Array<SAnimationChannelBase>, DisplayNameAnnotation, ResizableArray> animationChannels_{{}, {"Animation Channels"}, {}};
	Property<Table, DisplayNameAnnotation> outputs_{{}, {"Outputs"}};
};

using SAnimation = std::shared_ptr<Animation>;

}
