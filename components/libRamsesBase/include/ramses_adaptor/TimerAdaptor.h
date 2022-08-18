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
#include "user_types/Timer.h"


namespace raco::ramses_adaptor {

class TimerAdaptor final : public UserTypeObjectAdaptor<user_types::Timer>, public ILogicPropertyProvider {
public:
	TimerAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::STimer timer);

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;

	bool sync(core::Errors* errors) override;
	void readDataFromEngine(core::DataChangeRecorder& recorder);

private:
	raco::ramses_base::RamsesTimerNode timerNode_;
	raco::components::Subscription dirtySubscription_;
	raco::components::Subscription nameSubscription_;
	raco::components::Subscription inputSubscription_;
};

};	// namespace raco::ramses_adaptor