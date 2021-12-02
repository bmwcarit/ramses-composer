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

#include "core/Context.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/AnimationChannel.h"

namespace raco::ramses_adaptor {


class AnimationChannelAdaptor final : public ObjectAdaptor {
public:
	explicit AnimationChannelAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SAnimationChannel mesh);

	core::SEditorObject baseEditorObject() noexcept override;
	const core::SEditorObject baseEditorObject() const noexcept override;

	bool sync(core::Errors* errors) override;
	raco::ramses_base::RamsesAnimationChannelHandle handle_;

private:
	user_types::SAnimationChannel editorObject_;
	std::array<raco::components::Subscription, 4> subscriptions_;
	raco::components::Subscription previewDirtySubscription_;
};

};	// namespace raco::ramses_adaptor
