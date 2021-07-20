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

// Templated functions for de-/serialization for ease of use with EditorObject

#include "Serialization.h"
#include "data_storage/Value.h"

#include <optional>
#include <string>

namespace raco::serialization {

// If you get a compile error for the function below, make sure that EditorObject.h is included before this file.
template <typename T>
std::optional<std::string> defaultIdResolver(const raco::data_storage::ValueBase& value) {
	if (auto ref = value.asRef()) {
		return ref->objectID();
	} else {
		return {};
	}
}

template <typename T>
std::string serialize(const T& object, const std::string& projectPath = "", const ResolveReferencedId& resolveReferenceId = defaultIdResolver<T>) {
	return serializeObject(object, projectPath, resolveReferenceId);
}

template <typename T, typename L>
std::string serialize(const T& object, const std::vector<L>& links, const std::string& projectPath = "", const ResolveReferencedId& resolveReferenceId = defaultIdResolver<T>) {
	return serializeObject(object, projectPath, resolveReferenceId);
}

template <typename T, typename L>
std::string serialize(const std::vector<T>& objects, const std::vector<std::string>& rootObjectIDs, const std::vector<L>& links, const std::string& originFolder, const std::string& originFilename, const std::string& projectID, const std::string& originProjectName, const std::map<std::string, serialization::ExternalProjectInfo>& externalProjectsMap, const std::map<std::string, std::string>& originFolders, const ResolveReferencedId& resolveReferenceId = defaultIdResolver<T>) {
	std::vector<SReflectionInterface> typeConversion{objects.begin(), objects.end()};
	return serializeObjects(
		std::vector<SReflectionInterface>{objects.begin(), objects.end()},
		rootObjectIDs,
		std::vector<SReflectionInterface>{links.begin(), links.end()},
		originFolder, originFilename, projectID, originProjectName, externalProjectsMap, originFolders, resolveReferenceId);
}

}  // namespace raco::serialization