/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/ProjectMigration.h"

#include "core/CoreAnnotations.h"
#include "core/DynamicEditorObject.h"
#include "core/EngineInterface.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Link.h"
#include "core/ProxyObjectFactory.h"
#include "core/ProxyTypes.h"
#include "data_storage/Array.h"

#include "log_system/log.h"

#include "user_types/EngineTypeAnnotation.h"

#include "utils/FileUtils.h"

#include <spdlog/fmt/fmt.h>

#include "core/PathManager.h"
#include <QSettings>

namespace raco::serialization {

void linkReplaceEndIfMatching(core::SLink& link, const std::string& oldProp, const std::vector<std::string>& newEndProp) {
	if (link->compareEndPropertyNames({oldProp})) {
		link->endProp_->set(newEndProp);
	}
};

template <class... Args>
data_storage::ValueBase* createDynamicProperty_V11(core::EnginePrimitive type) {
	using namespace raco::serialization::proxy;
	using namespace raco::data_storage;
	using namespace raco::core;

	switch (type) {
		case EnginePrimitive::Bool:
			return ProxyObjectFactory::staticCreateProperty<bool, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int32:
		case EnginePrimitive::UInt16:
		case EnginePrimitive::UInt32:
			return ProxyObjectFactory::staticCreateProperty<int, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Double:
			return ProxyObjectFactory::staticCreateProperty<double, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::String:
			return ProxyObjectFactory::staticCreateProperty<std::string, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2f:
			return ProxyObjectFactory::staticCreateProperty<Vec2f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3f:
			return ProxyObjectFactory::staticCreateProperty<Vec3f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4f:
			return ProxyObjectFactory::staticCreateProperty<Vec4f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2i:
			return ProxyObjectFactory::staticCreateProperty<Vec2i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3i:
			return ProxyObjectFactory::staticCreateProperty<Vec3i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4i:
			return ProxyObjectFactory::staticCreateProperty<Vec4i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Array:
		case EnginePrimitive::Struct:
			return ProxyObjectFactory::staticCreateProperty<Table, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2D:
			return ProxyObjectFactory::staticCreateProperty<STexture, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSamplerCube:
			return ProxyObjectFactory::staticCreateProperty<SCubeMap, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
	}
	return nullptr;
}

template <class... Args>
data_storage::ValueBase* createDynamicProperty_V36(core::EnginePrimitive type) {
	using namespace raco::serialization::proxy;
	using namespace raco::data_storage;
	using namespace raco::core;

	switch (type) {
		case EnginePrimitive::Bool:
			return ProxyObjectFactory::staticCreateProperty<bool, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int32:
		case EnginePrimitive::UInt16:
		case EnginePrimitive::UInt32:
			return ProxyObjectFactory::staticCreateProperty<int, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int64:
			return ProxyObjectFactory::staticCreateProperty<int64_t, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Double:
			return ProxyObjectFactory::staticCreateProperty<double, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::String:
			return ProxyObjectFactory::staticCreateProperty<std::string, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2f:
			return ProxyObjectFactory::staticCreateProperty<Vec2f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3f:
			return ProxyObjectFactory::staticCreateProperty<Vec3f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4f:
			return ProxyObjectFactory::staticCreateProperty<Vec4f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2i:
			return ProxyObjectFactory::staticCreateProperty<Vec2i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3i:
			return ProxyObjectFactory::staticCreateProperty<Vec3i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4i:
			return ProxyObjectFactory::staticCreateProperty<Vec4i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Array:
		case EnginePrimitive::Struct:
			return ProxyObjectFactory::staticCreateProperty<Table, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2D:
			return ProxyObjectFactory::staticCreateProperty<STextureSampler2DBase, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSamplerCube:
			return ProxyObjectFactory::staticCreateProperty<SCubeMap, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
	}
	return nullptr;
}

template <class... Args>
data_storage::ValueBase* createDynamicProperty_V51(core::EnginePrimitive type) {
	using namespace raco::serialization::proxy;
	using namespace raco::data_storage;
	using namespace raco::core;

	switch (type) {
		case EnginePrimitive::Bool:
			return ProxyObjectFactory::staticCreateProperty<bool, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int32:
		case EnginePrimitive::UInt16:
		case EnginePrimitive::UInt32:
			return ProxyObjectFactory::staticCreateProperty<int, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int64:
			return ProxyObjectFactory::staticCreateProperty<int64_t, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Double:
			return ProxyObjectFactory::staticCreateProperty<double, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::String:
			return ProxyObjectFactory::staticCreateProperty<std::string, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2f:
			return ProxyObjectFactory::staticCreateProperty<Vec2f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3f:
			return ProxyObjectFactory::staticCreateProperty<Vec3f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4f:
			return ProxyObjectFactory::staticCreateProperty<Vec4f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2i:
			return ProxyObjectFactory::staticCreateProperty<Vec2i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3i:
			return ProxyObjectFactory::staticCreateProperty<Vec3i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4i:
			return ProxyObjectFactory::staticCreateProperty<Vec4i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Array:
		case EnginePrimitive::Struct:
			return ProxyObjectFactory::staticCreateProperty<Table, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2D:
			return ProxyObjectFactory::staticCreateProperty<STextureSampler2DBase, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2DMS:
			return ProxyObjectFactory::staticCreateProperty<SRenderBufferMS, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSamplerCube:
			return ProxyObjectFactory::staticCreateProperty<SCubeMap, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::TextureSamplerExternal:
			return ProxyObjectFactory::staticCreateProperty<STextureExternal, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
	}
	return nullptr;
}

data_storage::Table* findItemByValue(data_storage::Table& table, core::SEditorObject obj) {
	for (size_t i{0}; i < table.size(); i++) {
		data_storage::Table& item = table.get(i)->asTable();
		if (item.get(1)->asRef() == obj) {
			return &item;
		}
	}
	return nullptr;
}

void insertPrefabInstancesRecursive(serialization::proxy::SEditorObject inst, std::vector<serialization::proxy::SEditorObject>& sortedInstances) {
	if (std::find(sortedInstances.begin(), sortedInstances.end(), inst) == sortedInstances.end()) {
		if (auto prefab = inst->get("template")->asRef()) {
			for (auto prefabChild : core::TreeIteratorAdaptor(prefab)) {
				if (prefabChild->serializationTypeName() == "PrefabInstance") {
					insertPrefabInstancesRecursive(prefabChild, sortedInstances);
				}
			}
		}
		sortedInstances.emplace_back(inst);
	}
}

std::string generateInterfaceTable(const data_storage::Table& table, int depth, const std::string& prefix = std::string(), const std::string& newLine = ",\n");

std::string generateInterfaceSingleEntry(const data_storage::ValueBase* value, int depth) {
	using namespace raco::core;

	auto anno = value->query<user_types::EngineTypeAnnotation>();
	switch (anno->type()) {
		case EnginePrimitive::Bool:
			return "Type:Bool()";
			break;
		case EnginePrimitive::Int32:
			return "Type:Int32()";
			break;
		case EnginePrimitive::Int64:
			return "Type:Int64()";
			break;
		case EnginePrimitive::Double:
			return "Type:Float()";
			break;
		case EnginePrimitive::String:
			return "Type:String()";
			break;
		case EnginePrimitive::Vec2f:
			return "Type:Vec2f()";
			break;
		case EnginePrimitive::Vec3f:
			return "Type:Vec3f()";
			break;
		case EnginePrimitive::Vec4f:
			return "Type:Vec4f()";
			break;
		case EnginePrimitive::Vec2i:
			return "Type:Vec2i()";
			break;
		case EnginePrimitive::Vec3i:
			return "Type:Vec3i()";
			break;
		case EnginePrimitive::Vec4i:
			return "Type:Vec4i()";
			break;

		case EnginePrimitive::Struct: {
			return "{\n" + generateInterfaceTable(value->asTable(), depth + 1) + "\n" + std::string(4 * depth, ' ') + "}";
			break;
		}

		case EnginePrimitive::Array: {
			auto table = value->asTable();
			auto size = table.size();
			return fmt::format("Type:Array({}, {})", std::to_string(size), generateInterfaceSingleEntry(table.get(0), depth));
			break;
		}
		default:
			return "Invalid";
			break;
	}
}

std::string generateInterfaceTable(const data_storage::Table& table, int depth, const std::string& prefix, const std::string& newLine) {
	std::string result;
	for (size_t i = 0; i < table.size(); i++) {
		result += std::string(4 * depth, ' ') + prefix + table.name(i) + " = " + generateInterfaceSingleEntry(table.get(i), depth);
		if (i < table.size() - 1) {
			result += newLine;
		}
	}
	return result;
}

std::string generateInterfaceScript(serialization::proxy::SEditorObject object) {
	return fmt::format(R"___(function interface(INOUT)
{}
end
)___",
		generateInterfaceTable(object->get("luaInputs")->asTable(), 1, "INOUT.", "\n"));
}

void createInterfaceProperties(const data_storage::Table& scriptTable, data_storage::Table& interfaceTable) {
	for (size_t i = 0; i < scriptTable.size(); i++) {
		auto anno = scriptTable.get(i)->query<user_types::EngineTypeAnnotation>();
		if (core::PropertyInterface::primitiveType(anno->type()) == PrimitiveType::Ref) {
			interfaceTable.addProperty(scriptTable.name(i), createDynamicProperty_V36<>(anno->type()), -1);
		} else {
			interfaceTable.addProperty(scriptTable.name(i), createDynamicProperty_V36<core::LinkStartAnnotation, core::LinkEndAnnotation>(anno->type()), -1);
		}

		if (scriptTable.get(i)->type() == PrimitiveType::Table) {
			createInterfaceProperties(scriptTable.get(i)->asTable(), interfaceTable.get(i)->asTable());
		} else {
			*interfaceTable.get(i) = *scriptTable.get(i);
		}
	}
}

serialization::proxy::SDynamicEditorObject createInterfaceObjectV36(serialization::proxy::ProxyObjectFactory& factory, serialization::ProjectDeserializationInfoIR& deserializedIR, std::vector<core::SLink>& createdLinks, serialization::proxy::SDynamicEditorObject& script, std::string& interfaceObjID, const utils::u8path& intfRelPath, const core::SEditorObject& parentObj) {
	using namespace raco::serialization::proxy;

	auto interfaceObj = std::dynamic_pointer_cast<DynamicEditorObject>(factory.createObject("LuaInterface", script->objectName(), interfaceObjID));
	interfaceObj->addProperty("uri", new Property<std::string, URIAnnotation, DisplayNameAnnotation>{intfRelPath.string(), {"Lua interface files(*.lua)"}, DisplayNameAnnotation("URI")}, -1);

	if (auto anno = script->query<core::ExternalReferenceAnnotation>()) {
		interfaceObj->addAnnotation(std::make_shared<core::ExternalReferenceAnnotation>(*anno->projectID_));
	}

	auto newIntfInputs = interfaceObj->addProperty("luaInputs", new Property<Table, DisplayNameAnnotation, LinkStartAnnotation, LinkEndAnnotation>{{}, DisplayNameAnnotation("Inputs"), {}, {}}, -1);
	createInterfaceProperties(script->get("luaInputs")->asTable(), newIntfInputs->asTable());

	deserializedIR.objects.emplace_back(interfaceObj);

	auto prop = parentObj->children_->addProperty(-1);
	*prop = interfaceObj;

	createdLinks.emplace_back(std::make_shared<core::Link>(
		core::PropertyDescriptor(interfaceObj, {"luaInputs"}),
		core::PropertyDescriptor(script, {"luaInputs"}),
		true));

	{
		auto it = deserializedIR.links.begin();
		while (it != deserializedIR.links.end()) {
			auto link = *it;
			if (*link->endObject_ == script) {
				if (parentObj->serializationTypeName() == "Prefab") {
					it = deserializedIR.links.erase(it);
				} else {
					link->endObject_ = interfaceObj;
					++it;
				}
			} else {
				++it;
			}
		}
	}

	return interfaceObj;
}

void splitUniformName(std::string uniformName, core::EnginePrimitive componentType,
	std::vector<std::string>& names,
	std::vector<core::EnginePrimitive>& types) {
	auto dotPos = uniformName.find('.');
	auto bracketPos = uniformName.find('[');

	if (dotPos != std::string::npos && dotPos < bracketPos) {
		// struct
		// notation: struct.member
		auto structName = uniformName.substr(0, dotPos);
		std::string memberName = uniformName.substr(dotPos + 1);

		names.emplace_back(structName);
		types.emplace_back(core::EnginePrimitive::Struct);

		splitUniformName(memberName, componentType, names, types);

	} else if (bracketPos != std::string::npos && bracketPos < dotPos) {
		// array of struct
		// notation: array[index].member
		auto closeBracketPos = uniformName.find(']');
		if (closeBracketPos != std::string::npos) {
			auto arrayName = uniformName.substr(0, bracketPos);
			auto indexStr = uniformName.substr(bracketPos + 1, closeBracketPos - bracketPos - 1);
			auto index = stoi(indexStr);
			auto rest = uniformName.substr(closeBracketPos + 2);

			names.emplace_back(arrayName);
			types.emplace_back(core::EnginePrimitive::Array);

			names.emplace_back(std::to_string(index + 1));
			types.emplace_back(core::EnginePrimitive::Struct);

			splitUniformName(rest, componentType, names, types);
		}
	} else {
		// scalar
		names.emplace_back(uniformName);
		types.emplace_back(componentType);
	}
}

void replaceUniforms_V51(core::Table& uniforms) {
	std::vector<std::string> uniformNames;
	for (size_t i = 0; i < uniforms.size(); i++) {
		uniformNames.emplace_back(uniforms.name(i));
	}

	for (auto name : uniformNames) {
		std::vector<std::string> names;
		std::vector<core::EnginePrimitive> types;
		auto type = uniforms.get(name)->query<user_types::EngineTypeAnnotation>()->type();
		splitUniformName(name, type, names, types);

		if (names.size() > 1) {
			core::Table* container = &uniforms;
			for (int depth = 0; depth < names.size(); depth++) {
				core::ValueBase* newProp;
				if (!container->hasProperty(names[depth])) {
					newProp = container->addProperty(names[depth], createDynamicProperty_V51<core::LinkEndAnnotation>(types[depth]), -1);
				} else {
					newProp = container->get(names[depth]);
				}

				if (depth < names.size() - 1) {
					container = &newProp->asTable();
				} else {
					// tranfer value
					auto oldProp = uniforms.get(name);
					*newProp = *oldProp;
				}
			}

			uniforms.removeProperty(name);
		}
	}
}

/**
 * @brief Recreate the EditorObject::parent_ and referencesToThis_ pointers in all objects
 *
 * Use this after changing the pointer structure of the deserialized objects, i.e. when
 * changing any reference properties.
 */
void recreateBackPointers(serialization::ProjectDeserializationInfoIR& deserializedIR) {
	for (const auto& obj : deserializedIR.objects) {
		obj->resetBackPointers();
	}
	for (const auto& obj : deserializedIR.objects) {
		obj->onAfterDeserialization();
	}
}


// Limitations
// - Annotations and links are handled as static classes:
//   we can't change the class definition in a way that prevents deserialization of the old annotation: this means that
//   the annotation name, the types and names of existing properties, and the serializationRequired flag must not change.
//   the only allowed change is the addition of a new property to the annotation. in this case the migration code must
//   find the existing annotations and fill the new property with the correct value.
// - If the need arises to migrate the annotationns we would need to create proxy types for them in the same
//   way currently done for the Struct PrimitiveTypes.

void migrateProject(ProjectDeserializationInfoIR& deserializedIR, serialization::proxy::ProxyObjectFactory& factory) {
	using namespace raco::data_storage;
	using namespace raco::serialization::proxy;

	if (deserializedIR.fileVersion < 2) {
		auto settingsID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto settings = std::make_shared<serialization::proxy::ProjectSettings>("", settingsID);
		deserializedIR.objects.emplace_back(settings);
	}

	// File Version 10: cameras store viewport as four individual integers instead of a vec4i (for camera bindings).
	if (deserializedIR.fileVersion < 10) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera") {
				auto& oldviewportprop = *dynObj->get("viewport");
				dynObj->addProperty("viewPortOffsetX", new data_storage::Property<int, RangeAnnotation<int>, DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asStruct().get("i1")->asInt(), {-7680, 7680}, {"Viewport Offset X"}, {}}, -1);
				dynObj->addProperty("viewPortOffsetY", new data_storage::Property<int, RangeAnnotation<int>, DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asStruct().get("i2")->asInt(), {-7680, 7680}, {"Viewport Offset Y"}, {}}, -1);
				dynObj->addProperty("viewPortWidth", new data_storage::Property<int, RangeAnnotation<int>, DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asStruct().get("i3")->asInt(), {0, 7680}, {"Viewport Width"}, {}}, -1);
				dynObj->addProperty("viewPortHeight", new data_storage::Property<int, RangeAnnotation<int>, DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asStruct().get("i4")->asInt(), {0, 7680}, {"Viewport Height"}, {}}, -1);
				dynObj->removeProperty("viewport");
			}
		}
	}

	// File Version 11: Added the viewport background color to the ProjectSettings.
	if (deserializedIR.fileVersion < 11) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "ProjectSettings") {
				dynObj->addProperty("backgroundColor", new data_storage::Property<Vec3f, DisplayNameAnnotation>{{}, {"Display Background Color"}}, -1);
			}
		}
	}

	// File version 12:
	// Add 'private' property to material slot containers in MeshNodes.
	// Rename 'depthfunction' ->  'depthFunction' in options container of meshnode material slot.
	// Add LinkEndAnnotation to material uniform properties
	if (deserializedIR.fileVersion < 12) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "MeshNode" && dynObj->hasProperty("materials")) {
				auto* materials = &dynObj->get("materials")->asTable();

				for (size_t i = 0; i < materials->size(); i++) {
					Table& matCont = materials->get(i)->asTable();
					matCont.addProperty("private", new data_storage::Property<bool, DisplayNameAnnotation>(true, {"Private Material"}), 1);
					Table& optionsCont = matCont.get("options")->asTable();
					optionsCont.renameProperty("depthfunction", "depthFunction");
				}
			}

			if (instanceType == "Material" && dynObj->hasProperty("uniforms")) {
				auto* uniforms = &dynObj->get("uniforms")->asTable();

				for (size_t i = 0; i < uniforms->size(); i++) {
					auto engineType = uniforms->get(i)->query<user_types::EngineTypeAnnotation>()->type();
					if (core::PropertyInterface::primitiveType(engineType) != PrimitiveType::Ref) {
						auto newValue = createDynamicProperty_V11<core::LinkEndAnnotation>(engineType);
						*newValue = *uniforms->get(i);
						uniforms->replaceProperty(i, newValue);
					}
				}
			}
		}
	}

	// File version 13: introduction of struct properties for camera viewport, frustum, and material/meshnode blend options
	if (deserializedIR.fileVersion < 13) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera") {
				auto viewport = new Property<serialization::proxy::CameraViewport, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Viewport"}, {}};
				(*viewport)->addProperty("offsetX", dynObj->extractProperty("viewPortOffsetX"), -1);
				(*viewport)->addProperty("offsetY", dynObj->extractProperty("viewPortOffsetY"), -1);
				(*viewport)->addProperty("width", dynObj->extractProperty("viewPortWidth"), -1);
				(*viewport)->addProperty("height", dynObj->extractProperty("viewPortHeight"), -1);
				dynObj->addProperty("viewport", viewport, -1);
			}

			if (instanceType == "PerspectiveCamera") {
				auto frustum = new Property<serialization::proxy::PerspectiveFrustum, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Frustum"}, {}};
				(*frustum)->addProperty("nearPlane", dynObj->extractProperty("near"), -1);
				(*frustum)->addProperty("farPlane", dynObj->extractProperty("far"), -1);
				(*frustum)->addProperty("fieldOfView", dynObj->extractProperty("fov"), -1);
				(*frustum)->addProperty("aspectRatio", dynObj->extractProperty("aspect"), -1);
				dynObj->addProperty("frustum", frustum, -1);
			}

			if (instanceType == "OrthographicCamera") {
				auto frustum = new Property<serialization::proxy::OrthographicFrustum, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Frustum"}, {}};
				(*frustum)->addProperty("nearPlane", dynObj->extractProperty("near"), -1);
				(*frustum)->addProperty("farPlane", dynObj->extractProperty("far"), -1);
				(*frustum)->addProperty("leftPlane", dynObj->extractProperty("left"), -1);
				(*frustum)->addProperty("rightPlane", dynObj->extractProperty("right"), -1);
				(*frustum)->addProperty("bottomPlane", dynObj->extractProperty("bottom"), -1);
				(*frustum)->addProperty("topPlane", dynObj->extractProperty("top"), -1);
				dynObj->addProperty("frustum", frustum, -1);
			}

			if (instanceType == "Material") {
				auto options = new Property<serialization::proxy::BlendOptions, DisplayNameAnnotation>{{}, {"Options"}};
				(*options)->addProperty("blendOperationColor", dynObj->extractProperty("blendOperationColor"), -1);
				(*options)->addProperty("blendOperationAlpha", dynObj->extractProperty("blendOperationAlpha"), -1);
				(*options)->addProperty("blendFactorSrcColor", dynObj->extractProperty("blendFactorSrcColor"), -1);
				(*options)->addProperty("blendFactorDestColor", dynObj->extractProperty("blendFactorDestColor"), -1);
				(*options)->addProperty("blendFactorSrcAlpha", dynObj->extractProperty("blendFactorSrcAlpha"), -1);
				(*options)->addProperty("blendFactorDestAlpha", dynObj->extractProperty("blendFactorDestAlpha"), -1);
				(*options)->addProperty("blendColor", dynObj->extractProperty("blendColor"), -1);
				(*options)->addProperty("depthwrite", dynObj->extractProperty("depthwrite"), -1);
				(*options)->addProperty("depthFunction", dynObj->extractProperty("depthFunction"), -1);
				(*options)->addProperty("cullmode", dynObj->extractProperty("cullmode"), -1);
				dynObj->addProperty("options", options, -1);
			}

			if (instanceType == "MeshNode" && dynObj->hasProperty("materials")) {
				auto& materials = dynObj->get("materials")->asTable();

				for (size_t i = 0; i < materials.size(); i++) {
					Table& matCont = materials.get(i)->asTable();
					Table& optionsCont = matCont.get("options")->asTable();

					auto options = new Property<serialization::proxy::BlendOptions, DisplayNameAnnotation>{{}, {"Options"}};
					(*options)->addProperty("blendOperationColor", optionsCont.get("blendOperationColor")->clone({}), -1);
					(*options)->addProperty("blendOperationAlpha", optionsCont.get("blendOperationAlpha")->clone({}), -1);
					(*options)->addProperty("blendFactorSrcColor", optionsCont.get("blendFactorSrcColor")->clone({}), -1);
					(*options)->addProperty("blendFactorDestColor", optionsCont.get("blendFactorDestColor")->clone({}), -1);
					(*options)->addProperty("blendFactorSrcAlpha", optionsCont.get("blendFactorSrcAlpha")->clone({}), -1);
					(*options)->addProperty("blendFactorDestAlpha", optionsCont.get("blendFactorDestAlpha")->clone({}), -1);
					(*options)->addProperty("blendColor", optionsCont.get("blendColor")->clone({}), -1);
					(*options)->addProperty("depthwrite", optionsCont.get("depthwrite")->clone({}), -1);
					(*options)->addProperty("depthFunction", optionsCont.get("depthFunction")->clone({}), -1);
					(*options)->addProperty("cullmode", optionsCont.get("cullmode")->clone({}), -1);

					matCont.replaceProperty("options", options);
				}
			}
		}

		for (auto& link : deserializedIR.links) {
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
	}

	// 14 : Replaced "U/V Origin" enum with Texture flip flag
	//      Origin "Top Left"->flag enabled
	if (deserializedIR.fileVersion < 14) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "Texture") {
				constexpr int TEXTURE_ORIGIN_BOTTOM(0);
				constexpr int TEXTURE_ORIGIN_TOP(1);
				int oldValue = TEXTURE_ORIGIN_BOTTOM;
				if (dynObj->hasProperty("origin")) {
					oldValue = dynObj->get("origin")->asInt();
					dynObj->removeProperty("origin");
				}

				bool flipTexture = oldValue == TEXTURE_ORIGIN_TOP;
				dynObj->addProperty("flipTexture", new Property<bool, DisplayNameAnnotation>{flipTexture, DisplayNameAnnotation("Flip U/V Origin")}, -1);
			}
		}
	}

	// File version 15: offscreen rendering
	// - changed texture uniform type for normal 2D textures from STexture -> STextureSampler2DBase
	if (deserializedIR.fileVersion < 15) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			auto migrateUniforms = [](Table& uniforms) {
				for (size_t i = 0; i < uniforms.size(); i++) {
					auto engineType = uniforms.get(i)->query<user_types::EngineTypeAnnotation>()->type();
					if (engineType == core::EnginePrimitive::TextureSampler2D) {
						auto newValue = ProxyObjectFactory::staticCreateProperty<STextureSampler2DBase, user_types::EngineTypeAnnotation>({}, {engineType});
						*newValue = uniforms.get(i)->asRef();
						uniforms.replaceProperty(i, newValue);
					}
				}
			};

			if (instanceType == "Material" && dynObj->hasProperty("uniforms")) {
				auto& uniforms = dynObj->get("uniforms")->asTable();
				migrateUniforms(uniforms);
			}

			if (instanceType == "MeshNode" && dynObj->hasProperty("materials")) {
				auto* materials = &dynObj->get("materials")->asTable();

				for (size_t i = 0; i < materials->size(); i++) {
					Table& matCont = materials->get(i)->asTable();
					Table& uniformsCont = matCont.get("uniforms")->asTable();
					migrateUniforms(uniformsCont);
				}
			}
		}

		// create default render setup
		// - tag top-level Nodes with "render_main" tag
		// - create default RenderLayer and RenderPass

		SDynamicEditorObject perspCamera;
		SDynamicEditorObject orthoCamera;

		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "PerspectiveCamera") {
				perspCamera = dynObj;
			}

			if (instanceType == "OrthographicCamera") {
				orthoCamera = dynObj;
			}
		}

		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "Node" || instanceType == "MeshNode" || instanceType == "PrefabInstance") {
				if (!dynObj->getParent()) {
					auto tags = new Property<Table, core::ArraySemanticAnnotation, core::TagContainerAnnotation, DisplayNameAnnotation>{{}, {}, {}, {"Tags"}};
					tags->set(std::vector<std::string>({"render_main"}));
					dynObj->addProperty("tags", tags, -1);
				}
			}
		}

		SDynamicEditorObject camera = perspCamera ? perspCamera : orthoCamera;

		auto layerID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto mainLayer = std::make_shared<serialization::proxy::RenderLayer>("MainRenderLayer", layerID);
		auto renderableTagsProp = new Property<Table, core::RenderableTagContainerAnnotation, DisplayNameAnnotation>{{}, {}, {"Renderable Tags"}};
		(*renderableTagsProp)->addProperty("render_main", std::make_unique<data_storage::Value<int>>(0));
		mainLayer->addProperty("renderableTags", renderableTagsProp, -1);
		mainLayer->addProperty("sortOrder", new Property<int, DisplayNameAnnotation, EnumerationAnnotation>{2, {"Render Order"}, core::EUserTypeEnumerations::RenderLayerOrder}, -1);

		auto passID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto mainPass = std::make_shared<serialization::proxy::RenderPass>("MainRenderPass", passID);

		ValueBase* cameraProp = new Property<serialization::proxy::SBaseCamera, DisplayNameAnnotation>{{}, {"Camera"}};
		if (camera) {
			*cameraProp = camera;
		}
		mainPass->addProperty("camera", cameraProp, -1);

		auto layer0Prop = new Property<SRenderLayer, DisplayNameAnnotation>{{}, {"Layer 0"}};
		*layer0Prop = mainLayer;
		mainPass->addProperty("layer0", layer0Prop, -1);

		deserializedIR.objects.emplace_back(mainLayer);
		deserializedIR.objects.emplace_back(mainPass);
	}

	if (deserializedIR.fileVersion < 16) {
		std::map<std::string, std::array<bool, 2>> objectsWithAffectedProperties;

		for (const auto& link : deserializedIR.links) {
			if (*link->isValid_ == false) {
				continue;
			}

			auto linkEndObjID = (*link->endObject_)->objectID();
			auto linkEndProps = link->endPropertyNamesVector();

			if (linkEndProps.size() == 1) {
				auto endProp = linkEndProps[0];

				objectsWithAffectedProperties[linkEndObjID][0] = objectsWithAffectedProperties[linkEndObjID][0] || endProp == "rotation";
				objectsWithAffectedProperties[linkEndObjID][1] = objectsWithAffectedProperties[linkEndObjID][1] || endProp == "scale";
			}
		}

		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType != "Node" && instanceType != "MeshNode") {
				continue;
			}

			auto& scaleVec = dynObj->get("scale")->asStruct();
			auto& rotationVec = dynObj->get("rotation")->asStruct();
			auto objID = dynObj->objectID();

			constexpr auto EPSILON = 0.0001;

			auto rotationNotZero = rotationVec.get("x")->asDouble() > EPSILON || rotationVec.get("y")->asDouble() > EPSILON || rotationVec.get("z")->asDouble() > EPSILON;
			auto scaleNotUniform = std::abs(scaleVec.get("x")->asDouble() - scaleVec.get("y")->asDouble()) > EPSILON || std::abs(scaleVec.get("x")->asDouble() - scaleVec.get("z")->asDouble()) > EPSILON;
			auto rotationLinked = objectsWithAffectedProperties[objID][0];
			auto scaleLinked = objectsWithAffectedProperties[objID][1];

			std::string warningText;

			if ((rotationNotZero || rotationLinked) && (scaleNotUniform || scaleLinked)) {
				warningText = fmt::format("This node has {} scaling values and {} rotation values{}.",
					scaleLinked ? "linked" : "non-uniform", rotationLinked ? "linked" : "non-zero",
					scaleLinked || rotationLinked ? " which may lead to non-uniform scaling values and non-zero rotation values" : "");
			}

			if (!warningText.empty()) {
				deserializedIR.migrationObjWarnings[objID] =
					fmt::format(
						"{}\n"
						"Due to a new Ramses version that changed transformation order (previously: Translation -> Rotation -> Scale, now: Translation -> Scale -> Rotation), this node may be rendered differently from the previous version."
						"\n\n"
						"This message will disappear after saving & reloading the project or when a new warning/error for this Node pops up.",
						warningText);
			}
		}
	}

	if (deserializedIR.fileVersion < 17) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "RenderLayer" && dynObj->hasProperty("sortOrder")) {
				auto& sortOrder = *dynObj->get("sortOrder");
				switch (sortOrder.asInt()) {
					case 0:
						sortOrder.asInt() = 0;
						break;
					case 1:
						sortOrder.asInt() = 0;
						break;
					case 2:
						sortOrder.asInt() = 1;
						break;
				}
			}
		}
	}

	// File version 19: Changed ProjectSettings::backgroundColor from Vec3f to Vec4f
	if (deserializedIR.fileVersion < 19) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "ProjectSettings") {
				auto bgColor3 = dynObj->extractProperty("backgroundColor");
				auto& bgColor3Vec = bgColor3->asStruct();
				auto bgColor4Vec = new Property<Vec4f, DisplayNameAnnotation>{{}, {"Display Background Color"}};
				if (bgColor3Vec.hasProperty("x")) {
					(*bgColor4Vec)->addProperty("x", bgColor3Vec.get("x")->clone(nullptr), -1);
				}
				if (bgColor3Vec.hasProperty("y")) {
					(*bgColor4Vec)->addProperty("y", bgColor3Vec.get("y")->clone(nullptr), -1);
				}
				if (bgColor3Vec.hasProperty("z")) {
					(*bgColor4Vec)->addProperty("z", bgColor3Vec.get("z")->clone(nullptr), -1);
				}
				(*bgColor4Vec)->addProperty("w", new Property<double, DisplayNameAnnotation, RangeAnnotation<double>>{{1.0}, DisplayNameAnnotation{"W"}, RangeAnnotation<double>(0.0, 1.0)}, -1);
				dynObj->addProperty("backgroundColor", bgColor4Vec, -1);
			}
		}
	}

	// File version 21: Added mipmap flag to textures
	if (deserializedIR.fileVersion < 21) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "Texture") {
				dynObj->addProperty("generateMipmaps", new Property<bool, DisplayNameAnnotation>{false, DisplayNameAnnotation("Generate Mipmaps")}, -1);
			}
		}
	}

	// File version 22: Added support for setting default resource folders per project
	if (deserializedIR.fileVersion < 22) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "ProjectSettings") {
				// The old resource folder settings from the RaCoPreferences are transferred into the ProjectSettings
				// We do not have access to the RaCoPreferences class here, however, we can just parse the ini file directly instead.
				const std::string projectSubdirectoryFilter = "projectSubDir";
				auto settings = core::PathManager::preferenceSettings();
				auto resourceFolders = new Property<serialization::proxy::DefaultResourceDirectories, DisplayNameAnnotation>{{}, {"Default Resource Folders"}};
				(*resourceFolders)->addProperty("imageSubdirectory", new Property<std::string, DisplayNameAnnotation, URIAnnotation>{settings.value("imageSubdirectory", "images").toString().toStdString(), {"Images"}, {projectSubdirectoryFilter}}, -1);
				(*resourceFolders)->addProperty("meshSubdirectory", new Property<std::string, DisplayNameAnnotation, URIAnnotation>{settings.value("meshSubdirectory", "meshes").toString().toStdString(), {"Meshes"}, {projectSubdirectoryFilter}}, -1);
				(*resourceFolders)->addProperty("scriptSubdirectory", new Property<std::string, DisplayNameAnnotation, URIAnnotation>{settings.value("scriptSubdirectory", "scripts").toString().toStdString(), {"Scripts"}, {projectSubdirectoryFilter}}, -1);
				(*resourceFolders)->addProperty("shaderSubdirectory", new Property<std::string, DisplayNameAnnotation, URIAnnotation>{settings.value("shaderSubdirectory", "shaders").toString().toStdString(), {"Shaders"}, {projectSubdirectoryFilter}}, -1);
				dynObj->addProperty("defaultResourceFolders", resourceFolders, -1);
			}
		}
	}

	// The following code repairs URIs which have been "rerooted" incorrectly during paste.
	if (deserializedIR.fileVersion < 23) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto findContainingPrefabInstance = [](SEditorObject object) -> SEditorObject {
				SEditorObject current = object;
				while (current) {
					if (auto inst = current->as<PrefabInstance>()) {
						return inst;
					}
					current = current->getParent();
				}
				return nullptr;
			};

			auto mapFromInstance = [](SEditorObject obj, SEditorObject instance) -> SEditorObject {
				if (Table* item = findItemByValue(instance->get("mapToInstance")->asTable(), obj)) {
					return item->get(0)->asRef();
				}
				return nullptr;
			};

			if (auto prefabInst = findContainingPrefabInstance(dynObj)) {
				if (auto prefab = prefabInst->get("template")->asRef()) {
					if (prefab->query<core::ExternalReferenceAnnotation>()) {
						for (size_t propIndex = 0; propIndex < dynObj->size(); propIndex++) {
							if (dynObj->get(propIndex)->query<core::URIAnnotation>()) {
								const auto& propName = dynObj->name(propIndex);

								SEditorObject prefabObj = dynObj;
								while (auto currentPrefabInst = findContainingPrefabInstance(prefabObj)) {
									// The "maptoInstance" property of the PrefabInstance is only filled for the outermost PrefabInstance if
									// there are nested instances. So we need to search the topmost PrefabInstance first:
									while (auto parentPrefabInst = findContainingPrefabInstance(currentPrefabInst->getParent())) {
										currentPrefabInst = parentPrefabInst;
									}
									prefabObj = mapFromInstance(prefabObj, currentPrefabInst);
								}

								auto& prefabInstPropValue = dynObj->get(propIndex)->asString();
								const auto& prefabPropValue = prefabObj->get(propName)->asString();
								if (prefabInstPropValue != prefabPropValue) {
									LOG_WARNING(log_system::DESERIALIZATION, "Rewrite URI property '{}.{}': '{}' -> '{}' in project '{}'",
										dynObj->objectName(), propName,
										prefabInstPropValue, prefabPropValue,
										deserializedIR.currentPath);
									prefabInstPropValue = prefabPropValue;
								}
							}
						}
					}
				}
			}
		}
	}

	// File version 24: Deterministics object IDs for PrefabInstance child objects
	if (deserializedIR.fileVersion < 24) {
		std::vector<SEditorObject> sortedInstances;

		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "PrefabInstance") {
				insertPrefabInstancesRecursive(dynObj, sortedInstances);
			}
		}

		for (auto instance : sortedInstances) {
			auto dynInst = std::dynamic_pointer_cast<DynamicEditorObject>(instance);

			auto oldProp = dynInst->extractProperty("mapToInstance");
			const Table& instMapTable = oldProp->asTable();

			for (size_t index = 0; index < instMapTable.size(); index++) {
				const Table& item = instMapTable.get(index)->asTable();
				auto prefabChild = item.get(0)->asRef();
				auto instChild = item.get(1)->asRef();
				auto instChildID = EditorObject::XorObjectIDs(prefabChild->objectID(), instance->objectID());
				instChild->objectID_ = instChildID;
			}
		}
	}

	// File version 30 : Animation property changes : removed loop, play, rewindOnStop properties and added progress property.
	if (deserializedIR.fileVersion < 30) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();
			if (instanceType == "Animation") {
				dynObj->removeProperty("play");
				dynObj->removeProperty("loop");
				dynObj->removeProperty("rewindOnStop");
			}
		}

		auto it = deserializedIR.links.begin();
		while (it != deserializedIR.links.end()) {
			auto endObj = *(*it)->endObject_;
			if (endObj->serializationTypeName() == "Animation") {
				it = deserializedIR.links.erase(it);
			} else {
				++it;
			}
		}
	}

	// File version 33: Added HiddenProperty annotation to all tag - related properties
	if (deserializedIR.fileVersion < 33) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "Node" || instanceType == "MeshNode" || instanceType == "PrefabInstance" || instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera" || instanceType == "RenderLayer" || instanceType == "Material") {
				if (dynObj->hasProperty("tags")) {
					auto oldProp = dynObj->extractProperty("tags");
					auto newProp = new Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation>{oldProp->asTable(), {}, {}, {}, {"Tags"}};
					dynObj->addProperty("tags", newProp, -1);
				}
			}

			if (instanceType == "RenderLayer") {
				if (dynObj->hasProperty("materialFilterTags")) {
					auto oldProp = dynObj->extractProperty("materialFilterTags");
					auto newProp = new Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation>{oldProp->asTable(), {}, {}, {}, {"Material Filter Tags"}};
					dynObj->addProperty("materialFilterTags", newProp, -1);
				}

				if (dynObj->hasProperty("renderableTags")) {
					auto oldProp = dynObj->extractProperty("renderableTags");
					auto newProp = new Property<Table, RenderableTagContainerAnnotation, HiddenProperty, DisplayNameAnnotation>{oldProp->asTable(), {}, {}, {"Renderable Tags"}};
					dynObj->addProperty("renderableTags", newProp, -1);
				}
			}
		}
	}

	// File version 34:  Replaced RenderLayer invertMaterialFilter bool by materialFilterMode int property.
	if (deserializedIR.fileVersion < 34) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "RenderLayer") {
				if (dynObj->hasProperty("invertMaterialFilter")) {
					auto oldProp = dynObj->extractProperty("invertMaterialFilter");
					// 1 -> Exclusive, 0 -> Inclusive
					int newValue = oldProp->asBool() ? 1 : 0;
					auto newProp = new Property<int, DisplayNameAnnotation, EnumerationAnnotation>{newValue, {"Material Filter Mode"}, core::EUserTypeEnumerations::RenderLayerMaterialFilterMode};
					dynObj->addProperty("materialFilterMode", newProp, -1);
				}
			}
		}
	}

	// File version 36: LuaInterfaces instead of LuaScripts as Prefab/PrefabInstance interfaces
	if (deserializedIR.fileVersion < 36) {
		// Add LinkEndAnnotation to LuaScript::luaInputs_ property
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "LuaScript") {
				auto oldInputs = dynObj->extractProperty("luaInputs");
				auto newInputs = dynObj->addProperty("luaInputs", new Property<Table, DisplayNameAnnotation, LinkEndAnnotation>{{}, DisplayNameAnnotation("Inputs"), {}}, -1);
				*newInputs = *oldInputs;
			}
		}

		// create interface objects
		// - synthesize lua from lua script inputs
		// - write file
		// - create interface objects (careful with object ids)

		auto projectPath = utils::u8path(deserializedIR.currentPath).normalized().parent_path();
		auto scriptRelPath = utils::u8path("interfaces");
		std::filesystem::create_directories(projectPath / scriptRelPath);

		std::vector<core::SLink> createdLinks;

		// Prefab pass 1:
		// Generate file names for the interface script we need to generate
		// - disambiguation based on script name and generated interface contents
		// - use file name of script if no ambiguity
		// - otherwise use object ids as fallback

		std::map<std::string, std::map<std::string, std::set<std::string>>> scriptNameToTextToIDMap;

		auto objectsCopy = deserializedIR.objects;
		for (const auto& dynObj : objectsCopy) {
			if (dynObj->serializationTypeName() == "Prefab") {
				for (auto child : dynObj->children_->asVector<SEditorObject>()) {
					if (child->serializationTypeName() == "LuaScript") {
						auto prefabScript = std::dynamic_pointer_cast<DynamicEditorObject>(child);
						if (!prefabScript->query<core::ExternalReferenceAnnotation>()) {
							auto interfaceText = generateInterfaceScript(prefabScript);
							utils::u8path path(prefabScript->get("uri")->asString());
							scriptNameToTextToIDMap[path.stem().string()][interfaceText].insert(prefabScript->objectID());
						}
					}
				}
			}
		}

		std::map<std::string, std::string> interfacePaths;
		for (auto const& [name, text_to_ids] : scriptNameToTextToIDMap) {
			if (text_to_ids.size() == 1) {
				for (auto const& id : text_to_ids.begin()->second) {
					interfacePaths[id] = (scriptRelPath / (name + ".lua")).string();
				}
			} else {
				for (auto const& id_set : text_to_ids) {
					for (auto id : id_set.second) {
						interfacePaths[id] = (scriptRelPath / (id + ".lua")).string();
					}
				}
			}
		}

		for (const auto& dynObj : objectsCopy) {
			if (dynObj->serializationTypeName() == "Prefab") {
				for (auto child : dynObj->children_->asVector<SEditorObject>()) {
					if (child->serializationTypeName() == "LuaScript") {
						auto prefabScript = std::dynamic_pointer_cast<DynamicEditorObject>(child);

						utils::u8path intfRelPath;

						if (!prefabScript->query<core::ExternalReferenceAnnotation>()) {
							auto it = interfacePaths.find(prefabScript->objectID());
							assert(it != interfacePaths.end());
							intfRelPath = it->second;
							auto interfaceText = generateInterfaceScript(prefabScript);
							auto intfAbsPath = projectPath / intfRelPath;
							LOG_INFO(log_system::DESERIALIZATION, "Writing generated interface file {} -> {}", prefabScript->objectName(), intfAbsPath.string());
							utils::file::write(intfAbsPath, interfaceText);
						}

						auto interfaceObjID = EditorObject::XorObjectIDs(prefabScript->objectID(), "00000000-0000-0000-0000-000000000001");
						auto interfaceObj = createInterfaceObjectV36(factory, deserializedIR, createdLinks, prefabScript, interfaceObjID, intfRelPath, dynObj);
					}
				}
			}
		}

		for (const auto& dynObj : objectsCopy) {
			if (dynObj->serializationTypeName() == "PrefabInstance") {
				auto inst = dynObj;
				for (auto child : dynObj->children_->asVector<SEditorObject>()) {
					if (child->serializationTypeName() == "LuaScript") {
						auto instScript = std::dynamic_pointer_cast<DynamicEditorObject>(child);

						utils::u8path intfRelPath;

						// Note: if we haven't written the file above the uri must remain empty.
						// Since 'load' will reload/sync from external files before the external reference update the
						// sync will remove the interface properties (since the file doesn't exist yet). The extref
						// update will then create the file. But the succeeding prefab update will only perform another
						// external file reload/sync on the interface if the object changed during the prefab update.
						// If we set the uri correctly here there is no change though and no sync will be triggered.
						auto prefabScriptID = EditorObject::XorObjectIDs(instScript->objectID(), inst->objectID());
						auto it = interfacePaths.find(prefabScriptID);
						if (it != interfacePaths.end()) {
							intfRelPath = it->second;
						}

						std::string instInterfaceID = EditorObject::XorObjectIDs(instScript->objectID(), "00000000-0000-0000-0000-000000000001");
						createInterfaceObjectV36(factory, deserializedIR, createdLinks, instScript, instInterfaceID, intfRelPath, inst);
					}
				}
			}
		}

		std::copy(createdLinks.begin(), createdLinks.end(), std::back_inserter(deserializedIR.links));

		for (const auto& obj : deserializedIR.objects) {
			auto dynObj = std::dynamic_pointer_cast<serialization::proxy::DynamicEditorObject>(obj);
			dynObj->onAfterDeserialization();
		}
	}

	// File version 40: Renamed internal properties
	if (deserializedIR.fileVersion < 40) {
		{
			std::unordered_map<std::string, std::vector<std::string>> newPropertyStrings = {
				{"luaInputs", {"inputs"}},
				{"luaOutputs", {"outputs"}},
				{"scale", {"scaling"}},
				{"visible", {"visibility"}},
				{"tickerInput", {"inputs", "ticker_us"}},
				{"tickerOutput", {"outputs", "ticker_us"}}};

			for (auto& link : deserializedIR.links) {
				auto linkStartProps = link->startPropertyNamesVector();
				auto linkEndProps = link->endPropertyNamesVector();

				auto stringIt = newPropertyStrings.find(linkStartProps.front());
				if (stringIt != newPropertyStrings.end()) {
					auto newlinkStartProps = stringIt->second;
					newlinkStartProps.insert(newlinkStartProps.end(), linkStartProps.begin() + 1, linkStartProps.end());
					link->startProp_->set(newlinkStartProps);
				}

				stringIt = newPropertyStrings.find(linkEndProps.front());
				if (stringIt != newPropertyStrings.end()) {
					auto newlinkEndProps = stringIt->second;
					newlinkEndProps.insert(newlinkEndProps.end(), linkEndProps.begin() + 1, linkEndProps.end());
					link->endProp_->set(newlinkEndProps);
				}
			}
		}

		for (const auto& dynObj : deserializedIR.objects) {
			const auto& typeName = dynObj->serializationTypeName();
			if (typeName == "Node" || typeName == "MeshNode" || typeName == "PerspectiveCamera" || typeName == "OrthographicCamera" || typeName == "PrefabInstance") {
				auto oldScale = dynObj->extractProperty("scale");
				auto newScale = dynObj->addProperty("scaling", new Property<Vec3f, DisplayNameAnnotation, LinkEndAnnotation>{{}, DisplayNameAnnotation("Scaling"), {}}, -1);
				auto oldVis = dynObj->extractProperty("visible");
				auto newVis = dynObj->addProperty("visibility", new Property<bool, DisplayNameAnnotation, LinkEndAnnotation>{true, DisplayNameAnnotation("Visibility"), {}}, -1);

				*newScale = *oldScale;
				*newVis = *oldVis;
			} else if (typeName == "LuaScript") {
				auto oldInputs = dynObj->extractProperty("luaInputs");
				auto newInputs = dynObj->addProperty("inputs", new Property<Table, DisplayNameAnnotation, LinkEndAnnotation>{{}, DisplayNameAnnotation("Inputs"), {}}, -1);
				auto oldOutputs = dynObj->extractProperty("luaOutputs");
				auto newOutputs = dynObj->addProperty("outputs", new Property<Table, DisplayNameAnnotation>{{}, DisplayNameAnnotation("Outputs")}, -1);

				*newInputs = *oldInputs;
				*newOutputs = *oldOutputs;
			} else if (typeName == "LuaInterface") {
				auto oldInputs = dynObj->extractProperty("luaInputs");
				auto newInputs = dynObj->addProperty("inputs", new Property<Table, DisplayNameAnnotation, LinkStartAnnotation, LinkEndAnnotation>{{}, DisplayNameAnnotation("Inputs"), {}, {}}, -1);

				*newInputs = *oldInputs;
			} else if (typeName == "Timer") {
				auto newInputs = new Property<TimerInput, DisplayNameAnnotation>{{}, DisplayNameAnnotation("Inputs")};
				(*newInputs)->addProperty("ticker_us", dynObj->extractProperty("tickerInput"), -1);
				dynObj->addProperty("inputs", newInputs, -1);

				auto newOutputs = new Property<TimerOutput, DisplayNameAnnotation>{{}, DisplayNameAnnotation("Outputs")};
				(*newOutputs)->addProperty("ticker_us", dynObj->extractProperty("tickerOutput"), -1);
				dynObj->addProperty("outputs", newOutputs, -1);
			}
		}
	}

	// File version 41: Renamed Animation property: "animationOutputs" -> "outputs"
	if (deserializedIR.fileVersion < 41) {
		for (auto& link : deserializedIR.links) {
			auto linkStartProps = link->startPropertyNamesVector();
			if (linkStartProps.at(0) == "animationOutputs") {
				linkStartProps[0] = "outputs";
				link->startProp_->set(linkStartProps);
			}
		}

		for (const auto& dynObj : deserializedIR.objects) {
			const auto& typeName = dynObj->serializationTypeName();
			if (typeName == "Animation") {
				auto oldOutputs = dynObj->extractProperty("animationOutputs");
				auto newOutputs = dynObj->addProperty("outputs", new Property<Table, DisplayNameAnnotation>{{}, DisplayNameAnnotation("Outputs")}, -1);
				*newOutputs = *oldOutputs;
			}
		}
	}

	if (deserializedIR.fileVersion < 43) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "ProjectSettings") {
				if (dynObj->hasProperty("runTimer")) {
					dynObj->removeProperty("runTimer");
				}
				if (dynObj->hasProperty("enableTimerFlag")) {
					dynObj->removeProperty("enableTimerFlag");
				}
			}
		}
	}

	if (deserializedIR.fileVersion < 44) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "PerspectiveCamera") {
				if (dynObj->hasProperty("frustum")) {
					auto frustumStruct = dynObj->extractProperty("frustum");

					auto frustumTable = new Property<Table, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Frustum"}, {}};

					(*frustumTable)->addProperty("nearPlane", new Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>(frustumStruct->asStruct().get("nearPlane")->asDouble(), DisplayNameAnnotation("nearPlane"), RangeAnnotation<double>(0.1, 1.0), {}), -1);

					(*frustumTable)->addProperty("farPlane", new Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>(frustumStruct->asStruct().get("farPlane")->asDouble(), DisplayNameAnnotation("farPlane"), RangeAnnotation<double>(100.0, 10000.0), {}), -1);

					(*frustumTable)->addProperty("fieldOfView", new Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>(frustumStruct->asStruct().get("fieldOfView")->asDouble(), DisplayNameAnnotation("fieldOfView"), RangeAnnotation<double>(10.0, 120.0), {}), -1);

					(*frustumTable)->addProperty("aspectRatio", new Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>(frustumStruct->asStruct().get("aspectRatio")->asDouble(), DisplayNameAnnotation("aspectRatio"), RangeAnnotation<double>(0.5, 4.0), {}), -1);

					dynObj->addProperty("frustum", frustumTable, -1);
				}
			}

			if (dynObj->serializationTypeName() == "RenderPass") {
				if (dynObj->hasProperty("enabled")) {
					auto enabled = dynObj->extractProperty("enabled");
					dynObj->addProperty("enabled", new Property<bool, DisplayNameAnnotation, LinkEndAnnotation>(enabled->asBool(), {"Enabled"}, {}), -1);
				}

				if (dynObj->hasProperty("order")) {
					auto order = dynObj->extractProperty("order");
					dynObj->addProperty("renderOrder", new Property<int, DisplayNameAnnotation, LinkEndAnnotation>(order->asInt(), {"Render Order"}, {2}), -1);
				}

				if (dynObj->hasProperty("clearColor")) {
					auto oldClearColor = dynObj->extractProperty("clearColor");
					auto newClearColor = dynObj->addProperty("clearColor", new Property<Vec4f, DisplayNameAnnotation, LinkEndAnnotation>({}, {"Clear Color"}, {2}), -1);
					*newClearColor = *oldClearColor;
				}
			}
		}
	}

	// File version 45 : Added LinkEndAnnotation to all properties in the RenderLayer::renderableTags property
	if (deserializedIR.fileVersion < 45) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "RenderLayer") {
				if (dynObj->hasProperty("renderableTags")) {
					auto oldRenderables = dynObj->extractProperty("renderableTags");
					auto newRenderables = new Property<Table, RenderableTagContainerAnnotation, DisplayNameAnnotation>{{}, {}, {"Renderable Tags"}};

					Table& oldTable = oldRenderables->asTable();
					for (size_t i = 0; i < oldTable.size(); i++) {
						int oldValue = oldTable.get(i)->asInt();
						(*newRenderables)->addProperty(oldTable.name(i), new Property<int, LinkEndAnnotation>(oldValue, {3}), -1);
					}

					dynObj->addProperty("renderableTags", newRenderables, -1);
				}
			}
		}
	}

	// File version 46: changed BaseCamera viewport width and height ranges
	if (deserializedIR.fileVersion < 46) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();
			if (instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera") {
				if (dynObj->hasProperty("viewport")) {
					auto& viewportprop = dynObj->get("viewport")->asStruct();
					auto widthRange = viewportprop.get("width")->query<RangeAnnotation<int>>();
					widthRange->min_ = 1;
					auto heightRange = viewportprop.get("height")->query<RangeAnnotation<int>>();
					heightRange->min_ = 1;
				}
			}

			if (instanceType == "RenderTarget") {
				if (dynObj->hasProperty("buffer0")) {
					auto oldProp = dynObj->extractProperty("buffer0");
					auto newProp = dynObj->addProperty("buffer0", new Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference>({}, {"Buffer 0"}, {}), -1);
					*newProp = oldProp->asRef();
				}
			}
		}
	}

	// File version 51: Added support for struct uniforms
	if (deserializedIR.fileVersion < 51) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "Material" && dynObj->hasProperty("uniforms")) {
				auto& uniforms = dynObj->get("uniforms")->asTable();
				replaceUniforms_V51(uniforms);
			}

			if (instanceType == "MeshNode" && dynObj->hasProperty("materials")) {
				auto* materials = &dynObj->get("materials")->asTable();
				for (size_t i = 0; i < materials->size(); i++) {
					Table& matCont = materials->get(i)->asTable();
					Table& uniformsCont = matCont.get("uniforms")->asTable();
					replaceUniforms_V51(uniformsCont);
				}
			}
		}

		auto migrateLink = [](core::SLink link, int numPrefixComponents) {
			auto linkEndProps = link->endPropertyNamesVector();

			std::vector<std::string> names;
			std::vector<core::EnginePrimitive> types;
			splitUniformName(linkEndProps[numPrefixComponents], core::EnginePrimitive::Undefined, names, types);

			if (names.size() > 1) {
				std::vector<std::string> newLinkEndProps;
				newLinkEndProps.insert(newLinkEndProps.end(), linkEndProps.begin(), linkEndProps.begin() + numPrefixComponents);
				newLinkEndProps.insert(newLinkEndProps.end(), names.begin(), names.end());
				newLinkEndProps.insert(newLinkEndProps.end(), linkEndProps.begin() + numPrefixComponents + 1, linkEndProps.end());

				link->endProp_->set(newLinkEndProps);
			}
		};

		for (auto& link : deserializedIR.links) {
			auto endType = (*link->endObject_)->serializationTypeName();
			if (endType == "Material") {
				auto linkEndProps = link->endPropertyNamesVector();
				if (linkEndProps[0] == "uniforms") {
					migrateLink(link, 1);
				}
			} else if (endType == "MeshNode") {
				auto linkEndProps = link->endPropertyNamesVector();
				if (linkEndProps.size() >= 3 && linkEndProps[2] == "uniforms") {
					migrateLink(link, 3);
				}
			}
		}
	}

	// File version 52 : Made MeshNode 'instanceCount' property linkable
	if (deserializedIR.fileVersion < 52) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "MeshNode") {
				if (dynObj->hasProperty("instanceCount")) {
					auto count = dynObj->extractProperty("instanceCount");
					dynObj->addProperty("instanceCount", new Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation>{count->asInt(), RangeAnnotation<int>(1, 20), DisplayNameAnnotation("Instance Count"), {5}}, -1);
				}
			}
		}
	}

	// File version 53 : Added the 'folderTypeKey' property to the URIAnnotation
	if (deserializedIR.fileVersion < 52) {
		std::map<std::string, core::PathManager::FolderTypeKeys> folderTypeKeys = {
			{"CubeMap", core::PathManager::FolderTypeKeys::Image},
			{"Texture", core::PathManager::FolderTypeKeys::Image},

			{"Mesh", core::PathManager::FolderTypeKeys::Mesh},
			{"AnimationChannel", core::PathManager::FolderTypeKeys::Mesh},
			{"Skin", core::PathManager::FolderTypeKeys::Mesh},

			{"LuaScript", core::PathManager::FolderTypeKeys::Script},
			{"LuaScriptModule", core::PathManager::FolderTypeKeys::Script},

			{"LuaInterface", core::PathManager::FolderTypeKeys::Interface},

			{"Material", core::PathManager::FolderTypeKeys::Shader},

			{"ProjectSettings", core::PathManager::FolderTypeKeys::Project}};

		for (const auto& dynObj : deserializedIR.objects) {
			for (size_t index = 0; index < dynObj->size(); index++) {
				auto uriAnno = dynObj->query<URIAnnotation>();
				if (uriAnno) {
					auto it = folderTypeKeys.find(dynObj->serializationTypeName());
					if (it != folderTypeKeys.end()) {
						uriAnno->folderTypeKey_ = static_cast<int>(it->second);
					}
				}
			}
		}
	}

	// File version 55: conversion of user types from Table and fixed properties to Array properties
	// Migration of EditorObject::children property from Table -> Array type is done implicitly by the deserialization
	// since we can't change the types of EditorObject properties in the migration.
	if (deserializedIR.fileVersion < 55) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "RenderPass") {
				auto newProperty = dynObj->addProperty("layers", new Property<Array<SRenderLayer>, DisplayNameAnnotation, ExpectEmptyReference>({}, {"Layers"}, {}), -1);

				for (auto propName : {"layer0", "layer1", "layer2", "layer3", "layer4", "layer5", "layer6", "layer7"}) {
					if (dynObj->hasProperty(propName)) {
						auto oldLayer = dynObj->extractProperty(propName);
						*newProperty->asArray().addProperty() = oldLayer->asRef();
					} else {
						*newProperty->asArray().addProperty() = SRenderLayer();
					}
				}
			}

			if (instanceType == "RenderTarget") {
				auto newBuffers = dynObj->addProperty("buffers", new Property<Array<SRenderBuffer>, DisplayNameAnnotation, ExpectEmptyReference>({}, {"Buffers"}, {}), -1);
				for (auto propName : {"buffer0", "buffer1", "buffer2", "buffer3", "buffer4", "buffer5", "buffer6", "buffer7"}) {
					if (dynObj->hasProperty(propName)) {
						auto oldProp = dynObj->extractProperty(propName);
						*newBuffers->asArray().addProperty() = oldProp->asRef();
					} else {
						*newBuffers->asArray().addProperty() = SRenderBuffer();
					}
				}

				auto newBuffersMS = dynObj->addProperty("buffersMS", new Property<Array<SRenderBufferMS>, DisplayNameAnnotation, ExpectEmptyReference>({}, {"Buffers (Multisampled)"}, {}), -1);
				for (auto propName : {"bufferMS0", "bufferMS1", "bufferMS2", "bufferMS3", "bufferMS4", "bufferMS5", "bufferMS6", "bufferMS7"}) {
					if (dynObj->hasProperty(propName)) {
						auto oldProp = dynObj->extractProperty(propName);
						*newBuffersMS->asArray().addProperty() = oldProp->asRef();
					} else {
						*newBuffersMS->asArray().addProperty() = SRenderBufferMS();
					}
				}
			}

			if (instanceType == "Animation") {
				if (dynObj->hasProperty("animationChannels")) {
					auto oldChannels = dynObj->extractProperty("animationChannels");
					auto newChannels = dynObj->addProperty("animationChannels", new Property<Array<SAnimationChannel>, DisplayNameAnnotation>({}, {"Animation Channels"}), -1);

					Table& oldTable = oldChannels->asTable();
					for (size_t i = 0; i < oldTable.size(); i++) {
						*newChannels->asArray().addProperty() = oldTable.get(i)->asRef();
					}
				}
			}

			if (instanceType == "Skin") {
				if (dynObj->hasProperty("targets")) {
					auto oldTargets = dynObj->extractProperty("targets");
					auto newTargets = dynObj->addProperty("targets", new Property<Array<SMeshNode>, DisplayNameAnnotation>({}, {"Target MeshNodes"}), -1);

					Table& oldTable = oldTargets->asTable();
					for (size_t i = 0; i < oldTable.size(); i++) {
						*newTargets->asArray().addProperty() = oldTable.get(i)->asRef();
					}
				}

				if (dynObj->hasProperty("joints")) {
					auto oldJoints = dynObj->extractProperty("joints");
					auto newJoints = dynObj->addProperty("joints", new Property<Array<SNode>, DisplayNameAnnotation>({}, {"Joint Nodes"}), -1);

					Table& oldTable = oldJoints->asTable();
					for (size_t i = 0; i < oldTable.size(); i++) {
						*newJoints->asArray().addProperty() = oldTable.get(i)->asRef();
					}
				}
			}
		}
	}

	// File version 56 : Split RenderTarget into RenderTarget and RenderTargetMS classes
	if (deserializedIR.fileVersion < 56) {
		std::vector<SDynamicEditorObject> toRemove;

		// Pass 1: change type of RenderPass target property
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "RenderPass") {
				if (dynObj->hasProperty("target")) {
					auto oldProp = dynObj->extractProperty("target");
					auto newProp = dynObj->addProperty("target", new Property<SRenderTargetBase, DisplayNameAnnotation, ExpectEmptyReference>({}, {"Target"}, {"Default Framebuffer"}), -1);
					*newProp = *oldProp;
				}
			}
		}

		// Pass 2: split RenderTargets
		// if this creates mew RenderTargMS objects the RenderPasses target properties must be changed, but we can't do this in
		// a single pass since we need to change the target property type before we can set it to a RenderTargetMS.

		// Iterate over copy of the objects since we create additional objects leading to iterator invalidation otherwise.
		auto objectsCopy = deserializedIR.objects;
		for (const auto& dynObj : objectsCopy) {
			if (dynObj->serializationTypeName() == "RenderTarget") {
				bool hasNormalBuffers = false;
				bool hasMSBuffers = false;
				if (dynObj->hasProperty("buffers")) {
					auto array_ptr = dynamic_cast<data_storage::Array<SRenderBuffer>*>(&dynObj->get("buffers")->asArray());
					auto buffers = array_ptr->asVector<SRenderBuffer>();
					hasNormalBuffers = std::any_of(buffers.begin(), buffers.end(), [](auto object) {
						return object != nullptr;
					});
				}

				if (dynObj->hasProperty("buffersMS")) {
					auto array_ptr = dynamic_cast<data_storage::Array<SRenderBufferMS>*>(&dynObj->get("buffersMS")->asArray());
					auto buffers = array_ptr->asVector<SRenderBufferMS>();
					hasMSBuffers = std::any_of(buffers.begin(), buffers.end(), [](auto object) {
						return object != nullptr;
					});
				}

				if (!hasMSBuffers && dynObj->hasProperty("buffersMS")) {
					dynObj->removeProperty("buffersMS");
				}

				if (hasNormalBuffers && hasMSBuffers && dynObj->hasProperty("buffersMS")) {
					dynObj->removeProperty("buffersMS");
					deserializedIR.migrationObjWarnings[dynObj->objectID()] = fmt::format("RenderTarget object '{}' has both non-empty buffer and bufferMS properties: it will be converted to a RenderTarget losing the bufferMS properties.", dynObj->objectName());
				}

				// !!! Wrong !!!
				// The save file optimization may have removed buffer references if the RenderTarget is an external reference.
				// In this case the check below may be wrong, i.e. different than without the save file optimization.
				// Since the information needed to make the correct decision is not present in the file we can't make 
				// the right choice here in all cases.
				// Instead the external reference update itself contains fixup code for this case.

				if (!hasNormalBuffers && hasMSBuffers) {
					// Replace the object by a RenderTargetMS with same object id and property values
					auto targetMS = std::dynamic_pointer_cast<DynamicEditorObject>(factory.createObject("RenderTargetMS", dynObj->objectName(), dynObj->objectID()));

					if (auto anno = dynObj->query<core::ExternalReferenceAnnotation>()) {
						targetMS->addAnnotation(std::make_shared<core::ExternalReferenceAnnotation>(*anno->projectID_));
					}

					if (dynObj->hasProperty("userTags")) {
						targetMS->addProperty("userTags", dynObj->extractProperty("userTags"), -1);
					}

					targetMS->addProperty("buffers", dynObj->extractProperty("buffersMS"), -1);

					deserializedIR.objects.emplace_back(targetMS);
					toRemove.emplace_back(dynObj);

					// Point references to the RenderTarget to the new RenderTargetMS instead:
					for (auto wobj : dynObj->referencesToThis()) {
						if (auto obj = wobj.lock()) {
							assert(obj->serializationTypeName() == "RenderPass");
							if (obj->hasProperty("target")) {
								*obj->get("target") = targetMS;
							}
						}
					}
				}
			}
		}

		for (auto obj : toRemove) {
			auto it = std::find(deserializedIR.objects.begin(), deserializedIR.objects.end(), obj);
			assert(it != deserializedIR.objects.end());
			deserializedIR.objects.erase(it);
		}

		// We need to update the back and parent pointers since we change the pointer structure above:
		recreateBackPointers(deserializedIR);
	}

	// File version 58: Removed userTags property from ProjectSettings
	if (deserializedIR.fileVersion < 58) {
		for (const auto& dynObj : deserializedIR.objects) {
			if (dynObj->serializationTypeName() == "ProjectSettings") {
				if (dynObj->hasProperty("userTags")) {
					dynObj->removeProperty("userTags");
				}
			}
		}
	}

	if (deserializedIR.fileVersion < 59) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "RenderPass") {
				if (dynObj->hasProperty("layers")) {
					auto oldProp = dynObj->extractProperty("layers");
					auto newProp = dynObj->addProperty("layers", new Property<Array<SRenderLayer>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray>({}, {"Layers"}, {}, {}), -1);
					*newProp = *oldProp;
				}
			}

			if (instanceType == "RenderTarget") {
				if (dynObj->hasProperty("buffers")) {
					auto oldProp = dynObj->extractProperty("buffers");
					auto newProp = dynObj->addProperty("buffers", new Property<Array<SRenderBuffer>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray>({}, {"Buffers"}, {}, {}), -1);
					*newProp = *oldProp;
				}
			}

			if (instanceType == "RenderTargetMS") {
				if (dynObj->hasProperty("buffers")) {
					auto oldProp = dynObj->extractProperty("buffers");
					auto newProp = dynObj->addProperty("buffers", new Property<Array<SRenderBufferMS>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray>({}, {"Buffers"}, {}, {}), -1);
					*newProp = *oldProp;
				}
			}

			if (instanceType == "Animation") {
				if (dynObj->hasProperty("animationChannels")) {
					auto oldProp = dynObj->extractProperty("animationChannels");
					auto newProp = dynObj->addProperty("animationChannels", new Property<Array<SAnimationChannel>, DisplayNameAnnotation, ResizableArray>({}, {"Animation Channels"}, {}), -1);
					*newProp = *oldProp;
				}
			}

			if (instanceType == "Skin") {
				if (dynObj->hasProperty("targets")) {
					auto oldProp = dynObj->extractProperty("targets");
					auto newProp = dynObj->addProperty("targets", new Property<Array<SMeshNode>, DisplayNameAnnotation, ResizableArray>({}, {"Target MeshNodes"}, {}), -1);
					*newProp = *oldProp;
				}
			}
		}
	}

	// Migration from version 60 -> 2001 (RaCo 2.x)
	// - feature level reset
	if (deserializedIR.fileVersion > 60 && deserializedIR.fileVersion < 2001) {
		throw std::runtime_error("non-migratable file version");
	}
	if (deserializedIR.fileVersion <= 60) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			// reset ProjectSettings feature level to 1
			if (instanceType == "ProjectSettings") {
				if (dynObj->hasProperty("featureLevel")) {
					*dynObj->get("featureLevel") = 1;
				}
			}

			// remove FeatureLevel annotation from Node::enabled property
			if (instanceType == "Node" || instanceType == "MeshNode" || instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera" || instanceType == "PrefabInstance") {
				if (dynObj->hasProperty("enabled")) {
					auto oldProp = dynObj->extractProperty("enabled");
					dynObj->addProperty("enabled", new Property<bool, DisplayNameAnnotation, LinkEndAnnotation>{oldProp->asBool(), DisplayNameAnnotation("Enabled"), {}}, -1);
				}
			}

			// remove FeatureLevel annotation from PerspectiveCamera::frustumType property
			if (instanceType == "PerspectiveCamera") {
				if (dynObj->hasProperty("frustumType")) {
					auto oldProp = dynObj->extractProperty("frustumType");
					dynObj->addProperty("frustumType", new Property<int, DisplayNameAnnotation, EnumerationAnnotation>{oldProp->asInt(), {"Frustum Type"}, {core::EUserTypeEnumerations::FrustumType}}, -1);
				}
			}

			// remove FeatureLevel annotation from RenderPass::renderOnce property
			if (instanceType == "RenderPass") {
				if (dynObj->hasProperty("renderOnce")) {
					auto oldProp = dynObj->extractProperty("renderOnce");
					dynObj->addProperty("renderOnce", new Property<bool, DisplayNameAnnotation, LinkEndAnnotation>{oldProp->asBool(), {"Render Once"}, {}}, -1);
				}
			}

			// remove FeatureLevel annotation from LuaInterface luaModules and stdModules properties
			if (instanceType == "LuaInterface") {
				if (dynObj->hasProperty("luaModules")) {
					auto oldProp = dynObj->extractProperty("luaModules");
					auto newProp = dynObj->addProperty("luaModules", new Property<Table, DisplayNameAnnotation>{{}, DisplayNameAnnotation("Modules")}, -1);
					*newProp = *oldProp;
				}
				if (dynObj->hasProperty("stdModules")) {
					auto oldProp = dynObj->extractProperty("stdModules");
					auto newProp = dynObj->addProperty("stdModules", new Property<LuaStandardModuleSelection, DisplayNameAnnotation>{{}, {"Standard Modules"}}, -1);
					*newProp = *oldProp;
				}
			}

			// reset feature level in LinkEndAnnotation of renderableTags child properties to 1
			if (instanceType == "RenderLayer") {
				if (dynObj->hasProperty("renderableTags")) {
					auto& tags = dynObj->get("renderableTags")->asTable();
					for (size_t i = 0; i < tags.size(); i++) {
						auto anno = tags.get(i)->query<LinkEndAnnotation>();
						anno->featureLevel_ = 1;
					}
				}
			}
		}
	}
}	
	
}  // namespace raco::serialization
