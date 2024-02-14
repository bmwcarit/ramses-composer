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


#include "testing/MockUserTypes.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "core/BasicAnnotations.h"
#include "core/ExternalReferenceAnnotation.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Texture.h"
#include "utils/FileUtils.h"
#include "utils/u8path.h"
#include <gtest/gtest.h>

constexpr bool WRITE_RESULT{false};
#ifndef CMAKE_CURRENT_SOURCE_DIR
#define CMAKE_CURRENT_SOURCE_DIR "."
#endif

using namespace raco::utils;
using namespace raco::data_storage;


class SerializationTest : public TestEnvironmentCore {
public:
	SerializationTest() : TestEnvironmentCore(&TestObjectFactory::getInstance()) {
	}

	void assertFileContentEqual(const std::string &filePath, const std::string &deserializedFileContent) {
		auto expectedFileContent = file::read(filePath);

#if (defined(__linux__))
		expectedFileContent.erase(std::remove(expectedFileContent.begin(), expectedFileContent.end(), '\r'), expectedFileContent.end());
#endif

		ASSERT_EQ(expectedFileContent, deserializedFileContent);
	}
};

TEST_F(SerializationTest, serializeNode) {
	const auto sNode{std::make_shared<user_types::Node>("node", "node_id")};
	sNode->scaling_->z.staticQuery<core::RangeAnnotation<double>>().max_ = 100.0;
	auto result = serialization::test_helpers::serializeObject(sNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "Node.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "Node.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeRotated) {
	const auto sNode{context.createObject(user_types::Node::typeDescription.typeName, "node", "node_id")};
	commandInterface.set({sNode, {"rotation", "x"}}, 90.0);
	commandInterface.set({sNode, {"rotation", "y"}}, -90.0);
	commandInterface.set({sNode, {"rotation", "z"}}, 180.0);
	auto result = serialization::test_helpers::serializeObject(sNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeRotated.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "NodeRotated.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeWithAnnotations) {
	const auto sNode{std::make_shared<user_types::Node>("node", "node_id")};

	auto anno = std::make_shared<core::ExternalReferenceAnnotation>();
	sNode->addAnnotation(anno);
	anno->projectID_ = "base_id";

	auto result = serialization::test_helpers::serializeObject(sNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeWithAnnotations.json").string(), result);

	ASSERT_EQ(file::read((test_path() / "expectations" / "NodeWithAnnotations.json").string()), result);
}
TEST_F(SerializationTest, serializeMeshNode) {
	const auto sMeshNode{std::make_shared<user_types::MeshNode>("mesh_node", "mesh_node_id")};
	auto result = serialization::test_helpers::serializeObject(sMeshNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshNode.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "MeshNode.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshNodeWithMesh) {
	const auto sMeshNode{context.createObject(user_types::MeshNode::typeDescription.typeName, "mesh_node", "mesh_node_id")};
	const auto sMesh{context.createObject(user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(test_relative_path() / "testData" / "duck.glb").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMeshNode, {"mesh"}}, sMesh);

	auto result = serialization::test_helpers::serializeObject(sMeshNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshNodeWithMesh.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "MeshNodeWithMesh.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeWithChildMeshNode) {
	const auto sMeshNode{context.createObject(user_types::MeshNode::typeDescription.typeName, "mesh_node", "mesh_node_id")};
	const auto sNode{context.createObject(user_types::Node::typeDescription.typeName, "node", "node_id")};
	commandInterface.moveScenegraphChildren({sMeshNode}, sNode);
	auto result = serialization::test_helpers::serializeObject(sNode);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeWithChildMeshNode.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "NodeWithChildMeshNode.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScript) {
	const auto sLuaScript{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto result = serialization::test_helpers::serializeObject(sLuaScript);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScript.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScript.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInFloat) {
	const auto sLuaScript{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri { (test_relative_path() / "testData" / "in-float.lua").string() };
	commandInterface.set(core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = serialization::test_helpers::serializeObject(sLuaScript);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInFloat.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptInFloat.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInFloatArray) {
	const auto sLuaScript{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(test_relative_path() / "testData" / "in-float-array.lua").string()};
	commandInterface.set(core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = serialization::test_helpers::serializeObject(sLuaScript);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInFloatArray.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptInFloatArray.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInStruct) {
	const auto sLuaScript{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(test_relative_path() / "testData" / "in-struct.lua").string()};
	commandInterface.set(core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = serialization::test_helpers::serializeObject(sLuaScript);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInStruct.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptInStruct.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInSpecificPropNames) {
	const auto sLuaScript{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(test_relative_path() / "testData" / "in-specific-prop-names.lua").string()};
	commandInterface.set(core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = serialization::test_helpers::serializeObject(sLuaScript);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptSpecificPropNames.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptSpecificPropNames.json").string(), result);
}

TEST_F(SerializationTest, serializeMesh) {
	const auto sMesh{context.createObject(user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(test_relative_path() / "testData" / "duck.glb").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	auto result = serialization::test_helpers::serializeObject(sMesh);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "Mesh.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "Mesh.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshglTFSubmesh) {
	const auto sMesh{context.createObject(user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(test_relative_path() / "testData" / "ToyCar.gltf").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMesh, {"bakeMeshes"}}, false);
	commandInterface.set({sMesh, {"meshIndex"}}, 2);
	auto result = serialization::test_helpers::serializeObject(sMesh);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshGLTFSubmesh.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "MeshGLTFSubmesh.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshglTFBakedSubmeshes) {
	const auto sMesh{context.createObject(user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(test_relative_path() / "testData" / "ToyCar.gltf").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMesh, {"meshIndex"}}, 2);
	commandInterface.set({sMesh, {"bakeMeshes"}}, true);
	auto result = serialization::test_helpers::serializeObject(sMesh);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshGLTFBaked.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "MeshGLTFBaked.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithRefToUserTypeWithAnnotation) {
	const auto editorObject{context.createObject(user_types::LuaScript::typeDescription.typeName, "mesh", "mesh_id")};
	user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<user_types::LuaScript>(editorObject)};
	sLuaScript->inputs_->addProperty("ref", new data_storage::Property<user_types::STexture, user_types::EngineTypeAnnotation>({}, {core::EnginePrimitive::TextureSampler2D}));

	auto result = serialization::test_helpers::serializeObject(editorObject);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithURI) {
	const auto editorObject{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<user_types::LuaScript>(editorObject)};
	sLuaScript->inputs_->addProperty("uri", new data_storage::Property<std::string, user_types::URIAnnotation>("", {}));

	auto result = serialization::test_helpers::serializeObject(editorObject);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithURI.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptWithURI.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithAnnotatedDouble) {
	const auto editorObject{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<user_types::LuaScript>(editorObject)};
	sLuaScript->inputs_->addProperty("double", new data_storage::Property<double, user_types::DisplayNameAnnotation, user_types::RangeAnnotation<double>>({}, {"Double"}, {-10.0, 10.0}));

	auto result = serialization::test_helpers::serializeObject(editorObject);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeAndScript_withLink) {
	const auto editorObject{context.createObject(user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<user_types::LuaScript>(editorObject)};
	sLuaScript->inputs_->addProperty("double", new data_storage::Property<double, user_types::DisplayNameAnnotation, user_types::RangeAnnotation<double>>({}, {"Double"}, {-10.0, 10.0}));

	auto result = serialization::test_helpers::serializeObject(editorObject);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);
}

TEST_F(SerializationTest, serializeObjects_luaScriptLinkedToNode) {
	auto objs{raco::createLinkedScene(context, test_relative_path())};
	std::map<std::string, serialization::ExternalProjectInfo> externalProjectsMap;
	std::map<std::string, std::string> originFolders;

	auto result = serialization::serializeObjects(
		{std::get<0>(objs), std::get<1>(objs)}, 
		{std::get<0>(objs)->objectID(), std::get<1>(objs)->objectID()},
		{std::get<2>(objs)}, "", "", "", "", externalProjectsMap, originFolders, -1, false);
	if (WRITE_RESULT) file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptLinkedToNode.json").string(), result);

    assertFileContentEqual((test_path() / "expectations" / "LuaScriptLinkedToNode.json").string(), result);
}

TEST_F(SerializationTest, serializeArrays) {
	auto obj = context.createObject(user_types::ObjectWithArrays::typeDescription.typeName, "obj_name", "obj_id")->as<user_types::ObjectWithArrays>();
	const auto node_1{context.createObject(user_types::Node::typeDescription.typeName, "node_1", "node_1_id")->as<user_types::Node>()};
	const auto node_2{context.createObject(user_types::Node::typeDescription.typeName, "node_2", "node_2_id")->as<user_types::Node>()};

	auto array = std::make_unique<Value<Array<double>>>();
	auto nested = std::make_unique<Value<Array<Array<double>>>>();
	for (size_t outer = 0; outer < 2; outer++) {
		*(*array)->addProperty() = outer;

		auto& row = (*nested)->addProperty()->asArray();
		for (size_t inner = 0; inner < 3; inner++) {
			*row.addProperty() = outer + static_cast<double>(inner) / 10.0;
		}

		*obj->array_double_->addProperty() = outer;

		{
			auto& row = obj->array_array_double_->addProperty()->asArray();
			for (size_t inner = 0; inner < 3; inner++) {
				*row.addProperty() = outer + static_cast<double>(inner) / 10.0;
			}
		}

	}
	obj->table_->addProperty("array", std::move(array));
	obj->table_->addProperty("nested", std::move(nested));

	*obj->array_ref_->addProperty() = node_1;
	*obj->array_ref_->addProperty() = node_2;

	auto array_ref = std::make_unique<Value<Array<user_types::SNode>>>();
	*(*array_ref)->addProperty() = node_1;
	*(*array_ref)->addProperty() = node_2;
	obj->table_->addProperty("array_ref", std::move(array_ref));

	std::map<std::string, serialization::ExternalProjectInfo> externalProjectsMap;
	std::map<std::string, std::string> originFolders;
	auto result = serialization::serializeObjects(
		{obj, node_1, node_2},
		{obj->objectID(), node_1->objectID(), node_2->objectID()},
		{}, "", "", "", "", externalProjectsMap, originFolders, -1, false);

	if (WRITE_RESULT) {
		file::write((u8path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "Arrays.json").string(), result);
	}

	assertFileContentEqual((test_path() / "expectations" / "Arrays.json").string(), result);
}
