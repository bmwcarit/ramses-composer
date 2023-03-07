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

	raco::core::Project* project();
	raco::core::Errors const* errors() const;
	raco::core::Errors* errors();
	raco::core::DataChangeRecorder* recorder();
	raco::core::CommandInterface* commandInterface();
	raco::core::UndoStack* undoStack();
	raco::core::MeshCache* meshCache();
	raco::components::TracePlayer& tracePlayer();

	QJsonDocument serializeProject(const std::unordered_map<std::string, std::vector<int>>& currentVersions);

	void applyDefaultCachedPaths();
	void setupCachedPathSubscriptions(const raco::components::SDataChangeDispatcher& dataChangeDispatcher);
		
Q_SIGNALS:
	void activeProjectFileChanged();
	void projectSuccessfullySaved();

private:
	void subscribeDefaultCachedPathChanges(const raco::components::SDataChangeDispatcher& dataChangeDispatcher);

	// @exception ExtrefError
	RaCoProject(const QString& file, raco::core::Project& p, raco::core::EngineInterface* engineInterface, const raco::core::UndoStack::Callback& callback, raco::core::ExternalProjectsStoreInterface* externalProjectsStore, RaCoApplication* app, core::LoadContext& loadContext);

	QJsonDocument serializeProjectData(const std::unordered_map<std::string, std::vector<int>>& currentVersions);


	void onAfterProjectPathChange(const std::string& oldPath, const std::string& newPath);
	void generateProjectSubfolder(const std::string& subFolderPath);
	void generateAllProjectSubfolders();
	void updateActiveFileListener();

	raco::core::DataChangeRecorder recorder_;
	raco::core::Errors errors_;
	raco::core::Project project_;

	raco::components::Subscription lifecycleSubscription_;
	raco::components::Subscription imageSubdirectoryUpdateSubscription_;
	raco::components::Subscription meshSubdirectoryUpdateSubscription_;
	raco::components::Subscription scriptSubdirectoryUpdateSubscription_;
	raco::components::Subscription interfaceSubdirectoryUpdateSubscription_;
	raco::components::Subscription shaderSubdirectoryUpdateSubscription_;

	std::shared_ptr<raco::core::BaseContext> context_;
	bool dirty_{false};

	components::ProjectFileChangeMonitor activeProjectFileChangeMonitor_;
	raco::components::ProjectFileChangeMonitor::UniqueListener activeProjectFileChangeListener_;

	raco::core::MeshCache* meshCache_;
	raco::core::UndoStack undoStack_;
	raco::core::CommandInterface commandInterface_;
	std::unique_ptr<raco::components::TracePlayer> tracePlayer_;
};

}  // namespace raco::application
