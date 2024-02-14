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

const ramses::EFeatureLevel BaseEngineBackend::minFeatureLevel = ramses::EFeatureLevel::EFeatureLevel_01;

// TODO if the maxFeatureLevel increases beyond 1 we need to create and/or check tests
// - feature level downgrade tests
//   - python api: in pyt_general.py -> test_load_raco_2x_feature_levels

const ramses::EFeatureLevel BaseEngineBackend::maxFeatureLevel = ramses::EFeatureLevel::EFeatureLevel_01;

const std::string BaseEngineBackend::featureLevelDescriptions =
R"(1 - Ramses v28.0.0
)";


ramses::RamsesFrameworkConfig& BaseEngineBackend::defaultRamsesFrameworkConfig() {
	// The feature level used here is just a placeholder. The real featureLevel is set in BaseEngineBackend::setup.
	static ramses::RamsesFrameworkConfig config(BaseEngineBackend::minFeatureLevel);
	config.setLogLevel("RAPI", ramses::ELogLevel::Trace);
	config.setLogLevel("RPER", ramses::ELogLevel::Off);
	config.setLogLevel("RRND", ramses::ELogLevel::Debug);
	config.setLogLevel("RFRA", ramses::ELogLevel::Off);
	config.setLogLevel("RCOM", ramses::ELogLevel::Info);
	config.setLogLevelConsole(ramses::ELogLevel::Off);
	config.setPeriodicLogInterval(std::chrono::seconds(0));
	return config;
}

BaseEngineBackend::BaseEngineBackend(const ramses::RamsesFrameworkConfig& frameworkConfig, const char* applicationName)
	: frameworkConfig_(frameworkConfig),
	  applicationName_(applicationName),
	  coreInterface_{this} {
}

BaseEngineBackend::~BaseEngineBackend() {
	reset();
}

void BaseEngineBackend::reset() {
	coreInterface_.clearModuleCache();
	if (framework_ && framework_->isConnected()) {
		framework_->disconnect();
	}

	logicEngine_.reset();
	scene_.reset();
	client_.reset();

	framework_.reset();
}

void BaseEngineBackend::setup(ramses::EFeatureLevel featureLevel) {
	ramses::RamsesFrameworkConfig config(frameworkConfig_);
	config.setFeatureLevel(featureLevel);
	framework_ = std::make_unique<ramses::RamsesFramework>(config);

	client_ = UniqueClient(framework_->createClient(applicationName_.c_str()), [this](ramses::RamsesClient* c) { framework_->destroyClient(*c); });
	scene_ = UniqueScene(client_->createScene(BuildOptions::internalSceneId), [this](ramses::Scene* scene) { client_->destroy(*scene); });
	logicEngine_ = UniqueLogicEngine(scene_->createLogicEngine(), [this](ramses::LogicEngine* logicEngine) { scene_->destroy(*logicEngine); });
}


bool BaseEngineBackend::connect() {
	if (!framework_->connect()) {
		LOG_ERROR(log_system::RAMSES_BACKEND, "Connection to ramses::RamsesFramework failed.");
		return true;
	} else {
		LOG_INFO(log_system::RAMSES_BACKEND, "Connection to ramses::RamsesFramework successful.");
		return false;
	}
}

ramses::RamsesFramework& BaseEngineBackend::framework() {
	return *framework_;
}

ramses::Scene& BaseEngineBackend::internalScene() {
	return *scene_.get();
}

ramses::RamsesClient& BaseEngineBackend::client() {
	return *client_.get();
}

core::EngineInterface* BaseEngineBackend::coreInterface() {
	return &coreInterface_;
}

ramses::LogicEngine* BaseEngineBackend::logicEngine() {
	return logicEngine_.get();
}

ramses::sceneId_t BaseEngineBackend::abstractSceneId() {
	return BuildOptions::abstractSceneId;
}

ramses::EFeatureLevel BaseEngineBackend::featureLevel() const {
	return featureLevel_;
}

void BaseEngineBackend::setFeatureLevel(ramses::EFeatureLevel newFeatureLevel) {
	if (!framework_ || featureLevel_ != newFeatureLevel) {
		reset();

		setup(newFeatureLevel);

		// Connect needs to be called after the renderer is created
		// Additonally there can only be one renderer per framework
		connect();

		featureLevel_ = newFeatureLevel;
	}
}

}  // namespace raco::ramses_base
