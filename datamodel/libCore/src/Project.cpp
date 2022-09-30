/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Project.h"
#include "core/PathManager.h"
#include "core/ExternalReferenceAnnotation.h"
#include "utils/u8path.h"
#include "core/CoreFormatter.h"
#include "log_system/log.h"

#include <cctype>
#include <filesystem>

namespace raco::core {

Project::Project() : instances_{} {
	setCurrentPath(utils::u8path::current().string());
}

Project::Project(const std::vector<SEditorObject>& instances) : instances_{instances} {
	for (auto obj : instances_) {
		instanceMap_[obj->objectID()] = obj;
	}
	setCurrentPath(utils::u8path::current().string());
}

bool Project::removeInstances(SEditorObjectSet const& objects, bool gcExternalProjectMap) {
	for (const auto& object : objects) {
		instances_.erase(std::find(instances_.begin(), instances_.end(), object));
		codeCtrldObjs_.erase(object);
		instanceMap_.erase(object->objectID());
	}
	if (gcExternalProjectMap) {
		return gcExternalProjectMapping();
	}
	return false;
}

void Project::addInstance(SEditorObject object) {
	if (instanceMap_.find(object->objectID()) != instanceMap_.end()) {
		throw std::runtime_error(fmt::format("duplicate object {} with id {}", object->objectName(), object->objectID()));
	}
	instances_.push_back(object);
	instanceMap_[object->objectID()] = object;
}

const std::vector<SEditorObject>& Project::instances() const {
	return instances_;
}

std::string Project::projectName() const {
	if (auto settingsObj = settings()) {
		return settingsObj->objectName();
	}
	return std::string();
}

std::string Project::projectID() const {
	if (auto settingsObj = settings()) {
		return settingsObj->objectID();
	}
	return std::string();
}

int Project::featureLevel() const {
	if (auto settingsObj = settings()) {
		return *settingsObj->featureLevel_;
	}
	return 1;
}

void Project::setFeatureLevel(int featureLevel) {
	if (auto settingsObj = settings()) {
		settingsObj->featureLevel_ = featureLevel;
	}
}

std::string Project::getProjectNameForObject(SEditorObject const& object, bool fallbackToLocalProject) const {
	if (auto extrefAnno = object->query<ExternalReferenceAnnotation>()) {
		return externalProjectsMap_.at(*extrefAnno->projectID_).name;
	}
	if (fallbackToLocalProject) {
		return projectName();
	}
	return std::string();
}

std::string Project::currentFolder() const {
	return folder_;
}

std::string Project::currentPath() const {
	return (raco::utils::u8path(currentFolder()) / currentFileName()).string();
}

std::string Project::currentFileName() const {
	return filename_;
}

void Project::setCurrentPath(const std::string& newPath) {
	auto path = utils::u8path(newPath);
	assert(path.is_absolute());
	if (path.existsDirectory()) {
		folder_ = newPath;
		filename_.clear();
	} else {
		path = path.normalized();
		folder_ = path.parent_path().string();
		filename_ = path.filename().string();
	}
}

void Project::addLink(SLink link) {
	links_.addLink(link);
	linkGraph_.addLink(link);
}

void Project::removeLink(SLink link) {
	links_.removeLink(link);
	linkGraph_.removeLink(link);
}

void Project::removeAllLinks() {
	linkGraph_.removeAllLinks();
	links_.clear();
}

SLink Project::findLinkByObjectID(SLink link) const {
	return links_.findLinkByObjectID(link);
}

const std::map<std::string, std::set<SLink>>& Project::linkStartPoints() const {
	return links_.linkStartPoints_;
}

const std::map<std::string, std::set<SLink>>& Project::linkEndPoints() const {
	return links_.linkEndPoints_;
}

const LinkContainer& Project::links() const {
	return links_;
}

bool Project::checkLinkDuplicates() const {
	for (const auto& linksEnd : links_.linkEndPoints_) {
		std::vector<SLink> cmpLinks;
		for (const auto& link : linksEnd.second) {
			for (auto cmpLink : cmpLinks) {
				if (compareLinksByObjectID(*cmpLink, *link)) {
					return true;
				}
			}
			cmpLinks.emplace_back(link);
		}
	}
	return false;
}

void Project::deduplicateLinks() {
	std::map<std::string, std::set<SLink>> newLinkEndPoints;
	for (const auto& linksEnd : links_.linkEndPoints_) {
		for (const auto& link : linksEnd.second) {
			if (!LinkContainer::findLinkByObjectID(newLinkEndPoints, link)) {
				// no duplicate -> insert 
				newLinkEndPoints[(*link->endObject_)->objectID()].insert(link);
			} else {
				// duplicate -> log warning
				LOG_WARNING(log_system::PROJECT, "Duplicate link discarded: {}", link);
			}
		}
	}

	removeAllLinks();

	for (const auto& linksEnd : newLinkEndPoints) {
		for (const auto& link : linksEnd.second) {
			addLink(link);
		}
	}
}

std::shared_ptr<const ProjectSettings> Project::settings() const {
	for (auto& instance : instances_) {
		if (instance->isType<core::ProjectSettings>()) {
			return std::dynamic_pointer_cast<core::ProjectSettings>(instance);
		}
	}
	return {};
}

std::shared_ptr<ProjectSettings> Project::settings() {
	for (auto &instance : instances_) {
		if (instance->isType<core::ProjectSettings>()) {
			return std::dynamic_pointer_cast<core::ProjectSettings>(instance);
		}
	}
	return {};
}

SEditorObject Project::getInstanceByID(const std::string& objectID) const {
	auto it = instanceMap_.find(objectID);
	if (it != instanceMap_.end()) {
		return it->second;
	}
	return SEditorObject{};
}

bool Project::isInstance(const SEditorObject& object) const {
	auto it = instanceMap_.find(object->objectID());
	return it != instanceMap_.end() && it->second == object;
}

bool Project::createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const {
	return linkGraph_.createsLoop(start, end);
}

void Project::addExternalProjectMapping(const std::string& projectID, const std::string& absPath, const std::string& projectName) {
	if (projectID.empty()) {
		throw ExtrefError("External project with empty ID not allowed.");
	}
	if (projectID == this->projectID()) {
		throw ExtrefError("External reference project loop detected (based on same project ID).");
	}

	if (absPath == currentPath()) {
		throw ExtrefError("External reference project loop detected (based on same project path).");
	}

	auto relPath = raco::utils::u8path(absPath).normalizedRelativePath(currentFolder()).string();

	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		if (relPath != it->second.path) {
			throw ExtrefError(fmt::format("Duplicate external project ID detected (with a different file path): {} at {}", it->second.name, it->second.path));
		}
	}

	{
		auto it = std::find_if(externalProjectsMap_.begin(), externalProjectsMap_.end(), [&relPath](const auto& item) {
			return item.second.path == relPath;
		});
		if (it != externalProjectsMap_.end()) {
			if (it->first != projectID) {
				throw ExtrefError(fmt::format("Project ID change for file '{}' detected: '{}' changed to '{}'", relPath, it->first, projectID));
			}
		}
	}

	externalProjectsMap_[projectID] = serialization::ExternalProjectInfo{relPath, projectName};
}

void Project::updateExternalProjectName(const std::string& projectID, const std::string& projectName) {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		it->second.name = projectName;
	}
}

void Project::updateExternalProjectPath(const std::string& projectID, const std::string& projectPath) {
	auto projectName = lookupExternalProjectName(projectID);
	removeExternalProjectMapping(projectID);
	addExternalProjectMapping(projectID, projectPath, projectName);
}

void Project::removeExternalProjectMapping(const std::string& projectID) {
	externalProjectsMap_.erase(projectID);
}

bool Project::hasExternalProjectMapping(const std::string& projectID) const {
	return externalProjectsMap_.find(projectID) != externalProjectsMap_.end();
}

std::string Project::lookupExternalProjectPath(const std::string& projectID) const {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		return raco::utils::u8path(it->second.path).normalizedAbsolutePath(currentFolder()).string();
	}
	return std::string();
}

std::string Project::lookupExternalProjectName(const std::string& projectID) const {
	auto it = externalProjectsMap_.find(projectID);
	if (it != externalProjectsMap_.end()) {
		return it->second.name;
	}
	return std::string();
}

void Project::rerootExternalProjectPaths(const std::string oldFolder, const std::string newFolder) {
	for (auto& item : externalProjectsMap_) {
		item.second.path = raco::utils::u8path(item.second.path).rerootRelativePath(oldFolder, newFolder).string();
	}
}

bool Project::usesExternalProjectByPath(const std::string& absPath) const {
	for (auto item : externalProjectsMap_) {
		if (raco::utils::u8path(item.second.path).normalizedAbsolutePath(currentFolder()) == absPath) {
			return true;
		}
	}
	return false;
}

const std::map<std::string, serialization::ExternalProjectInfo>& Project::externalProjectsMap() const {
	return externalProjectsMap_;  
}

bool Project::gcExternalProjectMapping() {
	std::set<std::string> usedExtProjects;
	for (auto obj : instances_) {
		if (auto extrefAnno = obj->query<ExternalReferenceAnnotation>()) {
			usedExtProjects.insert(*extrefAnno->projectID_);
		}
	}

	bool changed = false;
	auto it = externalProjectsMap_.begin();
	while (it != externalProjectsMap_.end()) {
		auto projectID = it->first;
		if (usedExtProjects.find(projectID) == usedExtProjects.end()) {
			it = externalProjectsMap_.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	return changed;
}

bool Project::externalReferenceUpdateFailed() const {
	return externalReferenceUpdateFailed_;
}

void Project::setExternalReferenceUpdateFailed(bool status) {
	externalReferenceUpdateFailed_ = status;
}

void Project::lockCodeCtrldObjs(const SEditorObjectSet& editorObjs) {
	for (const auto& editorObj : editorObjs) {
		if (isCodeCtrldObj(editorObj)) {
			assert(false && "Object is already locked!");
			return;
		}
		codeCtrldObjs_.insert(editorObj);
	}
}

SEditorObjectSet Project::unlockCodeCtrldObjs(const SEditorObjectSet& editorObjs) {
	SEditorObjectSet unlockedObjs;

	for (const auto& editorObj : editorObjs) {
		if (isCodeCtrldObj(editorObj)) {
			unlockedObjs.insert(editorObj);
			codeCtrldObjs_.erase(editorObj);
		} else {
			/* do nothing; object was deleted or unlocked */
		}
	}

	return unlockedObjs;
}

bool Project::isCodeCtrldObj(const SEditorObject& editorObj) const {
	return (codeCtrldObjs_.find(editorObj) != codeCtrldObjs_.end());
}

}  // namespace raco::core