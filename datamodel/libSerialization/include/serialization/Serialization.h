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

#include "data_storage/BasicAnnotations.h"
#include "data_storage/BasicTypes.h"
#include "data_storage/Value.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>

namespace raco::serialization {

using raco::data_storage::PrimitiveType;
using raco::data_storage::ReflectionInterface;
using raco::data_storage::ClassWithReflectedMembers;
using raco::data_storage::ValueBase;
using SReflectionInterface = std::shared_ptr<ReflectionInterface>;

struct ExternalProjectInfo {
	std::string path;
	std::string name;
};

bool operator==(const ExternalProjectInfo& lhs, const ExternalProjectInfo& rhs);

using ResolveReferencedId = std::function<std::optional<std::string>(const raco::data_storage::ValueBase& value)>;
std::string serializeObject(const SReflectionInterface& object, const std::string &projectPath, const ResolveReferencedId& resolveReferenceId);
std::string serializeObjects(const std::vector<SReflectionInterface>& objects, const std::vector<std::string>& rootObjectIDs, const std::vector<SReflectionInterface>& links, const std::string& originFolder, const std::string& originFilename, const std::string& originProjectID, const std::string& originProjectName, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, const std::map<std::string, std::string>& originFolders, const ResolveReferencedId& resolveReferenceId);
QJsonDocument serializeProject(const std::unordered_map<std::string, std::vector<int>>& fileVersions, const std::vector<SReflectionInterface>& instances, const std::vector<SReflectionInterface>& links, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, const ResolveReferencedId& resolveReferenceId);

using UserTypeFactory = std::function<std::shared_ptr<data_storage::ReflectionInterface>(const std::string&)>;
using AnnotationFactory = std::function<std::shared_ptr<data_storage::AnnotationBase>(const std::string&)>;
using ValueBaseFactory = std::function<data_storage::ValueBase*(const std::string&)>;
struct DeserializationFactory {
	UserTypeFactory createUserType;
	AnnotationFactory createAnnotation;
	ValueBaseFactory createValueBase;
};

using References = std::map<raco::data_storage::ValueBase*, std::string>;
struct ObjectDeserialization {
	SReflectionInterface object;
	References references;
};
struct ObjectsDeserialization {
	std::vector<SReflectionInterface> objects;
	// object ids of the top-level (no parents) objects
	std::set<std::string> rootObjectIDs;
	std::vector<SReflectionInterface> links;
	References references;
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

	void operator=(const QJsonArray& jsonArray) {
		major = jsonArray[0].toInt();
		minor = jsonArray[1].toInt();
		patch = jsonArray[2].toInt();
	}
};

struct ProjectDeserializationInfo {
	DeserializedVersion raCoVersion;
	DeserializedVersion ramsesVersion;
	DeserializedVersion ramsesLogicEngineVersion;

	ObjectsDeserialization objectsDeserialization;

	static constexpr int NO_VERSION = -1;
};

ObjectDeserialization deserializeObject(const std::string& json, const DeserializationFactory& factory);
ObjectsDeserialization deserializeObjects(const std::string& json, const DeserializationFactory& factory);
int deserializeFileVersion(const QJsonDocument& document);
ProjectDeserializationInfo deserializeProjectVersionInfo(const QJsonDocument& document);
ProjectDeserializationInfo deserializeProject(const QJsonDocument& jsonDocument, const DeserializationFactory& factory);
ProjectDeserializationInfo deserializeProject(const std::string& json, const DeserializationFactory& factory);

std::optional<QJsonValue> serializePropertyForMigration(const ValueBase& value, const ResolveReferencedId& resolveReferenceId, bool dynamicallyTyped);
References deserializePropertyForMigration(const QJsonValue& property, ValueBase& value, const DeserializationFactory& factory = {});

SReflectionInterface deserializeTypedObject(const QJsonObject& jsonObject, const DeserializationFactory& factory, References& references);
QJsonObject serializeTypedObject(const ReflectionInterface& object, const ResolveReferencedId& resolveReferenceId);
void serializeExternalProjectsMap(QJsonObject& outContainer, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap);


};	// namespace raco::serialization
