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
#include "user_types/Animation.h"

#include <ramses/client/logic/AnimationTypes.h>

namespace raco::ramses_adaptor {

class AnimationAdaptor final : public UserTypeObjectAdaptor<user_types::Animation>, public ILogicPropertyProvider {
public:
	explicit AnimationAdaptor(SceneAdaptor* sceneAdaptor, user_types::SAnimation animation);

	void getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const override;
	ramses::Property* getProperty(const std::vector<std::string_view>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;

	bool sync(core::Errors* errors) override;
	void readDataFromEngine(core::DataChangeRecorder& recorder);
	std::vector<ExportInformation> getExportInformation() const override;

private:
	ramses_base::RamsesAnimationNode animNode_;
	components::Subscription progressSubscription_;
	components::Subscription dirtySubscription_;
	components::Subscription nameSubscription_;

	void updateGlobalAnimationSettings();
	void updateGlobalAnimationStats(core::Errors* errors);
};

};	// namespace raco::ramses_adaptor