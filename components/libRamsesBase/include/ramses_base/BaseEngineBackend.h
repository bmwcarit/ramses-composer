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

#include <ramses-client-api/RamsesClient.h>
#include <ramses-client-api/Scene.h>
#include <ramses-framework-api/RamsesFramework.h>
#include <ramses-framework-api/RamsesFrameworkConfig.h>
#include "ramses_base/CoreInterfaceImpl.h"
#include "ramses_base/LogicEngine.h"
#include <QtCore>
#include <memory>

namespace raco::ramses_base {

/**
 * "Abstract" base class for headless and gui ramses instantiation.
 */
class BaseEngineBackend {
	Q_DISABLE_COPY(BaseEngineBackend)
public:
	static const rlogic::EFeatureLevel minFeatureLevel = rlogic::EFeatureLevel::EFeatureLevel_01;
	static const rlogic::EFeatureLevel maxFeatureLevel = rlogic::EFeatureLevel::EFeatureLevel_05;
	static const std::string featureLevelDescriptions;

	typedef std::unique_ptr<ramses::RamsesClient, std::function<void(ramses::RamsesClient*)>> UniqueClient;
	typedef std::unique_ptr<ramses::Scene, std::function<void(ramses::Scene*)>> UniqueScene;
	typedef std::unique_ptr<rlogic::LogicEngine> UniqueLogicEngine;
	
	BaseEngineBackend(
		rlogic::EFeatureLevel featureLevel, 
		const ramses::RamsesFrameworkConfig& frameworkConfig = ramses::RamsesFrameworkConfig{},
		const char* applicationName = "Ramses Composer");
	virtual ~BaseEngineBackend();
	bool connect();
	ramses::RamsesFramework& framework();
	ramses::RamsesClient& client();
	LogicEngine& logicEngine();
	raco::core::EngineInterface* coreInterface();

	void setFeatureLevel(rlogic::EFeatureLevel newFeatureLevel);
	rlogic::EFeatureLevel getFeatureLevel();

	/**
	 * Scene used for internal validation / creation of resource.
	 * E.g. parsing a shader text.
	 */
	ramses::Scene& internalScene();

private:
	ramses::RamsesFramework framework_;
	UniqueLogicEngine logicEngine_;
	UniqueClient client_;
	UniqueScene scene_;
	CoreInterfaceImpl coreInterface_;
};

}  // namespace raco::ramses_base
