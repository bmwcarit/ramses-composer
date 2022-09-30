/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/ExternalProjectsStore.h"

#include "application/RaCoApplication.h"

#include "utils/u8path.h"

#include <QFile>
#include <QFileInfo>

namespace raco::application {

ExternalProjectsStore::ExternalProjectsStore(RaCoApplication* app) : application_(app) {
}

void ExternalProjectsStore::clear() {
	activeProject_ = nullptr;
	externalProjects_.clear();
	externalProjectFileChangeListeners_.clear();
	clearRelinkCallback();
	flError_.reset();
}

void ExternalProjectsStore::setActiveProject(RaCoProject* activeProject) {
	activeProject_ = activeProject;
}

void ExternalProjectsStore::buildProjectGraph(const std::string& absPath, std::vector<ProjectGraphNode>& outProjects) {
	if (std::find_if(outProjects.begin(), outProjects.end(), [absPath](const ProjectGraphNode& node) {
			return absPath == node.path;
		}) != outProjects.end()) {
		return;
	}

	auto racoProject = externalProjects_.at(absPath).get();
	if (racoProject) {
		ProjectGraphNode node{absPath};
		raco::core::Project& project = *racoProject->project();
		for (auto item : project.externalProjectsMap()) {
			auto path = project.lookupExternalProjectPath(item.first);
			buildProjectGraph(path, outProjects);
			node.externalProjectPaths.insert(path);
		}
		outProjects.push_back(node);
	}
}

void ExternalProjectsStore::updateExternalProjectsDependingOn(const std::string& absPath, int featureLevel) {
	std::vector<ProjectGraphNode> orderedProjects;
	for (const auto& item : externalProjects_) {
		buildProjectGraph(item.first, orderedProjects);
	}

	std::set<std::string> dirty;
	dirty.insert(absPath);

	auto it = orderedProjects.begin();
	while (it != orderedProjects.end()) {
		if (std::any_of(it->externalProjectPaths.begin(), it->externalProjectPaths.end(), [&dirty](auto path) {
				return dirty.find(path) != dirty.end();
			})) {
			try {
				core::LoadContext loadContext;
				loadContext.featureLevel = featureLevel;
				externalProjects_[it->path]->updateExternalReferences(loadContext);
			} catch (const raco::core::ExtrefError& e) {
				LOG_ERROR(raco::log_system::COMMON, "Exterrnal reference update failed {}", e.what());
			}
			dirty.insert(it->path);
		}
		++it;
	}

	if (std::any_of(activeProject_->project()->externalProjectsMap().begin(), activeProject_->project()->externalProjectsMap().end(), [this, &dirty](auto item) {
			auto path = activeProject_->project()->lookupExternalProjectPath(item.first);
			return dirty.find(path) != dirty.end();
		})) {
		try {
			core::LoadContext loadContext;
			loadContext.featureLevel = activeProject_->project()->featureLevel();
			activeProject_->updateExternalReferences(loadContext);
		} catch (const raco::core::ExtrefError& e) {
			LOG_ERROR(raco::log_system::COMMON, "Exterrnal reference update failed {}", e.what());
		}
	}
}

bool raco::application::ExternalProjectsStore::isCurrent(const std::string& projectPath) const {
	auto it = externalProjects_.find(projectPath);
	if (it != externalProjects_.end()) {
		auto racoProject = it->second.get();
		if (racoProject && !racoProject->project()->externalReferenceUpdateFailed()) {
			return true;
		}
	}
	return false;
}

raco::core::Project* ExternalProjectsStore::addExternalProject(const std::string& origProjectPath,  core::LoadContext& loadContext) {
	std::string projectPath = origProjectPath;
	if (relinkPathMapCache_.find(projectPath) != relinkPathMapCache_.end()) {
		projectPath = relinkPathMapCache_.at(projectPath);
	}

	auto it = externalProjects_.find(projectPath);
	if (it != externalProjects_.end()) {
		if (it->second) {
			return it->second->project();
		} else {
			return nullptr;
		}
	}

	if (std::find(loadContext.pathStack.begin(), loadContext.pathStack.end(), projectPath) != loadContext.pathStack.end()) {
		LOG_ERROR(raco::log_system::COMMON, "Can not add Project '{}' to Project Browser: project loop detected '{}' -> '{}'", projectPath, fmt::join(loadContext.pathStack, "' -> '"), projectPath);
		return nullptr;
	}

	QFileInfo fileInfo(QString::fromStdString(projectPath));
	QString absPath = fileInfo.absoluteFilePath();
	if (!raco::utils::u8path(absPath.toStdString()).existsFile() && relinkCallback_) {
		// Query for replacement path
		auto relinkPath = relinkCallback_(projectPath);
		if (!relinkPath.empty()) {
			if (auto it = externalProjects_.find(relinkPath); it != externalProjects_.end() && it->second) {
				return it->second->project();
			} else {
				relinkPathMapCache_[projectPath] = relinkPath;
				projectPath = relinkPath;
			}
		}
	}

	bool status = loadExternalProject(projectPath, loadContext);

	int featureLevel = loadContext.featureLevel;
	externalProjectFileChangeListeners_[projectPath] = externalProjectFileChangeMonitor_.registerFileChangedHandler(projectPath,
		[this, projectPath, featureLevel]() {
			core::LoadContext loadContext;
			loadContext.featureLevel = featureLevel;
			loadExternalProject(projectPath, loadContext);
			updateExternalProjectsDependingOn(projectPath, featureLevel);
		});
	application_->dataChangeDispatcher()->setExternalProjectChanged();

	if (status) {
		return externalProjects_.at(projectPath)->project();
	}
	return nullptr;
}

std::string ExternalProjectsStore::activeProjectPath() const {
	if (activeProject_ && !activeProject_->project()->currentFileName().empty()) {
		return activeProject_->project()->currentPath();
	}
	return std::string();
}

bool ExternalProjectsStore::loadExternalProject(const std::string& projectPath, core::LoadContext & loadContext) {
	std::unique_ptr<RaCoProject> project;
	bool success = false;
	if (projectPath != activeProjectPath()) {
		try {
			project = RaCoProject::loadFromFile(QString::fromStdString(projectPath), application_, loadContext, true, loadContext.featureLevel);
			success = true;
		} catch (raco::application::FutureFileVersion& fileVerError) {
			LOG_ERROR(raco::log_system::OBJECT_TREE_VIEW, "Can not add Project {} to Project Browser - incompatible file version {} of project file", projectPath, fileVerError.fileVersion_);
		} catch (raco::core::ExtrefError& error) {
			LOG_ERROR(raco::log_system::COMMON, "Can not add Project {} to Project Browser: loading failed {}", projectPath, error.what());
		} catch (const raco::application::FeatureLevelLoadError& error) {
			LOG_ERROR(raco::log_system::COMMON, "Feature level load error during external reference update");
			flError_.reset(new raco::application::FeatureLevelLoadError(error));
		} catch (std::runtime_error& error) {
			LOG_ERROR(raco::log_system::COMMON, "Loading external project '{}' failed with error: {}", projectPath, error.what());
		}
	}
	if (projectPath == activeProjectPath() ||
		(project && activeProject_ && project->project()->projectID() == activeProject_->project()->projectID())) {
		LOG_ERROR(raco::log_system::COMMON, "Can not add Project {} to Project Browser: would create project loop", projectPath);
		project = nullptr;
		success = false;
	}
	externalProjects_.insert_or_assign(projectPath, std::move(project));
	application_->dataChangeDispatcher()->setExternalProjectChanged();
	return success;
}

bool ExternalProjectsStore::canRemoveExternalProject(const std::string& projectPath) const {
	if (activeProject_) {
		return !activeProject_->project()->usesExternalProjectByPath(projectPath);
	}
	return true;
}

void ExternalProjectsStore::removeExternalProject(const std::string& projectPath) {
	if (canRemoveExternalProject(projectPath)) {
		externalProjects_.erase(externalProjects_.find(projectPath));
		externalProjectFileChangeListeners_.erase(projectPath);

		application_->dataChangeDispatcher()->setExternalProjectChanged();
	}
}

raco::core::CommandInterface* ExternalProjectsStore::getExternalProjectCommandInterface(const std::string& projectPath) const {
	auto it = externalProjects_.find(projectPath);
	if (it == externalProjects_.end() || it->second == nullptr) {
		return nullptr;
	}
	return it->second->commandInterface();
}

bool ExternalProjectsStore::isExternalProject(const std::string& projectPath) const {
	return externalProjects_.find(projectPath) != externalProjects_.end();
}

std::vector<std::pair<std::string, raco::core::CommandInterface*>> ExternalProjectsStore::allExternalProjects() const {
	std::vector<std::pair<std::string, raco::core::CommandInterface*>> projects;
	projects.reserve(externalProjects_.size());
	for (auto const& p : externalProjects_) {
		if (p.second) {
			projects.push_back({p.first, p.second->commandInterface()});
		} else {
			projects.push_back({p.first, nullptr});
		}
	}
	return projects;
}

raco::core::Project* ExternalProjectsStore::getExternalProject(const std::string& projectPath) const {
	auto it = externalProjects_.find(projectPath);
	if (it != externalProjects_.end()) {
		return it->second->project();
	}
	return nullptr;
}

void ExternalProjectsStore::setRelinkCallback(std::function<std::string(const std::string&)> relinkCallback) {
	relinkCallback_ = relinkCallback;
	relinkPathMapCache_.clear();
}

void ExternalProjectsStore::clearRelinkCallback() {
	relinkCallback_ = std::function<std::string(const std::string&)>();
	relinkPathMapCache_.clear();
}

const FeatureLevelLoadError* ExternalProjectsStore::getFlError() const {
	return flError_.get();
}

}  // namespace raco::application