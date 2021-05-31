/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

std::string resolveUriPropertyToAbsolutePath(const Project& project, const ValueHandle& uri) {
	std::string externalProjectID;
	if (auto prefabInst = PrefabOperations::findContainingPrefabInstance(uri.rootObject())) {
		if (auto prefab = *prefabInst->template_) {
			if (auto anno = prefab->query<raco::core::ExternalReferenceAnnotation>()) {
				externalProjectID = *anno->projectID_;
			}
		}
	} else {
		if (auto anno = uri.rootObject()->query<raco::core::ExternalReferenceAnnotation>()) {
			externalProjectID = *anno->projectID_;
		}
	}
	if (!externalProjectID.empty()) {
		if (project.hasExternalProjectMapping(externalProjectID)) {
			auto projectPath = project.lookupExternalProjectPath(externalProjectID);
			auto projectFolder = std::filesystem::path(projectPath).parent_path().generic_string();
			return PathManager::constructAbsolutePath(projectFolder, uri.asString());
		}
		return std::string();
	}

	return PathManager::constructAbsolutePath(project.currentFolder(), uri.asString());
}

}  // namespace raco::core::PathQueries
