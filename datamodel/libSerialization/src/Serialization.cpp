/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "serialization/Serialization.h"
#include "serialization/SerializationKeys.h"

#include "data_storage/Table.h"
#include "utils/stdfilesystem.h"
#include "log_system/log.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace raco::serialization;

namespace raco::serialization {

bool operator==(const ExternalProjectInfo& lhs, const ExternalProjectInfo& rhs) {
	return lhs.path == rhs.path && lhs.name == rhs.name;
}

}  // namespace raco::serialization

namespace {

/**
 * Serialize a value base to it's primitive json counterpart. This function also maps references to the associated id via `resolveReferenceId`.
 * @return QJsonValue for the given `value`. Will be [QJsonValue::Null] if the `value` type cannot be mapped to a [QJsonValue] type. 
 */
QJsonValue serializePrimitiveValue(const ValueBase& value, const ResolveReferencedId& resolveReferenceId) {
	switch (value.type()) {
		case PrimitiveType::Bool:
			return QJsonValue{value.asBool()};
		case PrimitiveType::Double:
			return QJsonValue{value.asDouble()};
		case PrimitiveType::Int:
			return QJsonValue{value.asInt()};
		case PrimitiveType::String: {
			assert(value.asString() == QString::fromStdString(value.asString()).toStdString());
			assert(value.asString() == QJsonValue{QString::fromStdString(value.asString())}.toString().toStdString());
			return QJsonValue{QString::fromStdString(value.asString())};
		}
		case PrimitiveType::Ref:
			if (const auto id{resolveReferenceId(value)}) {
				return QJsonValue{id.value().c_str()};
			} else {
				return QJsonValue::Null;
			}
		default:
			return QJsonValue::Null;
	}
}

/** Deserializes result of `serializeArrayProperties` from `properties` into the given `interface` or into `references` if the type is a PrimitiveType::Ref. */
void deserializePrimitiveValue(const QJsonValue& jsonValue, ValueBase& value, References& references) {
	switch (value.type()) {
		case PrimitiveType::Bool:
			value = jsonValue.toBool();
			break;
		case PrimitiveType::Double:
			value = jsonValue.toDouble();
			break;
		case PrimitiveType::Int:
			value = jsonValue.toInt();
			break;
		case PrimitiveType::String:
			value = jsonValue.toString().toStdString();
			break;
		case PrimitiveType::Ref:
			if (!jsonValue.isNull()) {
				references[&value] = jsonValue.toString().toStdString();
			}
			break;
		default:
			break;
	}
}

QJsonObject serializeTypedObject(const ReflectionInterface& object, const ResolveReferencedId& resolveReferenceId);

/** Serializations of annotations need to be handled separately. This is the only case where we require to serialize a typed object within a typed property. */
std::optional<QJsonArray> serializeAnnotations(const std::vector<raco::data_storage::AnnotationBase*>& annotations, const ResolveReferencedId& resolveReferenceId, bool dynamicallyTyped) {
	QJsonArray jsonArray{};
	for (auto anno : annotations) {
		if (anno->serializationRequired() || dynamicallyTyped) {
			jsonArray.push_back(serializeTypedObject(*anno, resolveReferenceId));
		}
	}
	if (jsonArray.size() > 0) {
		return jsonArray;
	} else {
		return {};
	}
}

void deserializeObjectProperties(const QJsonObject& properties, ReflectionInterface& objectInterface, References& references, const DeserializationFactory& factory, bool dynamicallyTyped);
void deserializeArrayProperties(const QJsonArray& properties, ReflectionInterface& arrayInterface, References& references, const DeserializationFactory& factory, bool dynamicallyType);

/** Deserializes result of `serializeAnnotations` from `annotations` into the annotations of the given `value`. */
void deserializeAnnotations(const QJsonArray& annotations, const ValueBase& value, References& references, const DeserializationFactory& factory) {
	for (const auto& annotation : annotations) {
		auto it = std::find_if(value.baseAnnotationPtrs().begin(), value.baseAnnotationPtrs().end(), [&annotation](const raco::data_storage::AnnotationBase* annoBase) {
			return annoBase->getTypeDescription().typeName == annotation[keys::TYPENAME].toString().toStdString();
		});
		deserializeObjectProperties(annotation[keys::PROPERTIES].toObject(), **it, references, factory, false);
	}
}

std::optional<QJsonArray> serializeObjectAnnotations(const ClassWithReflectedMembers* object, const ResolveReferencedId& resolveReferenceId) {
	QJsonArray jsonArray{};
	for (auto anno : object->annotations()) {
		jsonArray.push_back(serializeTypedObject(*anno, resolveReferenceId));
	}
	if (jsonArray.size() > 0) {
		return jsonArray;
	} else {
		return {};
	}
}

std::shared_ptr<raco::data_storage::AnnotationBase> deserializeSingleObjectAnnotation(const QJsonObject& jsonObject, const DeserializationFactory& factory, References& references) {
	auto object{factory.createAnnotation(jsonObject[keys::TYPENAME].toString().toStdString())};
	deserializeObjectProperties(jsonObject[keys::PROPERTIES].toObject(), *object.get(), references, factory, false);
	return object;
}

void deserializeObjectAnnotations(const QJsonArray& annotations, ClassWithReflectedMembers* object, References& references, const DeserializationFactory& factory) {
	for (const auto& annotation : annotations) {
		auto deserializedAnno = deserializeSingleObjectAnnotation(annotation.toObject(), factory, references);
		object->addAnnotation(deserializedAnno);
	}
}


std::optional<QJsonObject> serializeObjectProperties(const ReflectionInterface& objectInterface, const ResolveReferencedId& resolveReferenceId, bool dynamicallyType);
std::optional<QJsonArray> serializeArrayProperties(const ReflectionInterface& arrayInterface, const ResolveReferencedId& resolveReferenceId, bool dynamicallyType);

/**
 * Main decision function for the seralization. Will call serializeArrayProperties, serializeObjectProperties or serializePrimitiveValue based on the type of `value`.
 * Further more compression will be applied based on how much information is needed for deseralization (e.g. `dynamicallyType`).
 * @return a QJsonValue which seralize the given `value`, based on the type of the `value` the returned QJsonValue can either be an Object, Array or an actual Value.
 */
std::optional<QJsonValue> serializeValueBase(const ValueBase& value, const ResolveReferencedId& resolveReferenceId, bool dynamicallyTyped = false) {
	bool childrenDynamicallyTyped{value.type() == PrimitiveType::Table};
	bool valueIsClassType{hasTypeSubstructure(value.type())};
	auto annotations{serializeAnnotations(value.baseAnnotationPtrs(), resolveReferenceId, dynamicallyTyped)};
	if (dynamicallyTyped || annotations || childrenDynamicallyTyped) {
		// We need a object which holds typeName, properties/value and annotations.
		QJsonObject jsonObject{};
		if (dynamicallyTyped)
			jsonObject.insert(keys::TYPENAME, value.typeName().c_str());

		if (valueIsClassType) {
			if (value.query<raco::data_storage::ArraySemanticAnnotation>()) {
				if (auto properties{serializeArrayProperties(value.getSubstructure(), resolveReferenceId, childrenDynamicallyTyped)}) {
					jsonObject.insert(keys::PROPERTIES, properties.value());
				}
			} else {
				if (auto properties{serializeObjectProperties(value.getSubstructure(), resolveReferenceId, childrenDynamicallyTyped)}) {
					if (childrenDynamicallyTyped) {
						QJsonArray order{};
						for (size_t i{0}; i < value.getSubstructure().size(); i++) {
							order.push_back(value.getSubstructure().name(i).c_str());
						}
						jsonObject.insert(keys::ORDER, order);
					}
					jsonObject.insert(keys::PROPERTIES, properties.value());
				}
			}
		} else {
			jsonObject.insert(keys::VALUE, serializePrimitiveValue(value, resolveReferenceId));
		}
		if (annotations) {
			jsonObject.insert(keys::ANNOTATIONS, annotations.value());
		}
		if (jsonObject.size() > 0)
			return jsonObject;
		return {};
	} else if (valueIsClassType) {
		// We have a statically known class type which can be immediately seralized
		if (value.query<raco::data_storage::ArraySemanticAnnotation>()) {
			return serializeArrayProperties(value.getSubstructure(), resolveReferenceId, false);
		} else {
			return serializeObjectProperties(value.getSubstructure(), resolveReferenceId, false);
		}
	} else {
		// we have a primitive value which can be naivly serialized
		return serializePrimitiveValue(value, resolveReferenceId);
	}
}

void createMissingProperties(const QJsonArray& order, const QJsonObject& jsonObject, raco::data_storage::Table& table, const DeserializationFactory& factory) {
	for (const auto& qPropertyName : order) {
		const std::string propertyName{qPropertyName.toString().toStdString()};
		if (!table.hasProperty(propertyName)) {
			const std::string typeName{jsonObject[qPropertyName.toString()].toObject()[keys::TYPENAME].toString().toStdString()};
			if (raco::data_storage::isPrimitiveTypeName(typeName)) {
				table.addProperty(propertyName, raco::data_storage::toPrimitiveType(typeName));
			} else {
				// typeName: REF::Material
				table.addProperty(propertyName, factory.createValueBase(typeName));
			}
		}
	}
}

void createMissingProperties(const QJsonArray& jsonArray, raco::data_storage::Table& table, const DeserializationFactory& factory) {
	for (size_t i{0}; i < jsonArray.size(); i++) {
		if (!table[i]) {
			const std::string typeName{jsonArray[static_cast<int>(i)].toObject()[keys::TYPENAME].toString().toStdString()};
			if (raco::data_storage::isPrimitiveTypeName(typeName)) {
				table.addProperty(raco::data_storage::toPrimitiveType(typeName));
			} else {
				table.addProperty(factory.createValueBase(typeName));
			}
		}
	}
}

/**  Deserializes result of `serializeValueBase` from `property` into the given `value`. */
void deserializeValueBase(const QJsonValue& property, ValueBase& value, References& references, const DeserializationFactory& factory, bool dynamicallyTyped = false) {
	bool childrenDynamicallyTyped{value.type() == PrimitiveType::Table};
	auto valueIsClassType{hasTypeSubstructure(value.type())};
	auto hasAnnotations{property.isObject() && property.toObject().keys().contains(keys::ANNOTATIONS)};

	LOG_TRACE(raco::log_system::DESERIALIZATION, "{}, {}, {}", valueIsClassType, hasAnnotations, dynamicallyTyped);

	if (dynamicallyTyped || hasAnnotations || childrenDynamicallyTyped) {
		auto propertyAsObject{property.toObject()};
		if (valueIsClassType) {
			if (propertyAsObject[keys::PROPERTIES].isArray()) {
				if (value.type() == PrimitiveType::Table) {
					createMissingProperties(propertyAsObject[keys::PROPERTIES].toArray(), value.asTable(), factory);
				}
				deserializeArrayProperties(propertyAsObject[keys::PROPERTIES].toArray(), value.getSubstructure(), references, factory, childrenDynamicallyTyped);
			} else {
				if (value.type() == PrimitiveType::Table) {
					createMissingProperties(propertyAsObject[keys::ORDER].toArray(), propertyAsObject[keys::PROPERTIES].toObject(), value.asTable(), factory);
				}
				deserializeObjectProperties(propertyAsObject[keys::PROPERTIES].toObject(), value.getSubstructure(), references, factory, childrenDynamicallyTyped);
			}
		} else {
			deserializePrimitiveValue(propertyAsObject[keys::VALUE], value, references);
		}
		if (hasAnnotations) {
			deserializeAnnotations(propertyAsObject[keys::ANNOTATIONS].toArray(), value, references, factory);
		}
	} else if (valueIsClassType) {
		if (property.isArray()) {
			deserializeArrayProperties(property.toArray(), value.getSubstructure(), references, factory, false);
		} else {
			deserializeObjectProperties(property.toObject(), value.getSubstructure(), references, factory, false);
		}
	} else {
		deserializePrimitiveValue(property, value, references);
	}
}

/**
 * Serializes the properties of the given `interface` into a [QJsonArray]. The result is has the form (e.g. Node):
 * [ { typeName: "Ref", "value": "mesh_node_id" }, { typeName: "Ref", "value": "mesh_node_2_id" }, ... ].
 * 
 * @return a QJsonArray which contains all properties which are accessable in the `interface`. Will return an empty optional if serialized array is empty.
 */
std::optional<QJsonArray> serializeArrayProperties(const ReflectionInterface& arrayInterface, const ResolveReferencedId& resolveReferenceId, bool dynamicallyTyped = false) {
	QJsonArray properties{};
	for (size_t i{0}; i < arrayInterface.size(); i++) {
		const auto& property{arrayInterface.get(i)};
		if (auto child{serializeValueBase(*property, resolveReferenceId, dynamicallyTyped)}) {
			properties.push_back(child.value());
		}
	}
	if (properties.size() > 0) {
		return properties;
	} else {
		return {};
	}
}

/** Deserializes result of `serializeArrayProperties` from `properties` into the given `interface`. */
void deserializeArrayProperties(const QJsonArray& properties, ReflectionInterface& arrayInterface, References& references, const DeserializationFactory& factory, bool dynamicallyTyped = false) {
	for (size_t i{0}; i < properties.size(); i++) {
		deserializeValueBase(properties[static_cast<int>(i)].toObject(), *arrayInterface.get(i), references, factory, dynamicallyTyped);
	}
}

/**
 * Serializes the properties of the given `interface` into a [QJsonObject]. The result is has the form (e.g. Node):
 * { "objectID": "someID", "objectName": "someName", "translation": {...}, ... }
 * @return a QJsonObject which contains all properties which are accessable in the `interface`. Will return an empty optional if serialized object is empty.
 */
std::optional<QJsonObject> serializeObjectProperties(const ReflectionInterface& propertiesInterface, const ResolveReferencedId& resolveReferenceId, bool dynamicallyTyped = false) {
	QJsonObject properties{};
	for (size_t i{0}; i < propertiesInterface.size(); i++) {
		const auto& property{propertiesInterface.get(i)};
		if (auto serializedValue{serializeValueBase(*property, resolveReferenceId, dynamicallyTyped)}) {
			properties.insert(propertiesInterface.name(i).c_str(), serializedValue.value());
		}
	}
	if (properties.size() > 0) {
		return properties;
	} else {
		return {};
	}
}

/** Deserializes result of `serializeObjectProperties` from `properties` into the given `interface`. */
void deserializeObjectProperties(const QJsonObject& properties, ReflectionInterface& objectInterface, References& references, const DeserializationFactory& factory, bool dynamicallyTyped = false) {
	for (const auto& qPropertyName : properties.keys()) {
		std::string name = qPropertyName.toStdString();
		LOG_TRACE(raco::log_system::DESERIALIZATION, "{}, {}", name, dynamicallyTyped);
		ValueBase &value = *objectInterface.get(objectInterface.index(name));
		if (&value != nullptr) {
			deserializeValueBase(properties[qPropertyName], value, references, factory, dynamicallyTyped);
		} else {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "Dropping unsupported or deprecated property {}", name );
		}
	}
}

/**
 * We have some form of C++ Object. Either an EditorObject or an Annotation.
 * @return QJsonObject of form e.g.: { "typeName": "MeshNode", "properties": { ... } }
 */
QJsonObject serializeTypedObject(const ReflectionInterface& object, const ResolveReferencedId& resolveReferenceId) {
	QJsonObject jsonObject{};
	jsonObject.insert(keys::TYPENAME, object.serializationTypeName().c_str());
	
	auto cwrm = dynamic_cast<const ClassWithReflectedMembers*>(&object);
	if (cwrm) {
		auto annotations{serializeObjectAnnotations(cwrm, resolveReferenceId)};
		if (annotations) {
			jsonObject.insert(keys::ANNOTATIONS, annotations.value());
		}
	}

	if (auto properties{serializeObjectProperties(object, resolveReferenceId)}) {
		jsonObject.insert(keys::PROPERTIES, properties.value());
	}
	return jsonObject;
}

/**
 * We have some form of C++ Object. Either an EditorObject or an Annotation. E.g.:
 * { "typeName": "MeshNode", "properties": { ... } }
 * @return an Object created by the `factory` for the given `jsonObject`.
 */
SReflectionInterface deserializeTypedObject(const QJsonObject& jsonObject, const DeserializationFactory& factory, References& references) {
	auto object{factory.createUserType(jsonObject[keys::TYPENAME].toString().toStdString())};

	if (jsonObject.keys().contains(keys::ANNOTATIONS)) {
		deserializeObjectAnnotations(jsonObject[keys::ANNOTATIONS].toArray(),
			std::dynamic_pointer_cast<raco::data_storage::ClassWithReflectedMembers>(object).get(),
			references, factory);
	}

	deserializeObjectProperties(jsonObject[keys::PROPERTIES].toObject(), *object.get(), references, factory);
	return object;
}

/**
 * Deserialize and log the values of version arrays that consist of the values [major.minor.patch].
 * This includes Ramses, Logic Engine, and RaCo versions.
 * @return a QJsonArray holding the version array values.
 */
QJsonArray deserializeVersionNumberArray(const QJsonDocument& document, const char* jsonVersionKey, const char* whichVersion) {
	auto versionNums = QJsonArray{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION};

	if (document[jsonVersionKey].isUndefined()) {
		LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version is not saved in project file", whichVersion);
	} else {
		auto deserializedVersionNums = document[jsonVersionKey].toArray();
		if (deserializedVersionNums.size() < 3) {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version has not been saved correctly in project file - not enough version values", whichVersion);
		} else if (deserializedVersionNums.size() > 3) {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version has not been saved correctly in project file - too many version values", whichVersion);
		}

		for (int i = 0; i < std::min(deserializedVersionNums.size(), versionNums.size()); ++i) {
			versionNums[i] = deserializedVersionNums[i];
		}
	}

	LOG_INFO(raco::log_system::DESERIALIZATION, "{} version from project file is {}.{}.{}", whichVersion, versionNums[0].toInt(), versionNums[1].toInt(), versionNums[2].toInt());
	return versionNums;
};

}  // namespace

std::string raco::serialization::serializeObject(const SReflectionInterface& object, const std::string& originPath, const ResolveReferencedId& resolveReferenceId) {
	return QJsonDocument{serializeTypedObject(*object.get(), resolveReferenceId)}.toJson().toStdString();
}

std::string ObjectsDeserialization::originPath() const {
	return (std::filesystem::path(originFolder) / originFileName).generic_string();
}

ObjectDeserialization raco::serialization::deserializeObject(const std::string& json, const DeserializationFactory& factory) {
	References references{};
	return {
		deserializeTypedObject(QJsonDocument::fromJson(json.c_str()).object(), factory, references),
		references};
}

void serializeExternalProjectsMap(QJsonObject& outContainer, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap) {
	QMap<QString, QVariant> map;
	for (auto [id, info] : externalProjectsMap) {
		auto qvinfo = QVariantMap({{keys::EXTERNAL_PROJECT_PATH, QString::fromStdString(info.path)}, 
			{keys::EXTERNAL_PROJECT_NAME, QString::fromStdString(info.name)}});
		map[QString::fromStdString(id)] = QVariant(qvinfo);
	}
	auto vmap = QVariant(map);
	outContainer.insert(keys::EXTERNAL_PROJECTS, QJsonValue::fromVariant(map));
}

void deserializeExternalProjectsMap(const QVariant& container, std::map<std::string, ExternalProjectInfo>& outMap) {
	for (auto [id, info] : container.toMap().toStdMap()) {
		auto qvinfo = info.toMap();
		outMap[id.toStdString()] = ExternalProjectInfo{qvinfo[keys::EXTERNAL_PROJECT_PATH].toString().toStdString(), qvinfo[keys::EXTERNAL_PROJECT_NAME].toString().toStdString()};
	}
}

void serializeObjectOriginFolderMap(QJsonObject& outContainer, const std::map<std::string, std::string>& originFolders) {
	QMap<QString, QVariant> map;
	for (auto [id, folder] : originFolders) {
		map[QString::fromStdString(id)] = QVariant(QString::fromStdString(folder));
	}
	outContainer.insert(keys::OBJECT_ORIGIN_FOLDERS, QJsonValue::fromVariant(map));
}

void deserializeObjectOriginFolderMap(const QVariant& container, std::map<std::string, std::string>& outOriginFolders) {
	for (auto [id, folder] : container.toMap().toStdMap()) {
		outOriginFolders[id.toStdString()] = folder.toString().toStdString();
	}
}

ObjectsDeserialization raco::serialization::deserializeObjects(const std::string& json, const DeserializationFactory& factory) {
	ObjectsDeserialization result{};
	auto container{QJsonDocument::fromJson(json.c_str()).object()};
	if (!container[keys::ORIGIN_FOLDER].isUndefined() && !container[keys::ORIGIN_FOLDER].toString().isEmpty()) {
		result.originFolder = container[keys::ORIGIN_FOLDER].toString().toStdString();
	}
	if (!container[keys::ORIGIN_FILENAME].isUndefined() && !container[keys::ORIGIN_FILENAME].toString().isEmpty()) {
		result.originFileName = container[keys::ORIGIN_FILENAME].toString().toStdString();
	}
	if (!container[keys::ORIGIN_PROJECT_ID].isUndefined() && !container[keys::ORIGIN_PROJECT_ID].toString().isEmpty()) {
		result.originProjectID = container[keys::ORIGIN_PROJECT_ID].toString().toStdString();
	}
	if (!container[keys::ORIGIN_PROJECT_NAME].isUndefined() && !container[keys::ORIGIN_PROJECT_NAME].toString().isEmpty()) {
		result.originProjectName = container[keys::ORIGIN_PROJECT_NAME].toString().toStdString();
	}

	deserializeExternalProjectsMap(container[keys::EXTERNAL_PROJECTS].toVariant(), result.externalProjectsMap);
	deserializeObjectOriginFolderMap(container[keys::OBJECT_ORIGIN_FOLDERS].toVariant(), result.objectOriginFolders);

	auto rootObjectIDs = container[keys::ROOT_OBJECT_IDS].toVariant().toStringList();
	for (auto id : rootObjectIDs) {
		result.rootObjectIDs.insert(id.toStdString());
	}

	for (const auto& objJson : container.value(keys::OBJECTS).toArray()) {
		auto deserializeObject{deserializeTypedObject(objJson.toObject(), factory, result.references)};
		result.objects.push_back(deserializeObject);
	}
	for (const auto& linkJson : container.value(keys::LINKS).toArray()) {
		auto deserializeObject{deserializeTypedObject(linkJson.toObject(), factory, result.references)};
		result.links.push_back(deserializeObject);
	}
	return result;
}


std::string raco::serialization::serializeObjects(const std::vector<SReflectionInterface>& objects, const std::vector<std::string>& rootObjectIDs, const std::vector<SReflectionInterface>& links, const std::string& originFolder, const std::string& originFilename, const std::string& originProjectID, const std::string& originProjectName, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, const std::map<std::string, std::string>& originFolders, const ResolveReferencedId& resolveReferenceId) {
	QJsonObject result{};

	if (!originFolder.empty()) {
		result.insert(keys::ORIGIN_FOLDER, originFolder.c_str());
	}
	if (!originFilename.empty()) {
		result.insert(keys::ORIGIN_FILENAME, originFilename.c_str());
	}
	if (!originProjectID.empty()) {
		result.insert(keys::ORIGIN_PROJECT_ID, originProjectID.c_str());
	}
	if (!originProjectName.empty()) {
		result.insert(keys::ORIGIN_PROJECT_NAME, originProjectName.c_str());
	}

	serializeExternalProjectsMap(result, externalProjectsMap);
	
	serializeObjectOriginFolderMap(result, originFolders);

	QStringList qRootObjectIDs;
	for (auto id : rootObjectIDs) {
		qRootObjectIDs.push_back(QString::fromStdString(id));
	}
	result.insert(keys::ROOT_OBJECT_IDS, QJsonValue::fromVariant(qRootObjectIDs));

	QJsonArray jsonObjects{};
	for (const auto& object : objects) {
		jsonObjects.push_back(serializeTypedObject(*object.get(), resolveReferenceId));
	}
	result.insert(keys::OBJECTS, jsonObjects);

	QJsonArray jsonLinks{};
	for (const auto& link : links) {
		jsonLinks.push_back(serializeTypedObject(*link.get(), resolveReferenceId));
	}
	result.insert(keys::LINKS, jsonLinks);

	return QJsonDocument{result}.toJson().toStdString();
}

QJsonDocument raco::serialization::serializeProject(const std::unordered_map<std::string, std::vector<int>>& fileVersions, const std::vector<SReflectionInterface>& instances, const std::vector<SReflectionInterface>& links, 
	const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, 
	const ResolveReferencedId& resolveReferenceId) {
	QJsonObject container{};

	auto ramsesVer = fileVersions.at(keys::RAMSES_VERSION);
	auto logicEngineVer = fileVersions.at(keys::RAMSES_LOGIC_ENGINE_VERSION);
	auto raCoVersion = fileVersions.at(keys::RAMSES_COMPOSER_VERSION);
	container.insert(keys::FILE_VERSION, fileVersions.at(keys::FILE_VERSION)[0]);
	container.insert(keys::RAMSES_VERSION, QJsonArray{ramsesVer[0], ramsesVer[1], ramsesVer[2]});
	container.insert(keys::RAMSES_LOGIC_ENGINE_VERSION, QJsonArray{logicEngineVer[0], logicEngineVer[1], logicEngineVer[2]});
	container.insert(keys::RAMSES_COMPOSER_VERSION, QJsonArray{raCoVersion[0], raCoVersion[1], raCoVersion[2]});

	serializeExternalProjectsMap(container, externalProjectsMap);

	QJsonArray objectArray{};
	for (const auto& object : instances) {
		objectArray.push_back(serializeTypedObject(*object.get(), resolveReferenceId));
	}
	container.insert(keys::INSTANCES, objectArray);
	QJsonArray linkArray{};
	for (const auto& link : links) {
		linkArray.push_back(serializeTypedObject(*link.get(), resolveReferenceId));
	}
	container.insert(keys::LINKS, linkArray);
	return QJsonDocument{container};
}

ProjectDeserializationInfo raco::serialization::deserializeProject(const QJsonDocument& document, const DeserializationFactory& factory) {
	ProjectDeserializationInfo deserializedProjectInfo;

	deserializedProjectInfo.ramsesVersion = deserializeVersionNumberArray(document, keys::RAMSES_VERSION, "Ramses");
	deserializedProjectInfo.ramsesLogicEngineVersion = deserializeVersionNumberArray(document, keys::RAMSES_LOGIC_ENGINE_VERSION, "Ramses Logic Engine");
	deserializedProjectInfo.raCoVersion = deserializeVersionNumberArray(document, keys::RAMSES_COMPOSER_VERSION, "Ramses Composer");

	deserializeExternalProjectsMap(document[keys::EXTERNAL_PROJECTS].toVariant(), deserializedProjectInfo.objectsDeserialization.externalProjectsMap);

	const auto instances = document[keys::INSTANCES].toArray();
	deserializedProjectInfo.objectsDeserialization.objects.reserve(instances.size());
	for (const auto& instance : instances) {
		deserializedProjectInfo.objectsDeserialization.objects.push_back(deserializeTypedObject(instance.toObject(), factory, deserializedProjectInfo.objectsDeserialization.references));
	}
	const auto links = document[keys::LINKS].toArray();
	deserializedProjectInfo.objectsDeserialization.links.reserve(links.size());
	for (const auto& link : links) {
		deserializedProjectInfo.objectsDeserialization.links.push_back(deserializeTypedObject(link.toObject(), factory, deserializedProjectInfo.objectsDeserialization.references));
	}
	return deserializedProjectInfo;
}

int raco::serialization::deserializeFileVersion(const QJsonDocument& document) {
	return document.object()[keys::FILE_VERSION].toInt();
}

ProjectDeserializationInfo raco::serialization::deserializeProject(const std::string& json, const DeserializationFactory& factory) {
	return deserializeProject(QJsonDocument::fromJson(json.c_str()), factory);
}

std::optional<QJsonValue> raco::serialization::serializePropertyForMigration(const ValueBase& value, const ResolveReferencedId& resolveReferenceId) {
	return serializeValueBase(value, resolveReferenceId);
}

References raco::serialization::deserializePropertyForMigration(const QJsonValue& property, ValueBase& value, const DeserializationFactory& factory) {
	References references{};
	deserializeValueBase(property, value, references, factory);
	return references;
}

