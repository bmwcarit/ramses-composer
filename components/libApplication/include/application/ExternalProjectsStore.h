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

#include "core/ExtrefOperations.h"

#include "application/ExternalProjectsStore.h"
#include "application/RaCoProject.h"
#include "components/DataChangeDispatcher.h"
#include "core/ChangeRecorder.h"
#include "core/Project.h"
#include <map>
#include <memory>
#include <set>
#include <string>

class ObjectTreeViewExternalProjectModelTest;

namespace raco::application {

class RaCoApplication;

class ExternalProjectsStore : public raco::core::ExternalProjectsStoreInterface {
public:
	ExternalProjectsStore(RaCoApplication* app);

	void clear();

	void setActiveProject(RaCoProject* activeProject);

	bool isCurrent(const std::string& projectPath) const;

	// @return true if loaded successfully
	raco::core::Project* addExternalProject(const std::string& projectPath, std::vector<std::string>& pathStack) override;
	void removeExternalProject(const std::string& projectPath) override;
	bool canRemoveExternalProject(const std::string& projectPath) const override;

	raco::core::CommandInterface* getExternalProjectCommandInterface(const std::string& projectPath) const override;
	bool isExternalProject(const std::string& projectPath) const override;
	std::vector<std::pair<std::string, raco::core::CommandInterface*>> allExternalProjects() const override;
	raco::core::Project* getExternalProject(const std::string& projectPath) const override;

private:
	// Needs to access externalProjects_ directly:
	friend class ::ObjectTreeViewExternalProjectModelTest;

	struct ProjectGraphNode {
		std::string path;
		std::set<std::string> externalProjectPaths;
	};

	std::string activeProjectPath() const;

	void buildProjectGraph(const std::string& absPath, std::vector<ProjectGraphNode>& outProjects);
	void updateExternalProjectsDependingOn(const std::string& absPath);
	bool loadExternalProject(const std::string& projectPath, std::vector<std::string>& pathStack);

	RaCoProject* activeProject_ = nullptr;
	RaCoApplication* application_ = nullptr;

	std::map<std::string, std::unique_ptr<RaCoProject>> externalProjects_;

	components::ProjectFileChangeMonitor externalProjectFileChangeMonitor_;

	std::unordered_map<std::string, raco::components::ProjectFileChangeMonitor::UniqueListener> externalProjectFileChangeListeners_;
};

}  // namespace raco::application
