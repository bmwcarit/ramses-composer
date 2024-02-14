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

#include "components/DataChangeDispatcher.h"
#include "components/MeshCacheImpl.h"
#include "components/Naming.h"
#include "core/CommandInterface.h"
#include "core/Context.h"
#include "core/Errors.h"
#include "core/ExtrefOperations.h"
#include "core/Project.h"
#include "core/Serialization.h"
#include "core/Undo.h"
#include "user_types/UserObjectFactory.h"
#include <QObject>
#include <exception>
#include <functional>
#include <memory>
#include <QFileInfo>

namespace raco::components {
class TracePlayer;
}
namespace raco::application {

class RaCoApplication;

struct FutureFileVersion : public std::exception {
	explicit FutureFileVersion(int fileVersion) : fileVersion_{fileVersion} {}
	int fileVersion_;
	const char* what() const throw() {
		return "FutureFileVersion";
	}
};

struct FeatureLevelLoadError : public std::runtime_error {
	explicit FeatureLevelLoadError(const std::string& what, int currentFeatureLevel, int projectFeatureLevel, const std::string& projectPath)
		: runtime_error(what),
		  currentFeatureLevel_(currentFeatureLevel),
		  projectFeatureLevel_(projectFeatureLevel),
		  projectPath_(projectPath) {}

	int currentFeatureLevel_;
	int projectFeatureLevel_;
	std::string projectPath_;
};

class RaCoProject : public QObject {
	Q_OBJECT
public:
	Q_DISABLE_COPY(RaCoProject);
	~RaCoProject();

	/**
	 * @brief Create and initialize new scene
	 * @param app Application object containing the global caches, dispatchers, and the external project store.
	 * @param createDefaultScene Create default scene objects
	 * @param featureLevel create scene with specified feature level; use max feature level if -1 is passed in.
	 * @return 
	*/
	static std::unique_ptr<RaCoProject> createNew(RaCoApplication* app, bool createDefaultScene, int featureLevel);

	static int preloadFeatureLevel(const QString& filename, int featureLevel);

	/**
	 * @brief Loads and checks project file
	 * @param filename path to the project file
	 * @return file JSON content
	 */
	static QJsonDocument loadJsonDocument(const QString& filename);

	/**
	 * @brief Load scene
	 * @param featureLevel update scene to given feature level if >0 and use feature level from file if -1
	 * @exception FutureFileVersion when the loaded file contains a file version which is bigger than the known versions
	 * @exception ExtrefError
	 */
	static std::unique_ptr<RaCoProject> loadFromFile(const QString& filename, RaCoApplication* app, core::LoadContext& loadContext, bool logErrors = true, int featureLevel = -1, bool generateNewObjectIDs = false);
	
	QString name() const;

	bool dirty() const noexcept;
	bool save(std::string &outError);
	bool saveAs(const QString& fileName, std::string& outError, bool setProjectName = false);

	// @exception ExtrefError
	void updateExternalReferences(core::LoadContext& loadContext);

	core::Project* project();
	core::Errors const* errors() const;
	core::Errors* errors();
	core::DataChangeRecorder* recorder();
	core::CommandInterface* commandInterface();
	core::UndoStack* undoStack();
	core::MeshCache* meshCache();
	components::TracePlayer& tracePlayer();

	QJsonDocument serializeProject(const std::unordered_map<std::string, std::vector<int>>& currentVersions);

	void applyPreferences() const;
	void applyDefaultCachedPaths();
	void setupCachedPathSubscriptions(const components::SDataChangeDispatcher& dataChangeDispatcher);
		
Q_SIGNALS:
	void activeProjectFileChanged();
	void projectSuccessfullySaved();

private:
	void subscribeDefaultCachedPathChanges(const components::SDataChangeDispatcher& dataChangeDispatcher);

	// @exception ExtrefError
	RaCoProject(const QString& file, core::Project& p, core::EngineInterface* engineInterface, const core::UndoStack::Callback& callback, core::ExternalProjectsStoreInterface* externalProjectsStore, RaCoApplication* app, core::LoadContext& loadContext, int fileVersion);

	QJsonDocument serializeProjectData(const std::unordered_map<std::string, std::vector<int>>& currentVersions);


	void onAfterProjectPathChange(const std::string& oldPath, const std::string& newPath);
	void generateProjectSubfolder(const std::string& subFolderPath);
	void generateAllProjectSubfolders();
	void updateActiveFileListener();

	core::DataChangeRecorder recorder_;
	core::Errors errors_;
	core::Project project_;

	components::Subscription lifecycleSubscription_;
	components::Subscription imageSubdirectoryUpdateSubscription_;
	components::Subscription meshSubdirectoryUpdateSubscription_;
	components::Subscription scriptSubdirectoryUpdateSubscription_;
	components::Subscription interfaceSubdirectoryUpdateSubscription_;
	components::Subscription shaderSubdirectoryUpdateSubscription_;

	std::shared_ptr<core::BaseContext> context_;
	bool dirty_{false};

	components::ProjectFileChangeMonitor activeProjectFileChangeMonitor_;
	components::ProjectFileChangeMonitor::UniqueListener activeProjectFileChangeListener_;

	core::MeshCache* meshCache_;
	core::UndoStack undoStack_;
	core::CommandInterface commandInterface_;
	std::unique_ptr<components::TracePlayer> tracePlayer_;

	std::filesystem::file_time_type lastModifiedTime_;
};

}  // namespace raco::application
