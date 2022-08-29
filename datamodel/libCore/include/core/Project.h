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

#include "EditorObject.h"
#include "Handles.h"
#include "Link.h"
#include "core/ProjectSettings.h"
#include "core/Serialization.h"
#include "log_system/log.h"
#include <map>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace raco::core {

struct ExtrefError : public std::runtime_error {
	explicit ExtrefError(const std::string& what) : std::runtime_error(what) {}
};

class UndoStack;

class Project {
public:

	explicit Project();
	explicit Project(const std::vector<SEditorObject>& instances);

	// Remove a set of objects from the instance pool.
	bool removeInstances(SEditorObjectSet const& objects, bool gcExternalProjectMap = true);

	void addInstance(SEditorObject object);

	const std::vector<SEditorObject>& instances() const;

	std::string projectName() const;
	std::string projectID() const;

	std::string getProjectNameForObject(SEditorObject const& object, bool fallbackToLocalProject = true) const;

	// Directory of current project
	// - will always be non-empty 
	// - contains default project directory for new project that hasn't been saved yet.
	std::string currentFolder() const;

	// Complete file path; empty for new project that hasn't been saved yet.
	std::string currentPath() const;
	
	// File name of project; empty for new project that hasn't been saved yet.
	std::string currentFileName() const;

	std::shared_ptr<const ProjectSettings> settings() const;
	std::shared_ptr<ProjectSettings> settings();
	void setCurrentPath(const std::string& newPath);

	int featureLevel() const;
	void setFeatureLevel(int featureLevel);

	void addLink(SLink link);
	void removeLink(SLink link);

	// Find link in the current Project corresponding to the given link.
	// The argument link may be from a different Project.
	// Objects are compared by object id and not by pointer comparison.
	SLink findLinkByObjectID(SLink link) const;


	const std::map<std::string, std::set<SLink>>& linkStartPoints() const;
	const std::map<std::string, std::set<SLink>>& linkEndPoints() const;
	const std::vector<SLink>& links() const;

	bool checkLinkDuplicates() const;

	SEditorObject getInstanceByID(const std::string& objectID) const;

	// Check if an object is part of this project.
	bool isInstance(const SEditorObject& object) const;

	bool createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const;

	// @exception ExtrefError if collisions are detected.
	void addExternalProjectMapping(const std::string& projectID, const std::string& path, const std::string& projectName);
	void updateExternalProjectName(const std::string& projectID, const std::string& projectName);
	void updateExternalProjectPath(const std::string& projectID, const std::string& projectPath);
	void removeExternalProjectMapping(const std::string& projectID);
	bool hasExternalProjectMapping(const std::string& projectID) const;
	std::string lookupExternalProjectPath(const std::string& projectID) const;
	std::string lookupExternalProjectName(const std::string& projectID) const;
	void rerootExternalProjectPaths(const std::string oldFolder, const std::string newFolder);
	bool usesExternalProjectByPath(const std::string& absPath) const;

	const std::map<std::string, serialization::ExternalProjectInfo>& externalProjectsMap() const;

	// "garbage collect" external project mapping by removing external projects that are 
	// not used in the current project.
	bool gcExternalProjectMapping();

	bool externalReferenceUpdateFailed() const;
	void setExternalReferenceUpdateFailed(bool status);

	template <typename It>
	static std::string findAvailableUniqueName(It begin, It end, SEditorObject newObject, const std::string& name) {
		if (!(std::find_if(begin, end, [newObject, name](auto obj) {
				return obj != newObject && obj->objectName() == name;
			}) != end)) {
			return name;
		}

		std::string basename;
		std::smatch match;
		if (std::regex_match(name, match, NAMING_PATTERN)) {
			basename = match[1];
		} else {
			basename = name;
		}

		std::set<int> indices;
		for (auto it = begin; it != end; ++it) {
			auto currentName = (*it)->objectName();
			std::smatch m;
			if (std::regex_match(currentName, m, NAMING_PATTERN)) {
				if (basename == m[1]) {
					indices.insert(std::stoi(m[2]));
				}
			} else {
				if (currentName == basename) {
					indices.insert(0);
				}
			}
		}

		auto it = std::adjacent_find(indices.begin(), indices.end(), [](int l, int r) { return l + 1 < r; });
		if (it == indices.end()) {
			--it;
		}
		return fmt::format("{} ({})", basename, *it + 1);
	}

	// Repair project by removing duplicate links
	void deduplicateLinks();

	// Lock/unlock code-controlled objects in the Project
	void lockCodeCtrldObjs(const SEditorObjectSet& editorObjs);
	SEditorObjectSet unlockCodeCtrldObjs(const SEditorObjectSet& editorObjs);
	// Check if the object is code-controlled
	bool isCodeCtrldObj(const SEditorObject& editorObj) const;

private:
	// Needed because undo/redo needs to set the complete externalProjectsMap_ at once but
	// we don't want public functions to allow anybody to do that.
	friend class UndoStack;

	static SLink findLinkByObjectID(const std::map<std::string, std::set<SLink>>& linksByEndPointID, SLink link);

	void removeAllLinks();


	/**
	 * @brief Maintains the connection graph of all non-weak links in the project and implements loop detection.
	 * 
	 * Weak links are not relevant for loop detection and are therefore not included in the link graph.
	*/
	class LinkGraph {
	public:
		LinkGraph(const Project& project);

		void addLink(SLink link);
		void removeLink(const Project& project, SLink link);

		void removeAllLinks();

		bool createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const;

	private:
		bool depthFirstSearch(SEditorObject current, SEditorObject obj, SEditorObjectSet& visited) const;
		std::map<SEditorObject, SEditorObjectSet> graph;
	};

	std::string folder_;
	std::string filename_;

	// Ordered list of all instances.
	std::vector<SEditorObject> instances_;
	// Instance dictionary using object id as key for faster lookup.
	std::unordered_map<std::string, SEditorObject> instanceMap_;

	// This map contains all the external project used by the current one;
	// Keys are the project IDs
	// Values contain the relative file paths and names of the external project files.
	// The project names are only cached here.
	// Note: the ExternalReferenceAnnotation attached to objects only contains the 
	// project id and needs to map to resolve relative paths.
	std::map<std::string, serialization::ExternalProjectInfo> externalProjectsMap_;
	bool externalReferenceUpdateFailed_ = false;

	// This map contains all links used by the project,
	// using the link start object ID as the key value, for easier lookup.
	// Mostly used for link-related functions in raco::core::Queries.
	std::map<std::string, std::set<SLink>> linkStartPoints_;

	// This map contains all links used by the project,
	// using the link end object ID as the key value, for easier lookup.
	std::map<std::string, std::set<SLink>> linkEndPoints_;

	std::vector<SLink> links_;
	LinkGraph linkGraph_;

	static inline const std::regex NAMING_PATTERN{"(.*)\\s+\\((\\d+)\\)"};

	// List of all code-controlled objects.
	std::unordered_set<SEditorObject> codeCtrldObjs_{};
};

}  // namespace raco::core