/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Serialization.h"
#include "core/SerializationKeys.h"

#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "core/ExternalReferenceAnnotation.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Texture.h"

#include "utils/FileUtils.h"

#include <gtest/gtest.h>

using namespace raco::user_types;

struct DeserializationTest : public TestEnvironmentCore {
	DeserializationTest() : TestEnvironmentCore(&TestObjectFactory::getInstance()) {
	}

	serialization::ObjectDeserialization deserializeObject(const std::string& fileName) {
		return serialization::test_helpers::deserializeObject(utils::file::read(test_path() / "expectations" / fileName), *context.objectFactory());
	}

	std::optional<serialization::ObjectsDeserialization> deserializeObjects(const std::string& fileName) {
		return serialization::deserializeObjects(utils::file::read(test_path() / "expectations" / fileName), false, *context.objectFactory());
	}
};

TEST_F(DeserializationTest, deserializeNode) {
	auto result = serialization::test_helpers::deserializeObject(utils::file::read((test_path() / "expectations" / "Node.json").string()));

	ASSERT_EQ(user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(sNode->objectID(), "node_id");
	ASSERT_EQ(sNode->objectName(), "node");
	ASSERT_EQ(100.0, *sNode->scaling_->z.staticQuery<core::RangeAnnotation<double>>().max_);
}

TEST_F(DeserializationTest, deserializeNodeRotated) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "NodeRotated.json").string()));
	ASSERT_EQ(user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(*sNode->rotation_->x, 90.0);
	ASSERT_EQ(*sNode->rotation_->y, -90.0);
	ASSERT_EQ(*sNode->rotation_->z, 180.0);
}

TEST_F(DeserializationTest, deserializeNodeWithAnnotations) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "NodeWithAnnotations.json").string()));
	ASSERT_EQ(user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	auto anno = sNode->query<core::ExternalReferenceAnnotation>();
	ASSERT_TRUE(anno != nullptr);
	ASSERT_EQ(*anno->projectID_, std::string("base_id"));
}

TEST_F(DeserializationTest, deserializeMeshNodeWithMesh) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "MeshNodeWithMesh.json").string()));
	ASSERT_EQ(user_types::MeshNode::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SMeshNode sMeshNode{std::dynamic_pointer_cast<MeshNode>(result.object)};
	ASSERT_EQ(1, result.references.size());
	ASSERT_EQ(result.references.at(&sMeshNode->mesh_), "mesh_id");
	ASSERT_EQ("Material", sMeshNode->materials_->get("material")->asTable().get("material")->typeName());
}

TEST_F(DeserializationTest, deserializeNodeWithMeshNode) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "NodeWithChildMeshNode.json").string()));
	ASSERT_EQ(user_types::Node::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SNode sNode{std::dynamic_pointer_cast<Node>(result.object)};
	ASSERT_EQ(1, result.references.size());
}

TEST_F(DeserializationTest, deserializeMesh) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "Mesh.json").string()));
	ASSERT_EQ(user_types::Mesh::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SMesh sMesh{std::dynamic_pointer_cast<Mesh>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(1, sMesh->materialNames_->size());
}

TEST_F(DeserializationTest, deserializeLuaScript) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScript.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(0, sLuaScript->inputs_->size());
	ASSERT_EQ(0, sLuaScript->outputs_->size());
}

TEST_F(DeserializationTest, deserializeLuaScriptInStruct) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScriptInStruct.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	ASSERT_EQ(0, result.references.size());
	ASSERT_EQ(1, sLuaScript->inputs_->size());
	ASSERT_EQ(data_storage::PrimitiveType::Double, sLuaScript->inputs_->get(0)->asTable().get(0)->type());
	ASSERT_EQ(data_storage::PrimitiveType::Double, sLuaScript->inputs_->get(0)->asTable().get(1)->type());
	ASSERT_EQ(0, sLuaScript->outputs_->size());
}

TEST_F(DeserializationTest, deserializeLuaScriptInSpecificPropNames) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScriptSpecificPropNames.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
}

TEST_F(DeserializationTest, deserializeLuaScriptWithRefToUserTypeWithAnnotation) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	auto* property{sLuaScript->inputs_->get(sLuaScript->inputs_->index("ref"))};
	ASSERT_EQ("Texture::EngineTypeAnnotation", property->typeName());
	ASSERT_EQ(1, property->baseAnnotationPtrs().size());
	ASSERT_TRUE(property->dynamicQuery<user_types::EngineTypeAnnotation>() != nullptr);
	ASSERT_EQ(static_cast<int>(core::EnginePrimitive::TextureSampler2D), *property->query<user_types::EngineTypeAnnotation>()->engineType_);
}

TEST_F(DeserializationTest, deserializeLuaScriptWithURI) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScriptWithURI.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};
	auto* property{sLuaScript->inputs_->get(sLuaScript->inputs_->index("uri"))};
	ASSERT_EQ("String::URIAnnotation", property->typeName());
	ASSERT_EQ(1, property->baseAnnotationPtrs().size());
	ASSERT_TRUE(property->dynamicQuery<core::URIAnnotation>() != nullptr);
}

TEST_F(DeserializationTest, deserializeLuaScriptWithAnnotatedDouble) {
	auto result = serialization::test_helpers::deserializeObject(
		utils::file::read((test_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string()));
	ASSERT_EQ(user_types::LuaScript::typeDescription.typeName, result.object->getTypeDescription().typeName);
	SLuaScript sLuaScript{std::dynamic_pointer_cast<LuaScript>(result.object)};

	auto* property = sLuaScript->inputs_->get(sLuaScript->inputs_->index("double"));
	ASSERT_EQ("Double", *property->query<user_types::DisplayNameAnnotation>()->name_);
	ASSERT_EQ(-10.0, *property->query<user_types::RangeAnnotation<double>>()->min_);
	ASSERT_EQ(10.0, *property->query<user_types::RangeAnnotation<double>>()->max_);
}

TEST_F(DeserializationTest, deserializeVersionArray) {
	using serialization::ProjectDeserializationInfo;
	QJsonObject fakeProjectJSON;

	auto compareVersionValues = [this, &fakeProjectJSON](serialization::DeserializedVersion&& expectedRamsesVer, serialization::DeserializedVersion&& expectedRaCoVer) {
		QJsonDocument fakeProjectJSONFile(fakeProjectJSON);
		auto versionInfo = serialization::deserializeProjectVersionInfo(fakeProjectJSONFile);

		ASSERT_TRUE(versionInfo.ramsesVersion == expectedRamsesVer);
		ASSERT_TRUE(versionInfo.raCoVersion == expectedRaCoVer);
	};

	compareVersionValues(
		{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION},
		{ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION});

	fakeProjectJSON[serialization::keys::RAMSES_VERSION] = QJsonArray{1};
	fakeProjectJSON[serialization::keys::RAMSES_COMPOSER_VERSION] = QJsonArray{99, 1, 3, 5};

	compareVersionValues(
		{1, ProjectDeserializationInfo::NO_VERSION, ProjectDeserializationInfo::NO_VERSION},
		{99, 1, 3});
}

TEST_F(DeserializationTest, deserializeObjects_luaScriptLinkedToNode_outputsAreDeserialized) {
	auto result = serialization::deserializeObjects(
		utils::file::read((test_path() / "expectations" / "LuaScriptLinkedToNode.json").string()), false);
	ASSERT_TRUE(result.has_value());

	user_types::SLuaScript sScript{ raco::select<user_types::LuaScript>(result->objects)};
	ASSERT_EQ(3, sScript->outputs_->size());
}

TEST_F(DeserializationTest, deserializeObjects_luaScriptLinkedToNode) {
	auto result = serialization::deserializeObjects(
		utils::file::read((test_path() / "expectations" / "LuaScriptLinkedToNode.json").string()), false);
	ASSERT_TRUE(result.has_value());

	std::vector<core::SEditorObject> objects{};
	objects.reserve(result->objects.size());
	for (auto& i : result->objects) {
		objects.push_back(std::dynamic_pointer_cast<core::EditorObject>(i));
	}
	for (auto& ref : result->references) {
		*ref.first = *std::find_if(objects.begin(), objects.end(), [&ref](const core::SEditorObject& obj) {
			return obj->objectID() == ref.second;
		});
	}

	ASSERT_EQ(2, result->objects.size());
	ASSERT_EQ(1, result->links.size());
	ASSERT_EQ(2, result->references.size());

	auto sLink{std::dynamic_pointer_cast<core::Link>(result->links.at(0))};
	user_types::SLuaScript sLuaScript{raco::select<user_types::LuaScript>(result->objects)};
	user_types::SNode sNode{raco::select<user_types::Node>(result->objects)};

	core::PropertyDescriptor startProp {sLuaScript, {"outputs", "translation"}};
	EXPECT_EQ(startProp, sLink->startProp());
	core::PropertyDescriptor endProp{sNode, {"translation"}};
	EXPECT_EQ(endProp, sLink->endProp());

	std::set<std::string> refRootObjectIDs{"node_id", "lua_script_id"};
	EXPECT_EQ(result->rootObjectIDs, refRootObjectIDs);
}

TEST_F(DeserializationTest, deserializeArrays) {
	auto result = deserializeObjects("Arrays.json");
	ASSERT_TRUE(result.has_value());

	for (auto& ref : result->references) {
		*ref.first = *std::find_if(result->objects.begin(), result->objects.end(), [&ref](const core::SEditorObject& obj) {
			return obj->objectID() == ref.second;
		});
	}

	auto obj = raco::select<user_types::ObjectWithArrays>(result->objects);
	auto node_1 = raco::select<user_types::Node>(result->objects, "node_1");
	auto node_2 = raco::select<user_types::Node>(result->objects, "node_2");
	ASSERT_TRUE(obj != nullptr);
	ASSERT_TRUE(node_1 != nullptr);
	ASSERT_TRUE(node_2 != nullptr);

	auto& array = obj->table_->get("array")->asArray();
	auto& nested = obj->table_->get("nested")->asArray();
	EXPECT_EQ(array.size(), 2);
	EXPECT_EQ(nested.size(), 2);
	for (size_t outer = 0; outer < 2; outer++) {
		EXPECT_EQ(array.get(outer)->asDouble(), outer);
		EXPECT_EQ(**obj->array_double_->get(outer), outer);

		EXPECT_EQ(nested.get(outer)->asArray().size(), 3);
		for (size_t inner = 0; inner < 3; inner++) {
			EXPECT_EQ(**(*obj->array_array_double_->get(outer))->get(inner), outer + static_cast<double>(inner) / 10.0);
			EXPECT_EQ(nested.get(outer)->asArray().get(inner)->asDouble(), outer + static_cast<double>(inner) / 10.0);
		}
	}

	EXPECT_EQ(obj->array_ref_->size(), 2);
	EXPECT_EQ(**obj->array_ref_->get(0), node_1);
	EXPECT_EQ(**obj->array_ref_->get(1), node_2);

	auto& array_ref = obj->table_->get("array_ref")->asArray();
	EXPECT_EQ(array_ref.size(), 2);
	EXPECT_EQ(array_ref.get(0)->asRef(), node_1);
	EXPECT_EQ(array_ref.get(1)->asRef(), node_2);
}
