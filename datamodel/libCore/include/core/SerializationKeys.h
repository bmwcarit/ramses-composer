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

namespace raco::serialization::keys {

constexpr const char* TYPENAME{"typeName"};
constexpr const char* OBJECTS{"objects"};
constexpr const char* LINKS{"links"};
constexpr const char* ORDER{"order"};
constexpr const char* PROPERTIES{"properties"};
constexpr const char* VALUE{"value"};
constexpr const char* ANNOTATIONS{"annotations"};
constexpr const char* FILE_VERSION{"fileVersion"};
constexpr const char* RAMSES_VERSION{"ramsesVersion"};
constexpr const char* RAMSES_LOGIC_ENGINE_VERSION{"logicEngineVersion"};
constexpr const char* RAMSES_COMPOSER_VERSION{"racoVersion"};
constexpr const char* INSTANCES{"instances"};
constexpr const char* EXTERNAL_PROJECTS{"externalProjects"};
constexpr const char* ORIGIN_FOLDER{"originFolder"};
constexpr const char* ORIGIN_FILENAME{"originFilename"};
constexpr const char* ORIGIN_PROJECT_ID{"originProjectID"};
constexpr const char* ORIGIN_PROJECT_NAME{"originProjectName"};
constexpr const char* EXTERNAL_PROJECT_PATH{"path"};
constexpr const char* EXTERNAL_PROJECT_NAME{"name"};
constexpr const char* ROOT_OBJECT_IDS{"rootObjectIDs"};
constexpr const char* OBJECT_ORIGIN_FOLDERS{"objectOriginFolders"};
constexpr const char* USER_TYPE_PROP_MAP{"userTypePropMap"};
constexpr const char* STRUCT_PROP_MAP{"structPropMap"};
constexpr const char* FEATURE_LEVEL{"featureLevel"};

}	// namespace raco::serialization::keys
