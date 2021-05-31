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
void readprop(QJsonObject const& instanceproperties, QStringView propname, PropertyType& property) {
	auto const jsonprop = instanceproperties[propname];
	assert(!jsonprop.isUndefined());
	raco::serialization::deserializePropertyForMigration(jsonprop, property, deserializationFactoryV9());
}

// Add a property to a JSON block for the properties of a RaCo object. The passed in property parameter must have the exact
// type the property had in the class for the target file version, e. g. "data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> property;"
// This function requires serializePropertyForMigration to serialize the block consistent with the target file version (so if the code in serializePropertyForMigration
// changes in a way that this is no longer possible, older target file versions need to be migrated to with the original function).
template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringView propname, PropertyType const& property) {
	auto v = raco::serialization::serializePropertyForMigration(property);
	assert(v.has_value());
	instanceproperties[propname] = *v;
}

// Remove a property from a JSON block for the properties of a RaCo object.
void removeprop(QJsonObject& instanceproperties, QStringView propname) {
	instanceproperties.remove(propname);
}

}  // namespace

namespace raco::components {

QJsonDocument migrateProject(const QJsonDocument& document) {
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
		iterateInstances(documentObject, [](QString const& instancetype, QJsonObject& instanceproperties) {
			if (instancetype != "PerspectiveCamera" && instancetype != "OrthographicCamera") {
				return false;
			}
			data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> oldviewportprop;
			readprop(instanceproperties, u"viewport", oldviewportprop);
			removeprop(instanceproperties, u"viewport");

			addprop(instanceproperties, u"viewPortOffsetX", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i1_.asInt(), {"Viewport Offset X"}, {}});
			addprop(instanceproperties, u"viewPortOffsetY", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i2_.asInt(), {"Viewport Offset Y"}, {}});
			addprop(instanceproperties, u"viewPortWidth", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i3_.asInt(), {"Viewport Width"}, {}});
			addprop(instanceproperties, u"viewPortHeight", data_storage::Property<int, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i4_.asInt(), {"Viewport Height"}, {}});
			return true;
		});
	}

	QJsonDocument newDocument{documentObject};
	// for debugging:
	// auto migratedJSON = QString(newDocument.toJson());
	return newDocument;
}

}  // namespace raco::components