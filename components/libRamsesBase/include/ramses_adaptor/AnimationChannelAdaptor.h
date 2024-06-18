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

#include "core/Context.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/AnimationChannel.h"

namespace raco::ramses_adaptor {


class AnimationChannelAdaptor final : public UserTypeObjectAdaptor<user_types::AnimationChannelBase> {
public:
	explicit AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, user_types::SAnimationChannelBase channel);

	bool sync(core::Errors* errors) override;

	ramses_base::RamsesAnimationChannelHandle handle() const;
	std::vector<ExportInformation> getExportInformation() const override;

private:
	ramses_base::RamsesAnimationChannelHandle handle_;

	std::array<components::Subscription, 1> subscriptions_;
	components::Subscription previewDirtySubscription_;
};

};	// namespace raco::ramses_adaptor
