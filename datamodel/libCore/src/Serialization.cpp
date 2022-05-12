/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Serialization.h"

#include "core/DynamicEditorObject.h"
#include "core/EditorObject.h"
#include "core/Link.h"
#include "core/ProjectMigration.h"
#include "core/ProjectMigrationToV23.h"
#include "core/ProxyObjectFactory.h"
#include "core/SerializationKeys.h"
#include "core/UserObjectFactoryInterface.h"
#include "user_types/UserObjectFactory.h"

#include "data_storage/Table.h"
#include "log_system/log.h"
#include "utils/stdfilesystem.h"
#include "utils/u8path.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace raco::serialization;
using namespace raco::data_storage;

namespace raco::serialization {

bool operator==(const ExternalProjectInfo& lhs, const ExternalProjectInfo& rhs) {
	return lhs.path == rhs.path && lhs.name == rhs.name;
}

std::optional<std::string> resolveReferenceId(const raco::data_storage::ValueBase& value) {
	if (auto ref = value.asRef()) {
		return ref->objectID();
	} else {
		return {};
	}
}

}  // namespace raco::serialization

namespace {

QJsonObject serializeTypedObject(const ReflectionInterface& object);

SReflectionInterface deserializeTypedObject(const QJsonObject& jsonObject, const raco::core::UserObjectFactoryInterface& factory, References& references);

/**
 * Serialize a value base to it's primitive json counterpart. This function also maps references to the associated id via `resolveReferenceId`.
 * @return QJsonValue for the given `value`. Will be [QJsonValue::Null] if the `value` type cannot be mapped to a [QJsonValue] type. 
 */
QJsonValue serializePrimitiveValue(const ValueBase& value) {
	switch (value.type()) {
		case PrimitiveType::Bool:
			return QJsonValue{value.asBool()};
		case PrimitiveType::Double:
			return QJsonValue{value.asDouble()};
		case PrimitiveType::Int:
			return QJsonValue{value.asInt()};
		case PrimitiveType::Int64:
			return QJsonValue{QString::number(value.asInt64())};
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
		case PrimitiveType::Int64:
			// This cast seems to be necessary for the linux compiler to be able to decide which assignment operator to use
			value = (int64_t)jsonValue.toString().toLongLong();
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

/** Serializations of annotations need to be handled separately. This is the only case where we require to serialize a typed object within a typed property. */
std::optional<QJsonArray> serializeAnnotations(const std::vector<raco::data_storage::AnnotationBase*>& annotations, bool dynamicallyTyped) {
	QJsonArray jsonArray{};
	for (auto anno : annotations) {
		if (anno->serializationRequired() || dynamicallyTyped) {
			jsonArray.push_back(serializeTypedObject(*anno));
		}
	}
	if (jsonArray.size() > 0) {
		return jsonArray;
	} else {
		return {};
	}
}

void deserializeObjectProperties(const QJsonObject& properties, ReflectionInterface& objectInterface, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, bool dynamicallyTyped);
void deserializeArrayProperties(const QJsonArray& properties, ReflectionInterface& arrayInterface, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, bool dynamicallyType);

/** Deserializes result of `serializeAnnotations` from `annotations` into the annotations of the given `value`. */
void deserializeAnnotations(const QJsonArray& annotations, const ValueBase& value, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap) {
	for (const auto& annotation : annotations) {
		auto it = std::find_if(value.baseAnnotationPtrs().begin(), value.baseAnnotationPtrs().end(), [&annotation](const raco::data_storage::AnnotationBase* annoBase) {
			return annoBase->getTypeDescription().typeName == annotation[keys::TYPENAME].toString().toStdString();
		});
		deserializeObjectProperties(annotation[keys::PROPERTIES].toObject(), **it, references, factory, structPropTypesMap, false);
	}
}

std::optional<QJsonArray> serializeObjectAnnotations(const ClassWithReflectedMembers* object) {
	QJsonArray jsonArray{};
	for (auto anno : object->annotations()) {
		jsonArray.push_back(serializeTypedObject(*anno));
	}
	if (jsonArray.size() > 0) {
		return jsonArray;
	} else {
		return {};
	}
}

std::shared_ptr<raco::data_storage::AnnotationBase> deserializeSingleObjectAnnotation(const QJsonObject& jsonObject, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, References& references) {
	auto object{factory.createAnnotation(jsonObject[keys::TYPENAME].toString().toStdString())};
	deserializeObjectProperties(jsonObject[keys::PROPERTIES].toObject(), *object.get(), references, factory, structPropTypesMap, false);
	return object;
}

void deserializeObjectAnnotations(const QJsonArray& annotations, ClassWithReflectedMembers* object, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap) {
	for (const auto& annotation : annotations) {
		auto deserializedAnno = deserializeSingleObjectAnnotation(annotation.toObject(), factory, structPropTypesMap, references);
		object->addAnnotation(deserializedAnno);
	}
}

std::optional<QJsonObject> serializeObjectProperties(const ReflectionInterface& objectInterface, bool dynamicallyType);
std::optional<QJsonArray> serializeArrayProperties(const ReflectionInterface& arrayInterface, bool dynamicallyType);

/**
 * Main decision function for the seralization. Will call serializeArrayProperties, serializeObjectProperties or serializePrimitiveValue based on the type of `value`.
 * Further more compression will be applied based on how much information is needed for deseralization (e.g. `dynamicallyType`).
 * @return a QJsonValue which seralize the given `value`, based on the type of the `value` the returned QJsonValue can either be an Object, Array or an actual Value.
 */
std::optional<QJsonValue> serializeValueBase(const ValueBase& value, bool dynamicallyTyped = false) {
	bool childrenDynamicallyTyped{value.type() == PrimitiveType::Table};
	bool valueIsClassType{hasTypeSubstructure(value.type())};
	auto annotations{serializeAnnotations(value.baseAnnotationPtrs(), dynamicallyTyped)};
	if (dynamicallyTyped || annotations || childrenDynamicallyTyped) {
		// We need a object which holds typeName, properties/value and annotations.
		QJsonObject jsonObject{};
		if (dynamicallyTyped)
			jsonObject.insert(keys::TYPENAME, value.typeName().c_str());

		if (valueIsClassType) {
			if (value.query<raco::core::ArraySemanticAnnotation>()) {
				if (auto properties{serializeArrayProperties(value.getSubstructure(), childrenDynamicallyTyped)}) {
					jsonObject.insert(keys::PROPERTIES, properties.value());
				}
			} else {
				if (auto properties{serializeObjectProperties(value.getSubstructure(), childrenDynamicallyTyped)}) {
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
			jsonObject.insert(keys::VALUE, serializePrimitiveValue(value));
		}
		if (annotations) {
			jsonObject.insert(keys::ANNOTATIONS, annotations.value());
		}
		if (jsonObject.size() > 0)
			return jsonObject;
		return {};
	} else if (valueIsClassType) {
		// We have a statically known class type which can be immediately seralized
		if (value.query<raco::core::ArraySemanticAnnotation>()) {
			return serializeArrayProperties(value.getSubstructure(), false);
		} else {
			return serializeObjectProperties(value.getSubstructure(), false);
		}
	} else {
		// we have a primitive value which can be naivly serialized
		return serializePrimitiveValue(value);
	}
}

void createMissingProperties(const std::map<std::string, std::string>& propTypeMap, ReflectionInterface& object, const raco::core::UserObjectFactoryInterface& factory) {
	auto intf = dynamic_cast<proxy::DynamicPropertyInterface*>(&object);

	for (const auto& [propertyName, typeName] : propTypeMap) {
		if (!object.hasProperty(propertyName)) {
			if (raco::data_storage::isPrimitiveTypeName(typeName)) {
				intf->addProperty(propertyName, raco::data_storage::toPrimitiveType(typeName));
			} else {
				// typeName: REF::Material
				intf->addProperty(propertyName, factory.createValue(typeName));
			}
		}
	}
}

void createMissingProperties(const QJsonArray& order, const QJsonObject& jsonObject, raco::data_storage::Table& table, const raco::core::UserObjectFactoryInterface& factory) {
	for (const auto& qPropertyName : order) {
		const std::string propertyName{qPropertyName.toString().toStdString()};
		if (!table.hasProperty(propertyName)) {
			const std::string typeName{jsonObject[qPropertyName.toString()].toObject()[keys::TYPENAME].toString().toStdString()};
			if (raco::data_storage::isPrimitiveTypeName(typeName)) {
				table.addProperty(propertyName, raco::data_storage::toPrimitiveType(typeName));
			} else {
				// typeName: REF::Material
				table.addProperty(propertyName, factory.createValue(typeName));
			}
		}
	}
}

void createMissingProperties(const QJsonArray& jsonArray, raco::data_storage::Table& table, const raco::core::UserObjectFactoryInterface& factory) {
	for (size_t i{0}; i < jsonArray.size(); i++) {
		if (!table[i]) {
			const std::string typeName{jsonArray[static_cast<int>(i)].toObject()[keys::TYPENAME].toString().toStdString()};
			if (raco::data_storage::isPrimitiveTypeName(typeName)) {
				table.addProperty(raco::data_storage::toPrimitiveType(typeName));
			} else {
				table.addProperty(factory.createValue(typeName));
			}
		}
	}
}

/**  Deserializes result of `serializeValueBase` from `property` into the given `value`. */
void deserializeValueBase(const QJsonValue& property, ValueBase& value, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, bool dynamicallyTyped = false) {
	bool childrenDynamicallyTyped{value.type() == PrimitiveType::Table};
	auto valueIsClassType{hasTypeSubstructure(value.type())};
	auto hasAnnotations{property.isObject() && property.toObject().keys().contains(keys::ANNOTATIONS)};

	if (dynamicallyTyped || hasAnnotations || childrenDynamicallyTyped) {
		auto propertyAsObject{property.toObject()};
		if (valueIsClassType) {
			if (propertyAsObject[keys::PROPERTIES].isArray()) {
				if (value.type() == PrimitiveType::Table) {
					createMissingProperties(propertyAsObject[keys::PROPERTIES].toArray(), value.asTable(), factory);
				}
				deserializeArrayProperties(propertyAsObject[keys::PROPERTIES].toArray(), value.getSubstructure(), references, factory, structPropTypesMap, childrenDynamicallyTyped);
			} else {
				if (value.type() == PrimitiveType::Table) {
					createMissingProperties(propertyAsObject[keys::ORDER].toArray(), propertyAsObject[keys::PROPERTIES].toObject(), value.asTable(), factory);
				} else if (value.type() == PrimitiveType::Struct) {
					auto structTypeName = value.getSubstructure().getTypeDescription().typeName;
					createMissingProperties(structPropTypesMap.at(structTypeName), value.getSubstructure(), factory);
				}
				deserializeObjectProperties(propertyAsObject[keys::PROPERTIES].toObject(), value.getSubstructure(), references, factory, structPropTypesMap, childrenDynamicallyTyped);
			}
		} else {
			deserializePrimitiveValue(propertyAsObject[keys::VALUE], value, references);
		}
		if (hasAnnotations) {
			deserializeAnnotations(propertyAsObject[keys::ANNOTATIONS].toArray(), value, references, factory, structPropTypesMap);
		}
	} else if (valueIsClassType) {
		if (property.isArray()) {
			deserializeArrayProperties(property.toArray(), value.getSubstructure(), references, factory, structPropTypesMap, false);
		} else {
			if (value.type() == PrimitiveType::Struct) {
				auto structTypeName = value.getSubstructure().getTypeDescription().typeName;
				createMissingProperties(structPropTypesMap.at(structTypeName), value.getSubstructure(), factory);
			}
			deserializeObjectProperties(property.toObject(), value.getSubstructure(), references, factory, structPropTypesMap, false);
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
std::optional<QJsonArray> serializeArrayProperties(const ReflectionInterface& arrayInterface, bool dynamicallyTyped = false) {
	QJsonArray properties{};
	for (size_t i{0}; i < arrayInterface.size(); i++) {
		const auto& property{arrayInterface.get(i)};
		if (auto child{serializeValueBase(*property, dynamicallyTyped)}) {
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
void deserializeArrayProperties(const QJsonArray& properties, ReflectionInterface& arrayInterface, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, bool dynamicallyTyped = false) {
	for (size_t i{0}; i < properties.size(); i++) {
		deserializeValueBase(properties[static_cast<int>(i)].toObject(), *arrayInterface.get(i), references, factory, structPropTypesMap, dynamicallyTyped);
	}
}

/**
 * Serializes the properties of the given `interface` into a [QJsonObject]. The result is has the form (e.g. Node):
 * { "objectID": "someID", "objectName": "someName", "translation": {...}, ... }
 * @return a QJsonObject which contains all properties which are accessable in the `interface`. Will return an empty optional if serialized object is empty.
 */
std::optional<QJsonObject> serializeObjectProperties(const ReflectionInterface& propertiesInterface, bool dynamicallyTyped = false) {
	QJsonObject properties{};
	for (size_t i{0}; i < propertiesInterface.size(); i++) {
		const auto& property{propertiesInterface.get(i)};
		if (auto serializedValue{serializeValueBase(*property, dynamicallyTyped)}) {
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
void deserializeObjectProperties(const QJsonObject& properties, ReflectionInterface& objectInterface, References& references, const raco::core::UserObjectFactoryInterface& factory, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap, bool dynamicallyTyped = false) {
	for (const auto& qPropertyName : properties.keys()) {
		std::string name = qPropertyName.toStdString();
		ValueBase& value = *objectInterface.get(objectInterface.index(name));
		if (&value != nullptr) {
			deserializeValueBase(properties[qPropertyName], value, references, factory, structPropTypesMap, dynamicallyTyped);
		} else {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "Dropping unsupported or deprecated property {}", name);
		}
	}
}

/**
 * We have some form of C++ Object. Either an EditorObject or an Annotation.
 * @return QJsonObject of form e.g.: { "typeName": "MeshNode", "properties": { ... } }
 */
QJsonObject serializeTypedObject(const ReflectionInterface& object) {
	QJsonObject jsonObject{};
	jsonObject.insert(keys::TYPENAME, object.serializationTypeName().c_str());

	auto cwrm = dynamic_cast<const ClassWithReflectedMembers*>(&object);
	if (cwrm) {
		auto annotations{serializeObjectAnnotations(cwrm)};
		if (annotations) {
			jsonObject.insert(keys::ANNOTATIONS, annotations.value());
		}
	}

	if (auto properties{serializeObjectProperties(object)}) {
		jsonObject.insert(keys::PROPERTIES, properties.value());
	}
	return jsonObject;
}

/**
 * We have some form of C++ Object. Either an EditorObject or an Annotation. E.g.:
 * { "typeName": "MeshNode", "properties": { ... } }
 * @return an Object created by the `factory` for the given `jsonObject`.
 */

SReflectionInterface deserializeTypedObject(const QJsonObject& jsonObject, raco::core::UserObjectFactoryInterface& factory, References& references, const std::map<std::string, std::map<std::string, std::string>>& typesPropTypesMap, const std::map<std::string, std::map<std::string, std::string>>& structPropTypesMap) {
	auto typeName = jsonObject[keys::TYPENAME].toString().toStdString();

	SReflectionInterface object;
	if (typeName == raco::core::Link::typeDescription.typeName) {
		object = std::make_shared<raco::core::Link>();
	} else {
		object = factory.createObject(typeName);
	}

	if (jsonObject.keys().contains(keys::ANNOTATIONS)) {
		deserializeObjectAnnotations(jsonObject[keys::ANNOTATIONS].toArray(),
			std::dynamic_pointer_cast<raco::data_storage::ClassWithReflectedMembers>(object).get(),
			references, factory, structPropTypesMap);
	}

	if (typeName != raco::core::Link::typeDescription.typeName) {
		createMissingProperties(typesPropTypesMap.at(typeName), *object, factory);
	}
	deserializeObjectProperties(jsonObject[keys::PROPERTIES].toObject(), *object, references, factory, structPropTypesMap);
	return object;
}


/**
 * Deserialize and log the values of version arrays that consist of the values [major.minor.patch].
 * This includes Ramses, Logic Engine, and RaCo versions.
 * @return a QJsonArray holding the version array values.
 */
DeserializedVersion deserializeVersionNumber(const QJsonDocument& document, const char* jsonVersionKey, const char* whichVersion) {
	DeserializedVersion version = {ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION};

	if (document[jsonVersionKey].isUndefined()) {
		LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version is not saved in project file", whichVersion);
	} else {
		auto deserializedVersionNums = document[jsonVersionKey].toArray();
		if (deserializedVersionNums.size() < 3) {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version has not been saved correctly in project file - not enough version values", whichVersion);
		} else if (deserializedVersionNums.size() > 3) {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "{} version has not been saved correctly in project file - too many version values", whichVersion);
		}
		
		std::array<int*, 3> versionNums {&version.major, &version.minor, &version.patch};
		for (int i = 0; i < std::min(static_cast<size_t>(deserializedVersionNums.size()), versionNums.size()); ++i) {
			*versionNums[i] = deserializedVersionNums[i].toInt();
		}
	}

	LOG_INFO(raco::log_system::DESERIALIZATION, "{} version from project file is {}.{}.{}", whichVersion, version.major, version.minor, version.patch);
	return version;
};

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


QMap<QString, QVariant> serializeUserTypePropertyMap(const std::map<std::string, std::map<std::string, std::string>>& propTypeMap) {
	QMap<QString, QVariant> typesMap;
	for (const auto& [userTypeName, propMap] : propTypeMap) {
		QMap<QString, QVariant> map;
		for (const auto& [propName, propType] : propMap) {
			map[QString::fromStdString(propName)] = QString::fromStdString(propType);
		}
		typesMap[QString::fromStdString(userTypeName)] = QVariant(map);
	}
	return typesMap;
}

SReflectionInterface deserializeTypedObject(const QJsonObject& jsonObject, raco::core::UserObjectFactoryInterface& factory, References& references) {
	return deserializeTypedObject(jsonObject, factory, references, makeUserTypePropertyMap(), makeStructPropertyMap());
}

using translateRefFunc = std::function<raco::core::SEditorObject(raco::core::SEditorObject)>;

void convertObjectPropertiesIRToUser(const ReflectionInterface& dynObj, ReflectionInterface& userObj, translateRefFunc translateRef,  raco::core::UserObjectFactoryInterface& factory);
void convertTablePropertiesIRToUser(const Table& src, Table& dest, translateRefFunc translateRef, raco::core::UserObjectFactoryInterface& factory);

void convertPropertyAnnotationsIRToUser(const ValueBase& dynProp, ValueBase& userProp, translateRefFunc translateRef, raco::core::UserObjectFactoryInterface& factory, bool dynamicallyTyped) {
	for (auto srcAnno : dynProp.baseAnnotationPtrs()) {
		auto it = std::find_if(userProp.baseAnnotationPtrs().begin(), userProp.baseAnnotationPtrs().end(), [srcAnno](const raco::data_storage::AnnotationBase* destAnno) {
			return destAnno->getTypeDescription().typeName == srcAnno->getTypeDescription().typeName;
		});
		if (it != userProp.baseAnnotationPtrs().end() &&
			(dynamicallyTyped || srcAnno->serializationRequired())) {
			convertObjectPropertiesIRToUser(*srcAnno, **it, translateRef, factory);
		}
	}
}

void convertValueBaseIRToUser(const ValueBase& dynProp, ValueBase& userProp, translateRefFunc translateRef, raco::core::UserObjectFactoryInterface& factory, bool dynamicallyTyped = false) {
	if (userProp.type() == PrimitiveType::Table) {
		convertTablePropertiesIRToUser(dynProp.asTable(), userProp.asTable(), translateRef, factory);
	} else if (hasTypeSubstructure(userProp.type())) {
		convertObjectPropertiesIRToUser(dynProp.getSubstructure(), userProp.getSubstructure(), translateRef, factory);
	} else if (userProp.type() == PrimitiveType::Ref) {
		userProp = translateRef(dynProp.asRef());
	} else {
		// simple primitive type
		userProp = dynProp;
	}
	convertPropertyAnnotationsIRToUser(dynProp, userProp, translateRef, factory, dynamicallyTyped);
}

void convertTablePropertiesIRToUser(const Table& src, Table& dest, translateRefFunc translateRef, raco::core::UserObjectFactoryInterface& factory) {
	for (size_t i = 0; i < src.size(); i++) {
		auto propName = src.name(i);
		auto typeName = src.get(i)->typeName();
		if (raco::data_storage::isPrimitiveTypeName(typeName)) {
			dest.addProperty(propName, raco::data_storage::toPrimitiveType(typeName));
		} else {
			dest.addProperty(propName, factory.createValue(typeName));
		}
		convertValueBaseIRToUser(*src.get(i), *dest.get(i), translateRef, factory, true);
	}
}

void convertObjectPropertiesIRToUser(const ReflectionInterface& dynObj, ReflectionInterface& userObj, translateRefFunc translateRef,  raco::core::UserObjectFactoryInterface& factory) {
	for (size_t i = 0; i < dynObj.size(); i++) {
		auto propName = dynObj.name(i);
		convertValueBaseIRToUser(*dynObj.get(i), *userObj.get(propName), translateRef, factory);
	}
}

void convertObjectAnnotationsIRToUser(const ClassWithReflectedMembers& dynObj, ClassWithReflectedMembers& userObj, translateRefFunc translateRef, 
	raco::core::UserObjectFactoryInterface& factory) {
	for (auto anno : dynObj.annotations()) {
		auto userAnno = factory.createAnnotation(anno->getTypeDescription().typeName);
		userObj.addAnnotation(userAnno);

		convertObjectPropertiesIRToUser(*anno, *userAnno, translateRef, factory);
	}
}

ProjectDeserializationInfo ConvertFromIRToUserTypes(const ProjectDeserializationInfoIR& deserializedIR) {
	auto& userFactory{raco::user_types::UserObjectFactory::getInstance()};

	// dynamic -> static object translationB
	ProjectDeserializationInfo result;
	result.versionInfo = deserializedIR.versionInfo;

	result.externalProjectsMap = deserializedIR.externalProjectsMap;

	std::map<std::string, raco::core::SEditorObject> instanceMap;
	for (const auto& obj : deserializedIR.objects) {
		auto dynObj = std::dynamic_pointer_cast<raco::serialization::proxy::DynamicEditorObject>(obj);

		auto objectID = *dynObj->objectID_;
		auto typeName = dynObj->getTypeDescription().typeName;

		auto userObj = std::dynamic_pointer_cast<EditorObject>(userFactory.createObject(typeName));
		result.objects.emplace_back(userObj);

		instanceMap[objectID] = userObj;
	}

	auto translateRef = [&instanceMap](SEditorObject obj) -> SEditorObject {
		if (obj) {
			return instanceMap.at(obj->objectID());
		}
		return nullptr;
	};

	for (const auto& irLink : deserializedIR.links) {
		result.links.emplace_back(raco::core::Link::cloneLinkWithTranslation(std::dynamic_pointer_cast<raco::core::Link>(irLink), translateRef));
	}

	for (const auto& obj : deserializedIR.objects) {
		auto dynObj = std::dynamic_pointer_cast<raco::serialization::proxy::DynamicEditorObject>(obj);
		auto userObj = instanceMap[dynObj->objectID()];

		convertObjectPropertiesIRToUser(*dynObj, *userObj, translateRef, userFactory);
		convertObjectAnnotationsIRToUser(*dynObj, *userObj, translateRef, userFactory);
	}

	return result;
}

}  // namespace


namespace raco::serialization {

std::string test_helpers::serializeObject(const SReflectionInterface& object, const std::string& originPath) {
	return QJsonDocument{serializeTypedObject(*object.get())}.toJson().toStdString();
}

std::string ObjectsDeserialization::originPath() const {
	return (std::filesystem::path(originFolder) / originFileName).generic_string();
}

ObjectDeserialization test_helpers::deserializeObject(const std::string& json) {
	auto& factory{user_types::UserObjectFactory::getInstance()};
	References references{};
	return {
		std::dynamic_pointer_cast<EditorObject>(deserializeTypedObject(QJsonDocument::fromJson(json.c_str()).object(), factory, references)),
		references};
}

std::map<std::string, std::map<std::string, std::string>> makeUserTypePropertyMap() {
	auto& userFactory{user_types::UserObjectFactory::getInstance()};

	std::map<std::string, std::map<std::string, std::string>> typesPropTypesMap;

	for (const auto& [name, desc] : userFactory.getTypes()) {
		auto obj = userFactory.createObject(name);

		std::map<std::string, std::string> propTypeMap;
		for (size_t i = 0; i < obj->size(); i++) {
			auto propName = obj->name(i);
			auto propType = obj->get(i)->typeName();
			propTypeMap[propName] = propType;
		}
		typesPropTypesMap[name] = propTypeMap;
	}

	return typesPropTypesMap;
}

std::map<std::string, std::map<std::string, std::string>> makeStructPropertyMap() {
	auto& userFactory{raco::user_types::UserObjectFactory::getInstance()};

	std::map<std::string, std::map<std::string, std::string>> structPropTypesMap;

	for (const auto& [name, desc] : userFactory.getStructTypes()) {
		auto obj = userFactory.createStruct(name);

		std::map<std::string, std::string> propTypeMap;
		for (size_t i = 0; i < obj->size(); i++) {
			auto propName = obj->name(i);
			auto propType = obj->get(i)->typeName();
			propTypeMap[propName] = propType;
		}
		structPropTypesMap[name] = propTypeMap;
	}

	return structPropTypesMap;
}

std::map<std::string, std::map<std::string, std::string>> deserializeUserTypePropertyMap(const QVariant& container) {
	std::map<std::string, std::map<std::string, std::string>> typesPropTypeMap;

	for (auto [name, propMap] : container.toMap().toStdMap()) {
		auto qPropMap = propMap.toMap();
		std::map<std::string, std::string> propTypeMap;
		for (auto& [propName, propType] : qPropMap.toStdMap()) {
			propTypeMap[propName.toStdString()] = propType.toString().toStdString();
		}
		typesPropTypeMap[name.toStdString()] = propTypeMap;
	}
	return typesPropTypeMap;
}

std::string serializeObjects(const std::vector<raco::core::SEditorObject>& objects, const std::vector<std::string>& rootObjectIDs, const std::vector<raco::core::SLink>& links, const std::string& originFolder, const std::string& originFilename, const std::string& originProjectID, const std::string& originProjectName, const std::map<std::string, ExternalProjectInfo>& externalProjectsMap, const std::map<std::string, std::string>& originFolders) {
	QJsonObject result{};

	result.insert(keys::FILE_VERSION, RAMSES_PROJECT_FILE_VERSION);
	result.insert(keys::RAMSES_COMPOSER_VERSION, QJsonArray{RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH});

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
		jsonObjects.push_back(serializeTypedObject(*object.get()));
	}
	result.insert(keys::OBJECTS, jsonObjects);

	QJsonArray jsonLinks{};
	for (const auto& link : links) {
		jsonLinks.push_back(serializeTypedObject(*link.get()));
	}
	result.insert(keys::LINKS, jsonLinks);

	return QJsonDocument{result}.toJson().toStdString();
}

std::optional<ObjectsDeserialization> deserializeObjects(const std::string& json) {
	auto& factory{user_types::UserObjectFactory::getInstance()};
	ObjectsDeserialization result{};
	auto document{QJsonDocument::fromJson(json.c_str())};
	if (document.isNull()) {
		return {};
	}

	auto container{document.object()};
	if (container[keys::FILE_VERSION].isUndefined() || container[keys::FILE_VERSION].toInt() != RAMSES_PROJECT_FILE_VERSION) {
		return {};
	}
	if (container[keys::RAMSES_COMPOSER_VERSION].isUndefined() || container[keys::RAMSES_COMPOSER_VERSION].toArray() != QJsonArray{RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH}) {
		return {};
	}

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
		auto deserializedObject = std::dynamic_pointer_cast<EditorObject>(deserializeTypedObject(objJson.toObject(), factory, result.references));
		result.objects.push_back(deserializedObject);
	}
	for (const auto& linkJson : container.value(keys::LINKS).toArray()) {
		auto deserializedLink = std::dynamic_pointer_cast<raco::core::Link>(deserializeTypedObject(linkJson.toObject(), factory, result.references));
		result.links.push_back(deserializedLink);
	}
	return result;
}

QJsonDocument serializeProject(const std::unordered_map<std::string, std::vector<int>>& fileVersions, const std::vector<SReflectionInterface>& instances, const std::vector<SReflectionInterface>& links,
	const std::map<std::string, ExternalProjectInfo>& externalProjectsMap) {
	QJsonObject container{};

	auto ramsesVer = fileVersions.at(keys::RAMSES_VERSION);
	auto logicEngineVer = fileVersions.at(keys::RAMSES_LOGIC_ENGINE_VERSION);
	auto raCoVersion = fileVersions.at(keys::RAMSES_COMPOSER_VERSION);
	container.insert(keys::FILE_VERSION, fileVersions.at(keys::FILE_VERSION)[0]);
	container.insert(keys::RAMSES_VERSION, QJsonArray{ramsesVer[0], ramsesVer[1], ramsesVer[2]});
	container.insert(keys::RAMSES_LOGIC_ENGINE_VERSION, QJsonArray{logicEngineVer[0], logicEngineVer[1], logicEngineVer[2]});
	container.insert(keys::RAMSES_COMPOSER_VERSION, QJsonArray{raCoVersion[0], raCoVersion[1], raCoVersion[2]});

	serializeExternalProjectsMap(container, externalProjectsMap);

	container.insert(keys::USER_TYPE_PROP_MAP, QJsonValue::fromVariant(serializeUserTypePropertyMap(makeUserTypePropertyMap())));
	container.insert(keys::STRUCT_PROP_MAP, QJsonValue::fromVariant(serializeUserTypePropertyMap(makeStructPropertyMap())));

	QJsonArray objectArray{};
	for (const auto& object : instances) {
		objectArray.push_back(serializeTypedObject(*object.get()));
	}
	container.insert(keys::INSTANCES, objectArray);
	QJsonArray linkArray{};
	for (const auto& link : links) {
		linkArray.push_back(serializeTypedObject(*link.get()));
	}
	container.insert(keys::LINKS, linkArray);
	return QJsonDocument{container};
}

int deserializeFileVersion(const QJsonDocument& document) {
	return document.object()[keys::FILE_VERSION].toInt();
}

ProjectVersionInfo deserializeProjectVersionInfo(const QJsonDocument& document) {
	ProjectVersionInfo deserializedProjectInfo;

	deserializedProjectInfo.ramsesVersion = deserializeVersionNumber(document, keys::RAMSES_VERSION, "Ramses");
	deserializedProjectInfo.ramsesLogicEngineVersion = deserializeVersionNumber(document, keys::RAMSES_LOGIC_ENGINE_VERSION, "Ramses Logic Engine");
	deserializedProjectInfo.raCoVersion = deserializeVersionNumber(document, keys::RAMSES_COMPOSER_VERSION, "Ramses Composer");

	return deserializedProjectInfo;
}

ProjectDeserializationInfoIR deserializeProjectToIR(const QJsonDocument& document, const std::string& filename) {
	auto migratedJson{raco::serializationToV23::migrateProjectToV23(document)};

	auto& factory{raco::serialization::proxy::ProxyObjectFactory::getInstance()};

	ProjectDeserializationInfoIR deserializedProjectInfo;

	deserializedProjectInfo.versionInfo = deserializeProjectVersionInfo(migratedJson);
	deserializedProjectInfo.fileVersion = migratedJson.object()[keys::FILE_VERSION].toInt();

	deserializeExternalProjectsMap(migratedJson[keys::EXTERNAL_PROJECTS].toVariant(), deserializedProjectInfo.externalProjectsMap);
	auto userPropTypeMap = deserializeUserTypePropertyMap(migratedJson[keys::USER_TYPE_PROP_MAP]);
	auto structTypeMap = deserializeUserTypePropertyMap(migratedJson[keys::STRUCT_PROP_MAP]);

	References references;

	const auto instances = migratedJson[keys::INSTANCES].toArray();
	deserializedProjectInfo.objects.reserve(instances.size());
	for (const auto& instance : instances) {
		auto obj = std::dynamic_pointer_cast<raco::serialization::proxy::DynamicEditorObject>(deserializeTypedObject(instance.toObject(), factory, references, userPropTypeMap, structTypeMap));
		deserializedProjectInfo.objects.push_back(obj);
	}
	const auto links = migratedJson[keys::LINKS].toArray();
	deserializedProjectInfo.links.reserve(links.size());
	for (const auto& linkJson: links) {
		auto link = std::dynamic_pointer_cast<raco::core::Link>(deserializeTypedObject(linkJson.toObject(), factory, references, userPropTypeMap, structTypeMap));
		deserializedProjectInfo.links.push_back(link);
	}

	// Restore references
	std::map<std::string, raco::core::SEditorObject> instanceMap;
	for (auto& d : deserializedProjectInfo.objects) {
		auto obj = std::dynamic_pointer_cast<raco::core::EditorObject>(d);
		instanceMap[obj->objectID()] = obj;
	}
	for (const auto& pair : references) {
		if (instanceMap.find(pair.second) != instanceMap.end()) {
			*pair.first = instanceMap.at(pair.second);
		} else {
			LOG_WARNING(raco::log_system::DESERIALIZATION, "Load: referenced object not found: {}", pair.second);
		}
	}

	for (const auto& obj : deserializedProjectInfo.objects) {
		auto dynObj = std::dynamic_pointer_cast<raco::serialization::proxy::DynamicEditorObject>(obj);
		dynObj->onAfterDeserialization();
	}

	deserializedProjectInfo.currentPath = filename;

	return deserializedProjectInfo;
}

ProjectDeserializationInfo deserializeProject(const QJsonDocument& document, const std::string& filename) {
	auto deserializedIR{deserializeProjectToIR(document, filename)};

	// run new migration code
	migrateProject(deserializedIR);

	return ConvertFromIRToUserTypes(deserializedIR);
}

}  // namespace raco::serialization
