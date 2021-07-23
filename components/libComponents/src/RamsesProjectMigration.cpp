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

#include "core/EngineInterface.h"
#include "user_types/EngineTypeAnnotation.h"

#include "user_types/BaseCamera.h"
#include "user_types/DefaultValues.h"
#include "user_types/Material.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
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
		[objectFactory](const std::string& type) -> raco::serialization::SReflectionInterface {
			if (type == raco::user_types::Link::typeDescription.typeName) {
				return std::make_shared<raco::user_types::Link>();
			}
			return {};
		},
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
			o[raco::serialization::keys::PROPERTIES] = p;
			instance = o;
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

template <typename PropertyType>
raco::serialization::References readprop(raco::serialization::DeserializationFactory& deserializationFactory, QJsonObject const& instanceproperties, QStringList propnames, PropertyType& property) {
	QJsonValue v = instanceproperties;
	for (auto index = 0; index < propnames.size(); index++) {
		auto name = propnames[index];
		if (index > 0) {
			v = v[raco::serialization::keys::PROPERTIES];
		}
		v = v[name];
	}
	if (!v.isUndefined()) {
		return raco::serialization::deserializePropertyForMigration(v, property, deserializationFactory);
	}
	return {};
}

std::optional<std::string> resolveRefIdFromReferences(const raco::serialization::References& references, const raco::data_storage::ValueBase& value) {
	raco::data_storage::ValueBase* ptr = const_cast<raco::data_storage::ValueBase*>(&value);
	auto it = references.find(ptr);
	if (it != references.end()) {
		return it->second;
	}
	return {};
}

// Add a property to a JSON block for the properties of a RaCo object. The passed in property parameter must have the exact
// type the property had in the class for the target file version, e. g. "data_storage::Property<data_storage::Vec4i, data_storage::DisplayNameAnnotation> property;"
// This function requires serializePropertyForMigration to serialize the block consistent with the target file version (so if the code in serializePropertyForMigration
// changes in a way that this is no longer possible, older target file versions need to be migrated to with the original function).
template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringView propname, PropertyType const& property, raco::serialization::References& references, bool dynamicallyTyped) {
	auto v = raco::serialization::serializePropertyForMigration(
		property,
		[&references](const raco::data_storage::ValueBase& value) -> std::optional<std::string> {
			return resolveRefIdFromReferences(references, value);
		},
		dynamicallyTyped);
	assert(v.has_value());
	instanceproperties[propname] = *v;
}
template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringView propname, PropertyType const& property, bool dynamicallyTyped = false) {
	raco::serialization::References references;
	addprop<PropertyType>(instanceproperties, propname, property, references, dynamicallyTyped);
}

QStringList intersperse(QStringList sl, QString separator) {
	QStringList result;
	for (auto index = 0; index < sl.size() - 1; index++) {
		result.push_back(sl[index]);
		result.push_back(separator);
	}
	result.push_back(sl.back());
	return result;
}

template <typename PropertyType>
void addprop_rec(QJsonObject& o, QStringList names, PropertyType const& property, bool dynamicallyTyped) {
	if (names.size() == 1) {
		addprop(o, names[0], property, dynamicallyTyped);
	} else {
		auto v = o[names[0]];
		auto vo = v.toObject();
		addprop_rec(vo, QStringList(++names.begin(), names.end()), property, dynamicallyTyped);
		v = vo;
	}
}

template <typename PropertyType>
void addprop(QJsonObject& instanceproperties, QStringList propnames, PropertyType const& property, bool dynamicallyTyped) {
	addprop_rec(instanceproperties, intersperse(propnames, raco::serialization::keys::PROPERTIES), property, dynamicallyTyped);
}

// Remove a property from a JSON block for the properties of a RaCo object.
void removeprop(QJsonObject& instanceproperties, QStringView propname) {
	instanceproperties.remove(propname);
}

void removeprop_rec(QJsonObject& o, QStringList names) {
	if (names.size() == 1) {
		o.remove(names[0]);
	} else {
		auto v = o[names[0]];
		auto vo = v.toObject();
		removeprop_rec(vo, QStringList(++names.begin(), names.end()));
		v = vo;
	}
}

void removeprop(QJsonObject& instanceproperties, QStringList propnames) {
	removeprop_rec(instanceproperties, intersperse(propnames, raco::serialization::keys::PROPERTIES));
}

template <typename PropertyType>
raco::serialization::References extractprop(raco::serialization::DeserializationFactory& deserializationFactory, QJsonObject& instanceproperties, QStringView propname, PropertyType& property) {
	if (instanceproperties.contains(propname)) {
		auto refs = readprop(deserializationFactory, instanceproperties, propname, property);
		removeprop(instanceproperties, propname);
		return refs;
	}
	return {};
}

void linkEndPropPushFrontIfMatching(raco::core::SLink& link, const std::string& matching_prop, const std::string& prepend_prop) {
	if (link->compareEndPropertyNames({matching_prop})) {
		link->endProp_->set<std::string>({prepend_prop, matching_prop});
	}
};

void linkReplaceEndIfMatching(raco::core::SLink& link, const std::string& oldProp, const std::vector<std::string>& newEndProp) {
	if (link->compareEndPropertyNames({oldProp})) {
		link->endProp_->set<std::string>(newEndProp);
	}
};

}  // namespace

namespace raco::components {

QJsonDocument migrateProject(const QJsonDocument& document) {
	using namespace data_storage;
	using namespace user_types;

	using LinkEndAnnotation = core::LinkEndAnnotation;

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

			addprop(instanceproperties, u"viewPortOffsetX", data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i1_.asInt(), {-7680, 7680}, {"Viewport Offset X"}, {}});
			addprop(instanceproperties, u"viewPortOffsetY", data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i2_.asInt(), {-7680, 7680}, {"Viewport Offset Y"}, {}});
			addprop(instanceproperties, u"viewPortWidth", data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i3_.asInt(), {0, 7680}, {"Viewport Width"}, {}});
			addprop(instanceproperties, u"viewPortHeight", data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i4_.asInt(), {0, 7680}, {"Viewport Height"}, {}});
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

				addprop(instanceproperties, u"materials", materials, references, false);

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

				addprop(instanceproperties, u"uniforms", uniforms, references, false);
				return true;
			}

			return false;
		});
	}

	// File version 13: introduction of struct properties for camera viewport, frustum, and material/meshnode blend options
	if (documentVersion < 13) {
		auto factory = deserializationFactoryV10plus();
		iterateInstances(documentObject, [&factory](const QString& instanceType, QJsonObject& instanceproperties) {
			bool changed = false;
			if (instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera") {
				Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> offsetX{0, {-7680, 7680}, {"Viewport Offset X"}, {}};
				extractprop(factory, instanceproperties, u"viewPortOffsetX", offsetX);

				Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> offsetY{0, {-7680, 7680}, {"Viewport Offset Y"}, {}};
				extractprop(factory, instanceproperties, u"viewPortOffsetY", offsetY);

				Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> width{1440, {0, 7680}, {"Viewport Width"}, {}};
				extractprop(factory, instanceproperties, u"viewPortWidth", width);

				Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> height{720, {0, 7680}, {"Viewport Height"}, {}};
				extractprop(factory, instanceproperties, u"viewPortHeight", height);

				Property<user_types::CameraViewport, DisplayNameAnnotation, LinkEndAnnotation> viewport{{}, {"Viewport"}, {}};
				viewport->offsetX_.assign(offsetX, true);
				viewport->offsetY_.assign(offsetY, true);
				viewport->width_.assign(width, true);
				viewport->height_.assign(height, true);

				addprop(instanceproperties, u"viewport", viewport);

				changed = true;
			}

			if (instanceType == "PerspectiveCamera") {
				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> near{0.1, DisplayNameAnnotation("Near Plane"), RangeAnnotation<double>(0.1, 1.0), {}};
				extractprop(factory, instanceproperties, u"near", near);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> far{1000.0, DisplayNameAnnotation("Far Plane"), RangeAnnotation<double>(100.0, 10000.0), {}};
				extractprop(factory, instanceproperties, u"far", far);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> fov{35.0, DisplayNameAnnotation("Field of View"), RangeAnnotation<double>(10.0, 120.0), {}};
				extractprop(factory, instanceproperties, u"fov", fov);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> aspect{1440.0 / 720.0, DisplayNameAnnotation("Aspect"), RangeAnnotation<double>(0.5, 4.0), {}};
				extractprop(factory, instanceproperties, u"aspect", aspect);

				Property<user_types::PerspectiveFrustum, DisplayNameAnnotation, LinkEndAnnotation> frustum{{}, {"Frustum"}, {}};
				frustum->near_.assign(near, true);
				frustum->far_.assign(far, true);
				frustum->fov_.assign(fov, true);
				frustum->aspect_.assign(aspect, true);

				addprop(instanceproperties, u"frustum", frustum);

				changed = true;
			}

			if (instanceType == "OrthographicCamera") {
				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> near{0.1, DisplayNameAnnotation("Near Plane"), RangeAnnotation<double>(0.1, 1.0), {}};
				extractprop(factory, instanceproperties, u"near", near);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> far{1000.0, DisplayNameAnnotation("Far Plane"), RangeAnnotation<double>(100.0, 10000.0), {}};
				extractprop(factory, instanceproperties, u"far", far);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> left{-10.0, DisplayNameAnnotation("Left Plane"), RangeAnnotation<double>(-1000.0, 0.0), {}};
				extractprop(factory, instanceproperties, u"left", left);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> right{10.0, DisplayNameAnnotation("Right Plane"), RangeAnnotation<double>(0.0, 1000.0), {}};
				extractprop(factory, instanceproperties, u"right", right);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> bottom{-10.0, DisplayNameAnnotation("Bottom Plane"), RangeAnnotation<double>(-1000.0, 0.0), {}};
				extractprop(factory, instanceproperties, u"bottom", bottom);

				Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> top{10.0, DisplayNameAnnotation("Top Plane"), RangeAnnotation<double>(0.0, 1000.0), {}};
				extractprop(factory, instanceproperties, u"top", top);

				Property<user_types::OrthographicFrustum, DisplayNameAnnotation, LinkEndAnnotation> frustum{{}, {"Frustum"}, {}};
				frustum->near_.assign(near, true);
				frustum->far_.assign(far, true);
				frustum->left_.assign(left, true);
				frustum->right_.assign(right, true);
				frustum->bottom_.assign(bottom, true);
				frustum->top_.assign(top, true);

				addprop(instanceproperties, u"frustum", frustum);

				changed = true;
			}

			if (instanceType == "Material") {
				Property<bool, DisplayNameAnnotation> depthwrite{true, DisplayNameAnnotation("Depth Write")};
				extractprop(factory, instanceproperties, u"depthwrite", depthwrite);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> depthFunction{DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION, DisplayNameAnnotation("Depth Function"), EnumerationAnnotation{EngineEnumeration::DepthFunction}};
				extractprop(factory, instanceproperties, u"depthFunction", depthFunction);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> cullmode{DEFAULT_VALUE_MATERIAL_CULL_MODE, DisplayNameAnnotation("Cull Mode"), EnumerationAnnotation{EngineEnumeration::CullMode}};
				extractprop(factory, instanceproperties, u"cullmode", cullmode);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationColor{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EngineEnumeration::BlendOperation}};
				extractprop(factory, instanceproperties, u"blendOperationColor", blendOperationColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationAlpha{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EngineEnumeration::BlendOperation}};
				extractprop(factory, instanceproperties, u"blendOperationAlpha", blendOperationAlpha);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcColor{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EngineEnumeration::BlendFactor}};
				extractprop(factory, instanceproperties, u"blendFactorSrcColor", blendFactorSrcColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestColor{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EngineEnumeration::BlendFactor}};
				extractprop(factory, instanceproperties, u"blendFactorDestColor", blendFactorDestColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcAlpha{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EngineEnumeration::BlendFactor}};
				extractprop(factory, instanceproperties, u"blendFactorSrcAlpha", blendFactorSrcAlpha);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestAlpha{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EngineEnumeration::BlendFactor}};
				extractprop(factory, instanceproperties, u"blendFactorDestAlpha", blendFactorDestAlpha);

				Property<Vec4f, DisplayNameAnnotation> blendColor{{}, {"Blend Color"}};
				extractprop(factory, instanceproperties, u"blendColor", blendColor);

				Property<user_types::BlendOptions, DisplayNameAnnotation> options{{}, {"Options"}};
				options->depthwrite_.assign(depthwrite, true);
				options->depthFunction_.assign(depthFunction, true);
				options->cullmode_.assign(cullmode, true);
				options->blendOperationColor_.assign(blendOperationColor, true);
				options->blendOperationAlpha_.assign(blendOperationAlpha, true);
				options->blendFactorSrcColor_.assign(blendFactorSrcColor, true);
				options->blendFactorDestColor_.assign(blendFactorDestColor, true);
				options->blendFactorSrcAlpha_.assign(blendFactorSrcAlpha, true);
				options->blendFactorDestAlpha_.assign(blendFactorDestAlpha, true);
				options->blendColor_.assign(blendColor, true);

				addprop(instanceproperties, u"options", options);

				changed = true;
			}

			if (instanceType == "MeshNode") {
				Property<bool, DisplayNameAnnotation> depthwrite{true, DisplayNameAnnotation("Depth Write")};
				readprop(factory, instanceproperties, {"materials", "material", "options", "depthwrite"}, depthwrite);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> depthFunction{DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION, DisplayNameAnnotation("Depth Function"), EnumerationAnnotation{EngineEnumeration::DepthFunction}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "depthFunction"}, depthFunction);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> cullmode{DEFAULT_VALUE_MATERIAL_CULL_MODE, DisplayNameAnnotation("Cull Mode"), EnumerationAnnotation{EngineEnumeration::CullMode}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "cullmode"}, cullmode);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationColor{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EngineEnumeration::BlendOperation}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendOperationColor"}, blendOperationColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationAlpha{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EngineEnumeration::BlendOperation}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendOperationAlpha"}, blendOperationAlpha);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcColor{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EngineEnumeration::BlendFactor}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendFactorSrcColor"}, blendFactorSrcColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestColor{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EngineEnumeration::BlendFactor}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendFactorDestColor"}, blendFactorDestColor);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcAlpha{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EngineEnumeration::BlendFactor}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendFactorSrcAlpha"}, blendFactorSrcAlpha);

				Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestAlpha{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EngineEnumeration::BlendFactor}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendFactorDestAlpha"}, blendFactorDestAlpha);

				Property<Vec4f, DisplayNameAnnotation> blendColor{{}, {"Blend Color"}};
				readprop(factory, instanceproperties, {"materials", "material", "options", "blendColor"}, blendColor);

				removeprop(instanceproperties, {"materials", "material", "options"});

				Property<user_types::BlendOptions, DisplayNameAnnotation> options{{}, {"Options"}};
				options->depthwrite_.assign(depthwrite, true);
				options->depthFunction_.assign(depthFunction, true);
				options->cullmode_.assign(cullmode, true);
				options->blendOperationColor_.assign(blendOperationColor, true);
				options->blendOperationAlpha_.assign(blendOperationAlpha, true);
				options->blendFactorSrcColor_.assign(blendFactorSrcColor, true);
				options->blendFactorDestColor_.assign(blendFactorDestColor, true);
				options->blendFactorSrcAlpha_.assign(blendFactorSrcAlpha, true);
				options->blendFactorDestAlpha_.assign(blendFactorDestAlpha, true);
				options->blendColor_.assign(blendColor, true);

				addprop(instanceproperties, {"materials", "material", "options"}, options, true);

				changed = true;
			}

			return changed;
		});

		std::vector<SLink> links;
		auto inJsonLinks = documentObject[raco::serialization::keys::LINKS].toArray();
		raco::serialization::References references;
		for (const auto& linkJson : inJsonLinks) {
			links.push_back(std::dynamic_pointer_cast<Link>(raco::serialization::deserializeTypedObject(linkJson.toObject(), factory, references)));
		}

		for (auto& link : links) {
			// No need to check the object type of the endpoint since the property names alone are unique among top-level properties.
			linkReplaceEndIfMatching(link, "viewPortOffsetX", {"viewport", "offsetX"});
			linkReplaceEndIfMatching(link, "viewPortOffsetY", {"viewport", "offsetY"});
			linkReplaceEndIfMatching(link, "viewPortWidth", {"viewport", "width"});
			linkReplaceEndIfMatching(link, "viewPortHeight", {"viewport", "height"});

			linkReplaceEndIfMatching(link, "near", {"frustum", "nearPlane"});
			linkReplaceEndIfMatching(link, "far", {"frustum", "farPlane"});
			linkReplaceEndIfMatching(link, "fov", {"frustum", "fieldOfView"});
			linkReplaceEndIfMatching(link, "aspect", {"frustum", "aspectRatio"});
			linkReplaceEndIfMatching(link, "left", {"frustum", "leftPlane"}); 
			linkReplaceEndIfMatching(link, "right", {"frustum", "rightPlane"});
			linkReplaceEndIfMatching(link, "bottom", {"frustum", "bottomPlane"});
			linkReplaceEndIfMatching(link, "top", {"frustum", "topPlane"});
		}

		QJsonArray outJsonLinks{};
		for (const auto& link : links) {
			outJsonLinks.push_back(raco::serialization::serializeTypedObject(*link.get(),
				[&references](const raco::data_storage::ValueBase& value) -> std::optional<std::string> {
					return resolveRefIdFromReferences(references, value);
				}));
		}

		documentObject[raco::serialization::keys::LINKS] = outJsonLinks;
	}

	QJsonDocument newDocument{documentObject};
	// for debugging:
	//auto migratedJSON = QString(newDocument.toJson()).toStdString();
	return newDocument;
}

}  // namespace raco::components