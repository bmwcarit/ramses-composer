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

#include "application/ExternalProjectsStore.h"
#include "application/RaCoProject.h"
#include "components/DataChangeDispatcher.h"
#include "core/ChangeRecorder.h"
#include "core/Project.h"
#include "core/SceneBackendInterface.h"
#include <memory>

#include "core/ExtrefOperations.h"

class ObjectTreeViewExternalProjectModelTest;

namespace raco::core {
enum class ErrorLevel;
}

namespace raco::ramses_base {
class BaseEngineBackend;
}

namespace raco::ramses_adaptor {
class SceneBackend;
}

namespace raco::application {

struct RaCoApplicationLaunchSettings {
	QString initialProject{};
	bool createDefaultScene{true};
	bool enableRamsesTrace{false};
};

class RaCoApplication {
public:
	explicit RaCoApplication(ramses_base::BaseEngineBackend& engine, const RaCoApplicationLaunchSettings& settings = {});

	RaCoProject& activeRaCoProject();
	const RaCoProject& activeRaCoProject() const;
	std::string activeProjectPath() const;
	std::string activeProjectFolder() const;

	// @exception FutureFileVersion when the loaded file contains a file version which is bigger than the known versions
	// @exception ExtrefError
	void switchActiveRaCoProject(const QString& file, bool createDefaultScene = true);

	// Get scene description and Ramses/RamsesLogic validation status and error message with the scene
	// being setup as if exported.
	// 
	// The export setup includes enabling link optimization.
	core::ErrorLevel getExportSceneDescriptionAndStatus(std::vector<core::SceneBackendInterface::SceneItemDesc>& outDescription, std::string& outMessage); 

	bool exportProject(
		const std::string& ramsesExport,
		const std::string& logicExport,
		bool compress,
		std::string& outError, 
		bool forceExportWithErrors = false);


	void doOneLoop();

	void resetSceneBackend();

	bool canSaveActiveProject() const;

	raco::core::ExternalProjectsStoreInterface* externalProjects();
	raco::core::MeshCache* meshCache();

	const core::SceneBackendInterface* sceneBackend() const;

	raco::ramses_adaptor::SceneBackend* sceneBackendImpl() const;

	raco::components::SDataChangeDispatcher dataChangeDispatcher();

	raco::core::EngineInterface* engine();

	QString generateApplicationTitle() const;

	bool rendererDirty_ = false;

private:
	// Needs to access externalProjectsStore_ directly:
	friend class ::ObjectTreeViewExternalProjectModelTest;

	bool exportProjectImpl(const std::string& ramsesExport,	const std::string& logicExport,	bool compress, std::string& outError, bool forceExportWithErrors) const;

	ramses_base::BaseEngineBackend* engine_;

	raco::components::SDataChangeDispatcher dataChangeDispatcher_;
	raco::components::SDataChangeDispatcher dataChangeDispatcherEngine_;

	std::unique_ptr<raco::ramses_adaptor::SceneBackend> scenesBackend_;

	components::MeshCacheImpl meshCache_;

	std::unique_ptr<RaCoProject> activeProject_;

	ExternalProjectsStore externalProjectsStore_;

	bool logicEngineNeedsUpdate_ = false;

	std::chrono::high_resolution_clock::time_point startTime_;
};

}  // namespace raco::application
