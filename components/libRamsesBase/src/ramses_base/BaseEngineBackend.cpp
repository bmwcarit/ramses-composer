/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_base/BaseEngineBackend.h"
#include "ramses_base/BuildOptions.h"

#include "log_system/log.h"

namespace raco::ramses_base {

BaseEngineBackend::BaseEngineBackend(
	const ramses::RamsesFrameworkConfig& frameworkConfig,
	const char* applicationName)
	: framework_{frameworkConfig},
	  client_{framework_.createClient(applicationName), [=](ramses::RamsesClient* c) { framework_.destroyClient(*c); }},
	  scene_{client_->createScene(BuildOptions::internalSceneId), [this](ramses::Scene* scene) { client_->destroy(*scene); }},
	  coreInterface_{this} {
}

BaseEngineBackend::~BaseEngineBackend() {
	if (framework_.isConnected()) {
		framework_.disconnect();
	}
}

bool BaseEngineBackend::connect() {
	if (framework_.connect() != ramses::StatusOK) {
		LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Connection to ramses::RamsesFramework failed.");
		return true;
	} else {
		LOG_INFO(raco::log_system::RAMSES_BACKEND, "Connection to ramses::RamsesFramework successful.");
		return false;
	}
}

ramses::RamsesFramework& BaseEngineBackend::framework() {
	return framework_;
}

ramses::Scene& BaseEngineBackend::internalScene() {
	return *scene_.get();
}

ramses::RamsesClient& BaseEngineBackend::client() {
	return *client_.get();
}

LogicEngine& BaseEngineBackend::logicEngine() {
	return logicEngine_;
}

raco::core::EngineInterface* BaseEngineBackend::coreInterface() {
	return &coreInterface_;
}

}  // namespace raco::ramses_base
