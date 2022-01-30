/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

#include "log_system/log.h"

#include "user_types/EngineTypeAnnotation.h"

#include <spdlog/fmt/fmt.h>

#include "core/PathManager.h"
#include <QSettings>

namespace raco::serialization {

void linkReplaceEndIfMatching(raco::core::SLink& link, const std::string& oldProp, const std::vector<std::string>& newEndProp) {
	if (link->compareEndPropertyNames({oldProp})) {
		link->endProp_->set<std::string>(newEndProp);
	}
};

template <class... Args>
raco::data_storage::ValueBase* createDynamicProperty_V11(raco::core::EnginePrimitive type) {
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

raco::data_storage::Table* findItemByValue(raco::data_storage::Table& table, raco::core::SEditorObject obj) {
	for (size_t i{0}; i < table.size(); i++) {
		raco::data_storage::Table& item = table.get(i)->asTable();
		if (item.get(1)->asRef() == obj) {
			return &item;
		}
	}
	return nullptr;
}

void insertPrefabInstancesRecursive(raco::serialization::proxy::SEditorObject inst, std::vector<raco::serialization::proxy::SEditorObject>& sortedInstances) {
	if (std::find(sortedInstances.begin(), sortedInstances.end(), inst) == sortedInstances.end()) {
		if (auto prefab = inst->get("template")->asRef()) {
			for (auto prefabChild : raco::core::TreeIteratorAdaptor(prefab)) {
				if (prefabChild->serializationTypeName() == "PrefabInstance") {
					insertPrefabInstancesRecursive(prefabChild, sortedInstances);
				}
			}
		}
		sortedInstances.emplace_back(inst);
	}
}

// Limitations
// - Annotations and links are handled as static classes:
//   the class definition is not supposed to change in any observable way:
//   annotation name, property types and names, number of properties, serialiationRequired flag must not change.
// - If the need arises to migrate the annotationns we would need to create proxy types for them in the same
//   way currently done for the Struct PrimitiveTypes.

void migrateProject(ProjectDeserializationInfoIR& deserializedIR) {
	using namespace raco::data_storage;
	using namespace raco::serialization::proxy;

	if (deserializedIR.fileVersion < 2) {
		auto settingsID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto settings = std::make_shared<raco::serialization::proxy::ProjectSettings>("", settingsID);
		deserializedIR.objects.emplace_back(settings);
	}

	// File Version 10: cameras store viewport as four individual integers instead of a vec4i (for camera bindings).
	if (deserializedIR.fileVersion < 10) {
		for (const auto& dynObj : deserializedIR.objects) {
			auto instanceType = dynObj->serializationTypeName();

			if (instanceType == "PerspectiveCamera" || instanceType == "OrthographicCamera") {
				auto& oldviewportprop = *dynObj->get("viewport");
				dynObj->addProperty("viewPortOffsetX", new data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i1_.asInt(), {-7680, 7680}, {"Viewport Offset X"}, {}}, -1);
				dynObj->addProperty("viewPortOffsetY", new data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i2_.asInt(), {-7680, 7680}, {"Viewport Offset Y"}, {}}, -1);
				dynObj->addProperty("viewPortWidth", new data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i3_.asInt(), {0, 7680}, {"Viewport Width"}, {}}, -1);
				dynObj->addProperty("viewPortHeight", new data_storage::Property<int, RangeAnnotation<int>, data_storage::DisplayNameAnnotation, core::LinkEndAnnotation>{oldviewportprop.asVec4i().i4_.asInt(), {0, 7680}, {"Viewport Height"}, {}}, -1);
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
					auto engineType = uniforms->get(i)->query<raco::user_types::EngineTypeAnnotation>()->type();
					if (raco::core::PropertyInterface::primitiveType(engineType) != PrimitiveType::Ref) {
						auto newValue = createDynamicProperty_V11<raco::core::LinkEndAnnotation>(engineType);
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
				auto viewport = new Property<raco::serialization::proxy::CameraViewport, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Viewport"}, {}};
				(*viewport)->addProperty("offsetX", dynObj->extractProperty("viewPortOffsetX"), -1);
				(*viewport)->addProperty("offsetY", dynObj->extractProperty("viewPortOffsetY"), -1);
				(*viewport)->addProperty("width", dynObj->extractProperty("viewPortWidth"), -1);
				(*viewport)->addProperty("height", dynObj->extractProperty("viewPortHeight"), -1);
				dynObj->addProperty("viewport", viewport, -1);
			}

			if (instanceType == "PerspectiveCamera") {
				auto frustum = new Property<raco::serialization::proxy::PerspectiveFrustum, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Frustum"}, {}};
				(*frustum)->addProperty("nearPlane", dynObj->extractProperty("near"), -1);
				(*frustum)->addProperty("farPlane", dynObj->extractProperty("far"), -1);
				(*frustum)->addProperty("fieldOfView", dynObj->extractProperty("fov"), -1);
				(*frustum)->addProperty("aspectRatio", dynObj->extractProperty("aspect"), -1);
				dynObj->addProperty("frustum", frustum, -1);
			}

			if (instanceType == "OrthographicCamera") {
				auto frustum = new Property<raco::serialization::proxy::OrthographicFrustum, DisplayNameAnnotation, LinkEndAnnotation>{{}, {"Frustum"}, {}};
				(*frustum)->addProperty("nearPlane", dynObj->extractProperty("near"), -1);
				(*frustum)->addProperty("farPlane", dynObj->extractProperty("far"), -1);
				(*frustum)->addProperty("leftPlane", dynObj->extractProperty("left"), -1);
				(*frustum)->addProperty("rightPlane", dynObj->extractProperty("right"), -1);
				(*frustum)->addProperty("bottomPlane", dynObj->extractProperty("bottom"), -1);
				(*frustum)->addProperty("topPlane", dynObj->extractProperty("top"), -1);
				dynObj->addProperty("frustum", frustum, -1);
			}

			if (instanceType == "Material") {
				auto options = new Property<raco::serialization::proxy::BlendOptions, DisplayNameAnnotation>{{}, {"Options"}};
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

					auto options = new Property<raco::serialization::proxy::BlendOptions, DisplayNameAnnotation>{{}, {"Options"}};
					(*options)->addProperty("blendOperationColor", optionsCont.get("blendOperationColor")->clone({}), -1);
					(*options)->addProperty("blendOperationAlpha", optionsCont.get("blendOperationAlpha")->clone({}), -1);
					(*options)->addProperty("blendFactorSrcColor", optionsCont.get("blendFactorSrcColor")->clone({}), -1);
					(*options)->addProperty("blendFactorDestColor", optionsCont.get("blendFactorDestColor")->clone({}), -1);
					(*options)->addProperty("blendFactorSrcAlpha", optionsCont.get("blendFactorSrcAlpha")->clone({}), -1);
					(*options)->addProperty("blendFactorDestAlpha", optionsCont.get("blendFactorDestAlpha")->clone({}), -1);
					(*options)->addProperty("blendColor", optionsCont.get("blendColor"), -1);
					(*options)->addProperty("depthwrite", optionsCont.get("depthwrite"), -1);
					(*options)->addProperty("depthFunction", optionsCont.get("depthFunction"), -1);
					(*options)->addProperty("cullmode", optionsCont.get("cullmode"), -1);

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
					auto engineType = uniforms.get(i)->query<raco::user_types::EngineTypeAnnotation>()->type();
					if (engineType == raco::core::EnginePrimitive::TextureSampler2D) {
						auto newValue = ProxyObjectFactory::staticCreateProperty<STextureSampler2DBase, raco::user_types::EngineTypeAnnotation>({}, {engineType});
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
					auto tags = new Property<Table, raco::core::ArraySemanticAnnotation, raco::core::TagContainerAnnotation, DisplayNameAnnotation>{{}, {}, {}, {"Tags"}};
					tags->set(std::vector<std::string>({"render_main"}));
					dynObj->addProperty("tags", tags, -1);
				}
			}
		}

		SDynamicEditorObject camera = perspCamera ? perspCamera : orthoCamera;

		auto layerID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto mainLayer = std::make_shared<raco::serialization::proxy::RenderLayer>("MainRenderLayer", layerID);
		auto renderableTagsProp = new Property<Table, raco::core::RenderableTagContainerAnnotation, DisplayNameAnnotation>{{}, {}, {"Renderable Tags"}};
		(*renderableTagsProp)->addProperty("render_main", std::make_unique<data_storage::Value<int>>(0));
		mainLayer->addProperty("renderableTags", renderableTagsProp, -1);
		mainLayer->addProperty("sortOrder", new Property<int, DisplayNameAnnotation, EnumerationAnnotation>{2, {"Render Order"}, raco::core::EngineEnumeration::RenderLayerOrder}, -1);

		auto passID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
		auto mainPass = std::make_shared<raco::serialization::proxy::RenderPass>("MainRenderPass", passID);

		ValueBase* cameraProp = new Property<raco::serialization::proxy::SBaseCamera, DisplayNameAnnotation>{{}, {"Camera"}};
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

			auto& scaleVec = dynObj->get("scale")->asVec3f();
			auto& rotationVec = dynObj->get("rotation")->asVec3f();
			auto objID = dynObj->objectID();

			constexpr auto EPSILON = 0.0001;

			auto rotationNotZero = rotationVec.x.asDouble() > EPSILON || rotationVec.y.asDouble() > EPSILON || rotationVec.z.asDouble() > EPSILON;
			auto scaleNotUniform = std::abs(scaleVec.x.asDouble() - scaleVec.y.asDouble()) > EPSILON || std::abs(scaleVec.x.asDouble() - scaleVec.z.asDouble()) > EPSILON;
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
				auto bgColor3Vec = bgColor3->asVec3f();
				Vec4f bgColor4Vec;
				bgColor4Vec.x = bgColor3Vec.x;
				bgColor4Vec.y = bgColor3Vec.y;
				bgColor4Vec.z = bgColor3Vec.z;
				bgColor4Vec.w = 1.0;
				dynObj->addProperty("backgroundColor", new Property<Vec4f, DisplayNameAnnotation>{bgColor4Vec, {"Display Background Color"}}, -1);
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
				auto settings = raco::core::PathManager::preferenceSettings();
				auto resourceFolders = new Property<raco::serialization::proxy::DefaultResourceDirectories, DisplayNameAnnotation>{{}, {"Default Resource Folders"}};
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
					if (prefab->query<raco::core::ExternalReferenceAnnotation>()) {
						for (size_t propIndex = 0; propIndex < dynObj->size(); propIndex++) {
							if (dynObj->get(propIndex)->query<raco::core::URIAnnotation>()) {
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
									LOG_WARNING(raco::log_system::DESERIALIZATION, "Rewrite URI property '{}.{}': '{}' -> '{}' in project '{}'",
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
			const Table& instMapTable = instance->get("mapToInstance")->asTable();

			for (size_t index = 0; index < instMapTable.size(); index++) {
				const Table& item = instMapTable.get(index)->asTable();
				auto prefabChild = item.get(0)->asRef();
				auto instChild = item.get(1)->asRef();
				auto instChildID = EditorObject::XorObjectIDs(prefabChild->objectID(), instance->objectID());
				instChild->objectID_ = instChildID;
			}

			auto dynInst = std::dynamic_pointer_cast<DynamicEditorObject>(instance);
			dynInst->removeProperty("mapToInstance");
		}
	}
}

}  // namespace raco::serialization