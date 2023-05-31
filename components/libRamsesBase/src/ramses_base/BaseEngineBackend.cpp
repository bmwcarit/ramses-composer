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

const std::string BaseEngineBackend::featureLevelDescriptions =
R"(1 - LogicEngine v1.0.3
2 - LogicEngine v1.1.0
    - Added AnchorPoint
    - Added RamsesRenderPassBinding 
    - Added 'enabled' input property to RamsesNodeBinding
    - Added createRamsesCameraBindingWithFrustumPlanes
3 - LogicEngine v1.2.0
    - Added RenderGroupBinding
4 - LogicEngine v1.3.0
    - Added SkinBinding
5 - LogicEngine v1.4.0
    - Added support for LuaModules in LuaInterfaces
    - Added MeshNodeBinding
)";

BaseEngineBackend::BaseEngineBackend(
	rlogic::EFeatureLevel featureLevel, 
	const ramses::RamsesFrameworkConfig& frameworkConfig,
	const char* applicationName)
	: framework_{frameworkConfig},
	  logicEngine_(std::make_unique<rlogic::LogicEngine>(featureLevel)),
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
	LOG_INFO(raco::log_system::RAMSES_BACKEND, "LogicEngine feature level = {}.", static_cast<int>(getFeatureLevel()));
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
	return *logicEngine_;
}

raco::core::EngineInterface* BaseEngineBackend::coreInterface() {
	return &coreInterface_;
}

void BaseEngineBackend::setFeatureLevel(rlogic::EFeatureLevel newFeatureLevel) {
	if (getFeatureLevel() != newFeatureLevel) {
		logicEngine_ = std::make_unique<rlogic::LogicEngine>(newFeatureLevel);
		LOG_INFO(raco::log_system::RAMSES_BACKEND, "LogicEngine feature level = {}.", static_cast<int>(getFeatureLevel()));
	}
}

rlogic::EFeatureLevel BaseEngineBackend::getFeatureLevel() {
	return logicEngine_->getFeatureLevel();
}


}  // namespace raco::ramses_base
