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

#include <QJsonDocument>
#include <string>

namespace raco::components {
/**
 * History of versions:
 * 1: Initial
 * 2: Added [core::ProjectSettings].
 * 3: Added [viewport in core::ProjectSettings]
 * 4: Changed indexing of arrays [user_types::SyncTableWithEngineInterface]
 * 5: Added submesh index selection and baking flag [user_types::Mesh]
 *    added texture origin [user_types::Texture] 
 *    removed cubeMap format [user_types::CubeMap]
 * 6: Added URI for shader defines [user_types::Material]
 * 7: Prefab support: 
 *    added prefab child -> instance child map in PrefabInstance
 * 8: Added link validity flag to links
 *    Links can get broken now; these broken links get serialized just like normal links
 * 9: External references
 * 10: The viewport property in cameras is now four individual integers instead of a vec4i,
 *    needed for camera bindings [user_types::BaseCamera].
 */
constexpr int RAMSES_PROJECT_FILE_VERSION = 10;
QJsonDocument migrateProject(const QJsonDocument& doc);
}  // namespace raco::components
