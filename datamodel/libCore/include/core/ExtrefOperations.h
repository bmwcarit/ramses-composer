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

#include <string>
#include <utility>
#include <vector>

namespace raco::core {

class BaseContext;
class CommandInterface;
class Project;

class ExternalProjectsStoreInterface {
public:
	virtual ~ExternalProjectsStoreInterface() = default;

	virtual Project* addExternalProject(const std::string& projectPath, std::vector<std::string>& pathStack) = 0;
	virtual void removeExternalProject(const std::string& projectPath) = 0;
	virtual bool canRemoveExternalProject(const std::string& projectPath) const = 0;

	virtual CommandInterface* getExternalProjectCommandInterface(const std::string& projectPath) const = 0;	
	virtual bool isExternalProject(const std::string& projectPath) const = 0;
	virtual std::vector<std::pair<std::string, CommandInterface*>> allExternalProjects() const = 0;
	virtual Project* getExternalProject(const std::string& projectPath) const = 0;
};

class ExtrefOperations {
public:
	// @exception ExtrefError
    static void updateExternalObjects(BaseContext &context, Project *project, ExternalProjectsStoreInterface &externalProjectsStore, std::vector<std::string>& pathStack);

};

}  // namespace raco::core
