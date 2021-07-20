/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/RamsesProjectMigration.h"

#include "core/Context.h"
#include "core/Link.h"
#include "serialization/Serialization.h"
#include "serialization/SerializationKeys.h"
#include "user_types/UserObjectFactory.h"

#include "user_types/EngineTypeAnnotation.h"
#include "core/EngineInterface.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include <QJsonObject>

// Helper functions for migration - those are only implemented as far as they were needed.
// If you need more functionality, please add it.
namespace {

std::string serializedProjectSetting() {
	return
		R"___({
	"properties" : {
		"objectID" : ")___" +
		QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString() + R"___(",
		"objectName" : "",
		"sceneId" : {
			"annotations" : [
				{
					"properties" : {
						"max" : 1024,
						"min" : 1
					},
					"typeName" : "RangeAnnotationInt"
				}
			],
			"value" : 123
		}
	},
	"typeName" : "ProjectSettings"
})___";
}

// Helper object needed for readprop(...). Note that this clearly does not work for more complicated cases, so far (file version V9->V10) no calls to the deserializationFactory were needed.
// If you need a new object/property during migration, you need to make sure that you have the factory matching the version you are loading - and make sure you know what you are doing.
raco::serialization::DeserializationFactory deserializationFactoryV9() {
	return raco::user_types::UserObjectFactoryInterface::deserializationFactory(nullptr);
}

// Deserialization helper factory used from V10 onwards:
// This factory can create typed reference properties (e.g. texture uniforms) but can't create objects or annotations and 
// should be sufficient to deserialize even complex properties.
raco::serialization::DeserializationFactory deserializationFactoryV10plus() {
	auto objectFactory = &raco::user_types::UserObjectFactory::getInstance();
	return raco::serialization::DeserializationFactory{
		{},
		{},
		[objectFactory](const std::string& type) {
			return objectFactory->createValue(type);
		}};
}

// Iterate over all instances in the JSON document. If the visitor returns true, the changes done to instanceproperties in the visitor will
// be stored in the JSON document.
void iterateInstances(QJsonObject& documentObject, std::function<bool(QString const& instancetype, QJsonObject& instanceproperties)> const& visitor) {
	auto instances = documentObject[raco::serialization::keys::INSTANCES].toArray();
	for (QJsonValueRef instance : instances) {
		auto o = instance.toObject();
		auto t = o[raco::serialization::keys::TYPENAME].toString();
		auto p = o[raco::serialization::keys::PROPERTIES].toObject();
		if (visitor(t, p)) {
			auto y = p.keys();
			o[raco::serialization::keys::PROPERTIES] = p;
			instance = o;
			auto x = o[raco::serialization::keys::PROPERTIES].toObject().keys();
		}
	}
	documentObject[raco::serialization::keys::INSTANCES] = instances;
}

// Read a property from a JSON block for the properties of a RaCo object. The passed in property parameter must have the exact
// type the property had when the migrated file version was saved, e. g. "data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> property;"
// Once the function returns, the value of the property can be extracted using property.as....().
// This function requires deserializePropertyForMigration to be able to read the migrated file version (so if the code in deserializePropertyForMigration
// changes in a way that this is no longer possible, older file versions need to be migrated from with the original function).
template <typename PropertyType>
raco::serialization::References readprop(raco::serialization::DeserializationFactory& deserializationFactory, QJsonObject const& instanceproperties, QStringView propname, PropertyType& property) {
	auto const jsonprop = instanceproperties[propname];
	assert(!jsonprop.isUndefined());
	return raco::serialization::deserializePropertyForMigration(jsonprop, property, deserializationFactory);
}

// Add a property to a JSON block for the properties of a RaCo object. The passed in property parameter must have the exact
// type the property had in the class for the target file version, e. g. "data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> property;"
// This function requires serializePropertyForMigration to serialize the block consistent with the target file version (so if the code in serializePropertyForMigration
// changes in a way that this is no longer possible, older target file versions need to be migrated to with the original function).
template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringView propname, PropertyType const& property, raco::serialization::References& references) {
	auto v = raco::serialization::serializePropertyForMigration(
		property,
		[&references](const raco::data_storage::ValueBase& value) -> std::optional<std::string> {
			raco::data_storage::ValueBase* ptr = const_cast<raco::data_storage::ValueBase*>(&value);
			auto it = references.find(ptr);
			if (it != references.end()) {
				return it->second;
			}
			return {};
		});
	assert(v.has_value());
	instanceproperties[propname] = *v;
}
template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringView propname, PropertyType const& property) {
	raco::serialization::References references;
	addprop<PropertyType>(instanceproperties, propname, property, references);
}


// Remove a property from a JSON block for the properties of a RaCo object.
void removeprop(QJsonObject& instanceproperties, QStringView propname) {
	instanceproperties.remove(propname);
}

}  // namespace

namespace raco::components {

QJsonDocument migrateProject(const QJsonDocument& document) {
	using namespace data_storage;

	int const documentVersion = raco::serialization::deserializeFileVersion(document);

	QJsonObject documentObject{document.object()};
	if (documentVersion < 2) {
		auto instances = documentObject[raco::serialization::keys::INSTANCES].toArray();
		instances.push_back(QJsonDocument::fromJson(serializedProjectSetting().c_str()).object());
		documentObject[raco::serialization::keys::INSTANCES] = instances;
	}

	// File Version 3..9: no migration code needed

	// File Version 10: cameras store viewport as four individual integers instead of a vec4i (for camera bindings).
	if (documentVersion < 10) {
		auto factory = deserializationFactoryV9();
		iterateInstances(documentObject, [&factory](QString const& instancetype, QJsonObject& instanceproperties) {
			if (instancetype != "PerspectiveCamera" && instancetype != "OrthographicCamera") {
				return false;
			}
			data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> oldviewportprop;
			readprop(factory, instanceproperties, u"viewport", oldviewportprop);
			removeprop(instanceproperties, u"viewport");

			addprop(instanceproperties, u"viewPortOffsetX", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i1_.asInt(), {"Viewport Offset X"}, {}});
			addprop(instanceproperties, u"viewPortOffsetY", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i2_.asInt(), {"Viewport Offset Y"}, {}});
			addprop(instanceproperties, u"viewPortWidth", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i3_.asInt(), {"Viewport Width"}, {}});
			addprop(instanceproperties, u"viewPortHeight", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i4_.asInt(), {"Viewport Height"}, {}});
			return true;
		});
	}

	// File version 12:
	// Add 'private' property to material slot containers in MeshNodes.
	// Rename 'depthfunction' ->  'depthFunction' in options container of meshnode material slot.
	// Add LinkEndAnnotation to material uniform properties
	if (documentVersion < 12) {
		auto factory = deserializationFactoryV10plus();
		iterateInstances(documentObject, [&factory](const QString& instanceType, QJsonObject& instanceproperties) {
			if (instanceType == "MeshNode" && instanceproperties.contains(u"materials")) {
				Property<Table, DisplayNameAnnotation> materials;

				auto references = readprop(factory, instanceproperties, u"materials", materials);
				removeprop(instanceproperties, u"materials");

				for (size_t i = 0; i < materials->size(); i++) {
					Table& matCont = materials->get(i)->asTable();
					matCont.addProperty("private", new data_storage::Property<bool, DisplayNameAnnotation>(true, {"Private Material"}), 1);
					Table& optionsCont = matCont.get("options")->asTable();
					optionsCont.renameProperty("depthfunction", "depthFunction");
				}

				addprop(instanceproperties, u"materials", materials, references);

				return true;
			}

			if (instanceType == "Material" && instanceproperties.contains(u"uniforms")) {
				Property<Table, DisplayNameAnnotation> uniforms;
				
				auto references = readprop(factory, instanceproperties, u"uniforms", uniforms);
				removeprop(instanceproperties, u"uniforms");

				for (size_t i = 0; i < uniforms->size(); i++) {
					auto engineType = uniforms->get(i)->query<raco::user_types::EngineTypeAnnotation>()->type();
					if (raco::core::PropertyInterface::primitiveType(engineType) != PrimitiveType::Ref) {
						auto newValue = raco::user_types::createDynamicProperty<raco::core::LinkEndAnnotation>(engineType);
						*newValue = *uniforms->get(i);
						uniforms->replaceProperty(i, newValue);
					}
				}

				addprop(instanceproperties, u"uniforms", uniforms, references);
				return true;
			}

			return false;
		});
	}

	QJsonDocument newDocument{documentObject};
	// for debugging:
	// auto migratedJSON = QString(newDocument.toJson());
	return newDocument;
}

}  // namespace raco::components