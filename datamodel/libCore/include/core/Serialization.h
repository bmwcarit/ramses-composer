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

#include "core/CoreAnnotations.h"
#include "core/BasicAnnotations.h"
#include "core/BasicTypes.h"
#include "core/UserObjectFactoryInterface.h"
#include "data_storage/Value.h"
#include "user_types/UserObjectFactory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>

namespace raco::core {
class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;
class Project;
class Link;
using SLink = std::shared_ptr<Link>;
}  // namespace raco::core

namespace raco::serialization::proxy {
class DynamicEditorObject;
using SDynamicEditorObject = std::shared_ptr<DynamicEditorObject>;
}  // namespace raco::serialization::proxy

namespace raco::serialization {

using data_storage::ClassWithReflectedMembers;
using data_storage::PrimitiveType;
using data_storage::ReflectionInterface;
using data_storage::ValueBase;
using SReflectionInterface = std::shared_ptr<ReflectionInterface>;

struct ExternalProjectInfo {
	std::string path;
	std::string name;
};

bool operator==(const ExternalProjectInfo& lhs, const ExternalProjectInfo& rhs);

std::string serializeProperty(const core::Project& project, const data_storage::ValueBase& value);

std::string serializeObjects(const std::vector<core::SEditorObject>& objects, const std::vector<std::string>& rootObjectIDs, const std::vector<core::SLink>& links, const std::string& originFolder, const std::string& originFilename, const std::string& originProjectID, const std::string& originProjectName, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, const std::map<std::string, std::string>& originFolders, int featureLevel, bool includeVersionInfo = true);

QJsonDocument serializeProject(const std::unordered_map<std::string, std::vector<int>>& fileVersions, int featureLevel, const std::vector<SReflectionInterface>& instances, const std::vector<SReflectionInterface>& links, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap);

using References = std::map<data_storage::ValueBase*, std::string>;
struct ObjectDeserialization {
	core::SEditorObject object;
	References references;
};
struct ObjectsDeserialization {
	std::vector<core::SEditorObject> objects;
	// object ids of the top-level (no parents) objects
	std::set<std::string> rootObjectIDs;
	std::vector<core::SLink> links;
	References references;
	int featureLevel;
	std::string originFolder;
	std::string originFileName;
	std::string originProjectID;
	std::string originProjectName;

	std::string originPath() const;

	std::map<std::string, ExternalProjectInfo> externalProjectsMap;

	// Contains base directory for relative paths for objects _not_ using the originFolder.
	// Maps object ID -> absolute folder.
	// Needed because we can't figure out the correct path for non-extref objects which need to
	// use an external project path as origin folder. This concerns LuaScripts inside PrefabInstances
	// with an external reference Prefab when only the LuaScript but not the external reference
	// Prefab is included in the copied object set.
	std::map<std::string, std::string> objectOriginFolders;
};

struct DeserializedVersion {
	int major;
	int minor;
	int patch;
};

inline bool operator==(const DeserializedVersion& lhVersion, const DeserializedVersion& rhVersion) {
	return lhVersion.major == rhVersion.major && lhVersion.minor == rhVersion.minor && lhVersion.patch == rhVersion.patch;
};

struct ProjectVersionInfo {
	DeserializedVersion raCoVersion;
	DeserializedVersion ramsesVersion;
};

template <class SharedPtrEditorObjectType, class SharedPtrLinkType>
struct GenericProjectDeserializationInfo {
	int fileVersion = NO_VERSION;

	ProjectVersionInfo versionInfo;

	std::vector<SharedPtrEditorObjectType> objects;
	std::vector<SharedPtrLinkType> links;
	std::map<std::string, ExternalProjectInfo> externalProjectsMap;

	std::unordered_map<std::string, std::string> migrationObjWarnings;
	std::string currentPath;

	static constexpr int NO_VERSION = -1;
};

using ProjectDeserializationInfo = GenericProjectDeserializationInfo<core::SEditorObject, core::SLink>;
using ProjectDeserializationInfoIR = GenericProjectDeserializationInfo<serialization::proxy::SDynamicEditorObject, core::SLink>;


int deserializeFileVersion(const QJsonDocument& document);

/**
 * @brief Returns the migrated feature level from a Json document for a .rca project file.
 * 
 * The return value includes the feature level reset at major versions.
*/
int deserializeFeatureLevel(const QJsonDocument& document);
ProjectVersionInfo deserializeProjectVersionInfo(const QJsonDocument& document);

std::unique_ptr<ValueBase> deserializeProperty(const core::Project& project, const std::string& json);

std::optional<ObjectsDeserialization> deserializeObjects(const std::string& json, bool checkVersionInfo = true, core::UserObjectFactoryInterface& factory = user_types::UserObjectFactory::getInstance());

ProjectDeserializationInfo deserializeProject(const QJsonDocument& jsonDocument, const std::string& filename);

std::map<std::string, std::map<std::string, std::string>> makeUserTypePropertyMap(core::UserObjectFactoryInterface& objectFactory = user_types::UserObjectFactory::getInstance());
std::map<std::string, std::map<std::string, std::string>> makeStructPropertyMap();
std::map<std::string, std::map<std::string, std::string>> deserializeUserTypePropertyMap(const QVariant& container);

ProjectDeserializationInfoIR deserializeProjectToIR(const QJsonDocument& document, const std::string& filename);

namespace test_helpers {

std::string serializeObject(const SReflectionInterface& object, const std::string& projectPath = {});

ObjectDeserialization deserializeObject(const std::string& json, core::UserObjectFactoryInterface& objectFactory = user_types::UserObjectFactory::getInstance());


}  // namespace test_helpers

};	// namespace raco::serialization
