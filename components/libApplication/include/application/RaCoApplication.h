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

#include "application/ExternalProjectsStore.h"
#include "application/ReportStatistics.h"
#include "application/RaCoProject.h"
#include "components/DataChangeDispatcher.h"
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
class AbstractSceneAdaptor;
}  // namespace raco::ramses_adaptor

namespace raco::application {

struct RaCoApplicationLaunchSettings {
	RaCoApplicationLaunchSettings();
	RaCoApplicationLaunchSettings(QString initialProject,
		bool createDefaultScene,
		bool enableRamsesTrace,
		int newFileFeatureLevel,
		int initialLoadFeatureLevel,
		bool runningInUI);

	QString initialProject;
	bool createDefaultScene;
	bool enableRamsesTrace;
	int newFileFeatureLevel;
	int initialLoadFeatureLevel;
	bool runningInUI;
};

// Lua script saving mode. Wraps ramses::ELuaSavingMode.
enum class ELuaSavingMode {
	SourceCodeOnly = 0,
	ByteCodeOnly,
	SourceAndByteCode
};

class RaCoApplication : public QObject {
	Q_OBJECT

public:
	explicit RaCoApplication(ramses_base::BaseEngineBackend& engine, const RaCoApplicationLaunchSettings& settings = {});
	~RaCoApplication();

	RaCoProject& activeRaCoProject();
	const RaCoProject& activeRaCoProject() const;
	std::string activeProjectPath() const;
	std::string activeProjectFolder() const;

	// @exception FutureFileVersion when the loaded file contains a file version which is bigger than the known versions
	// @exception ExtrefError
	// featureLevel
	// - new scene: setup scene with given feature level; -1 = application feature level
	// - loading scene: -1 = load with feature level in project file; >0 migrate scene to specified feature level
	void switchActiveRaCoProject(const QString& file, std::function<std::string(const std::string&)> relinkCallback, bool createDefaultScene = true, int featureLevel = -1, bool generateNewObjectIDs = false);

	/**
	 * @brief Save project in new file while changing project ID and all object IDs.
	 * 
	 * This will perform a saveAs, load, save sequence. 
	 * 
	 * @param fileName New filename.
	 * @param outError Returns error message in case of failure.
	 * @param setProjectName Will set project name from filename if current project name is empty and flag is set.
	 * @return True is successful, otherwise outError will contain the error message.
	*/
	bool saveAsWithNewIDs(const QString& fileName, std::string& outError, bool setProjectName = false);

	// Get scene description and Ramses/RamsesLogic validation status and error message with the scene
	// being setup as if exported.
	// 
	// The export setup includes enabling link optimization.
	core::ErrorLevel getExportSceneDescriptionAndStatus(std::vector<core::SceneBackendInterface::SceneItemDesc>& outDescription, std::string& outMessage); 

	bool exportProject(
		const std::string& ramsesExport,
		bool compress,
		std::string& outError,
		bool forceExportWithErrors = false,
		ELuaSavingMode luaSavingMode = ELuaSavingMode::SourceCodeOnly,
		bool warningsAsErrors = false);

	void doOneLoop();

	void resetSceneBackend();

	bool canSaveActiveProject() const;

	core::ExternalProjectsStoreInterface* externalProjects();
	core::MeshCache* meshCache();

	const core::SceneBackendInterface* sceneBackend() const;

	ramses_adaptor::SceneBackend* sceneBackendImpl() const;
	ramses_adaptor::AbstractSceneAdaptor* abstractScene() const;

	components::SDataChangeDispatcher dataChangeDispatcher();

	core::EngineInterface* engine();

	QString generateApplicationTitle() const;

	bool isRunningInUI() const;

	// Take control of the time measuring inside the app. By default, RaCo will measure the elapsed wall time. By calling this function
	// RaCo instead will use the time given by the lambda. The returned value is the elapsed time in milliseconds since the application
	// was started and can never decrease.
	void overrideTime(std::function<int64_t()> getTime);

	static int minFeatureLevel();
	static int maxFeatureLevel();
	static const std::string& featureLevelDescriptions();

	void setNewFileFeatureLevel(int featureLevel);
	int newFileFeatureLevel() const;

	bool rendererDirty_ = false;

	const FeatureLevelLoadError* getFlError() const;

	void setRecordingStats(bool enable);
	void resetStats();
	const ReportStatistics& getLogicStats() const;

Q_SIGNALS:
	void performanceStatisticsUpdated();

private:
	// Needs to access externalProjectsStore_ directly:
	friend class ::ObjectTreeViewExternalProjectModelTest;

	bool exportProjectImpl(const std::string& ramsesExport, bool compress, std::string& outError, bool forceExportWithErrors, ELuaSavingMode luaSavingMode, bool warningsAsErrors) const;

	void setupScene(bool optimizedForExport, bool setupAbstractScene);

	ramses_base::BaseEngineBackend* engine_;

	// Used to set the feature level for newly created scenes.
	int newFileFeatureLevel_;

	components::SDataChangeDispatcher dataChangeDispatcher_;
	components::SDataChangeDispatcher dataChangeDispatcherPreviewScene_;
	components::SDataChangeDispatcher dataChangeDispatcherAbstractScene_;

	std::unique_ptr<ramses_adaptor::SceneBackend> previewSceneBackend_;
	std::unique_ptr<ramses_adaptor::AbstractSceneAdaptor> abstractScene_;

	components::MeshCacheImpl meshCache_;

	std::unique_ptr<RaCoProject> activeProject_;

	ExternalProjectsStore externalProjectsStore_;

	bool logicEngineNeedsUpdate_ = false;
	bool runningInUI_ = false;

	std::chrono::high_resolution_clock::time_point startTime_;
	
	std::function<int64_t()> getTime_;

	bool recordingStats_ = false;
	ReportStatistics logicStats_;
};

}  // namespace raco::application
