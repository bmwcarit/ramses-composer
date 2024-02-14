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

#include <ramses/client/RamsesClient.h>
#include <ramses/client/Scene.h>
#include <ramses/client/logic/LogicEngine.h>
#include <ramses/framework/RamsesFramework.h>
#include <ramses/framework/RamsesFrameworkConfig.h>
#include "ramses_base/CoreInterfaceImpl.h"
#include <QtCore>
#include <memory>

namespace raco::ramses_base {

/**
 * "Abstract" base class for headless and gui ramses instantiation.
 */
class BaseEngineBackend {
	Q_DISABLE_COPY(BaseEngineBackend)
public:
	static const ramses::EFeatureLevel minFeatureLevel;
	static const ramses::EFeatureLevel maxFeatureLevel;
	static const std::string featureLevelDescriptions;

	typedef std::unique_ptr<ramses::RamsesClient, std::function<void(ramses::RamsesClient*)>> UniqueClient;
	typedef std::unique_ptr<ramses::Scene, std::function<void(ramses::Scene*)>> UniqueScene;
	typedef std::unique_ptr<ramses::RamsesFramework> UniqueFramework;
	typedef std::unique_ptr<ramses::LogicEngine, std::function<void(ramses::LogicEngine*)>> UniqueLogicEngine;
	
	// The feature level used in the RamsesFrameworkConfig here is just a placeholder.
	BaseEngineBackend(
		const ramses::RamsesFrameworkConfig& frameworkConfig = ramses::RamsesFrameworkConfig{minFeatureLevel},
		const char* applicationName = "Ramses Composer");
	virtual ~BaseEngineBackend();
	bool connect();
	ramses::RamsesFramework& framework();
	ramses::RamsesClient& client();
	core::EngineInterface* coreInterface();
	ramses::LogicEngine* logicEngine();


	ramses::EFeatureLevel featureLevel() const;

	void setFeatureLevel(ramses::EFeatureLevel newFeatureLevel);

	/**
	 * Scene used for internal validation / creation of resource.
	 * E.g. parsing a shader text.
	 */
	ramses::Scene& internalScene();

	static ramses::sceneId_t abstractSceneId();

	static ramses::RamsesFrameworkConfig& defaultRamsesFrameworkConfig();

protected:
	virtual void reset();
	virtual void setup(ramses::EFeatureLevel featureLevel);

private:
	const ramses::RamsesFrameworkConfig& frameworkConfig_;
	std::string applicationName_;

	ramses::EFeatureLevel featureLevel_ = minFeatureLevel;

	UniqueFramework framework_;
	UniqueClient client_;
	UniqueScene scene_;

	// Global LogicEngine instance only used for LuaScript/Interface/Module parsing.
	// This will have the same feature level as the user project.
	UniqueLogicEngine logicEngine_;

	CoreInterfaceImpl coreInterface_;
};

}  // namespace raco::ramses_base
