/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/PathQueries.h"

#include "core/ExternalReferenceAnnotation.h"
#include "core/PathManager.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "user_types/PrefabInstance.h"

namespace raco::core::PathQueries {

std::string effectiveExternalProjectID(const Project &project, SEditorObject object) {
	if (auto prefabInst = PrefabOperations::findContainingPrefabInstance(object)) {
		if (auto prefab = *prefabInst->template_) {
			if (auto anno = prefab->query<core::ExternalReferenceAnnotation>()) {
				return *anno->projectID_;
			}
		}
	} else {
		if (auto anno = object->query<core::ExternalReferenceAnnotation>()) {
			return *anno->projectID_;
		}
	}
	return std::string();
}  

std::string baseFolderForRelativePath(const Project& project, SEditorObject object) {
	std::string externalProjectID = effectiveExternalProjectID(project, object);
	if (!externalProjectID.empty()) {
		if (project.hasExternalProjectMapping(externalProjectID)) {
			auto projectPath = project.lookupExternalProjectPath(externalProjectID);
			return utils::u8path(projectPath).parent_path().string();
		}
		return std::string();
	}
	return project.currentFolder();
}

std::string resolveUriPropertyToAbsolutePath(const Project& project, const ValueHandle& uri) {
	std::string uriValue = uri.asString();
	if (uriValue.empty()) {
		return std::string();
	}
	std::string externalProjectID = effectiveExternalProjectID(project, uri.rootObject());
	if (!externalProjectID.empty()) {
		if (project.hasExternalProjectMapping(externalProjectID)) {
			auto projectPath = project.lookupExternalProjectPath(externalProjectID);
			auto projectFolder = utils::u8path(projectPath).parent_path().string();
			return utils::u8path(uriValue).normalizedAbsolutePath(projectFolder).string();
		}
		return std::string();
	}

	return utils::u8path(uriValue).normalizedAbsolutePath(project.currentFolder()).string();
}

bool isPathRelativeToCurrentProject(const SEditorObject& object) {
	if (auto prefabInst = PrefabOperations::findContainingPrefabInstance(object)) {
		if (auto prefab = *prefabInst->template_) {
			return prefab->query<core::ExternalReferenceAnnotation>() == nullptr;
		}
	}
	return !object->query<ExternalReferenceAnnotation>();
}

}  // namespace core::PathQueries
