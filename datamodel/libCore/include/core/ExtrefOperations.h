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

#include <string>
#include <utility>
#include <vector>
#include <functional>

namespace raco::core {

class BaseContext;
class CommandInterface;
class Project;

struct LoadContext {
	std::vector<std::string> pathStack;
	int featureLevel{-1};
};

class ExternalProjectsStoreInterface {
public:
	virtual ~ExternalProjectsStoreInterface() = default;

	virtual Project* addExternalProject(const std::string& projectPath, LoadContext& loadContext) = 0;
	virtual void removeExternalProject(const std::string& projectPath) = 0;
	virtual bool canRemoveExternalProject(const std::string& projectPath) const = 0;

	virtual CommandInterface* getExternalProjectCommandInterface(const std::string& projectPath) const = 0;	
	virtual bool isExternalProject(const std::string& projectPath) const = 0;
	virtual std::vector<std::pair<std::string, CommandInterface*>> allExternalProjects() const = 0;
	virtual Project* getExternalProject(const std::string& projectPath) const = 0;

	// Setup callback to allow relinking of external projects.
	// The callback is invoked if a call to addExternalProject can't find the project file.
	// The return value of the callback is the alternative project path to use instead or an empty string
	// if no relinking is to be done.
	virtual void setRelinkCallback(std::function<std::string(const std::string&)> relinkCallback) = 0;

	// Clear the callback and related interal caches.
	virtual void clearRelinkCallback() = 0;
};

class ExtrefOperations {
public:
	// @exception ExtrefError
	static void updateExternalObjects(BaseContext& context, Project* project, ExternalProjectsStoreInterface& externalProjectsStore, LoadContext& loadContext);

	// Check if an external reference update would fail if the rootObjects would all be in the current project.
	// 
	// This will collect all external objects as is done in the actual external reference update but starting
	// with the supplied set of objects instead of the external reference objects in the current project.
	//
	// In case of failure this will throw the same exception as an actual external reference update would.
	static void precheckExternalReferenceUpdate(Project* project, ExternalProjectsStoreInterface& externalProjectsStore, LoadContext& loadContext, std::vector<SEditorObject> rootObjects);
};

}  // namespace raco::core
