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


#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "data_storage/BasicAnnotations.h"
#include "core/ExternalReferenceAnnotation.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/FileUtils.h"
#include "core/SerializationFunctions.h"
#include <gtest/gtest.h>

constexpr bool WRITE_RESULT{false};
#ifndef CMAKE_CURRENT_SOURCE_DIR
#define CMAKE_CURRENT_SOURCE_DIR "."
#endif

struct SerializationTest : public TestEnvironmentCore {
	void assertFileContentEqual(const std::string &filePath, const std::string &deserializedFileContent) {
		auto expectedFileContent = raco::utils::file::read(filePath);

#if (defined(__linux__))
        expectedFileContent.erase(std::remove(expectedFileContent.begin(), expectedFileContent.end(), '\r'), expectedFileContent.end());
#endif

		ASSERT_EQ(expectedFileContent, deserializedFileContent);
	}
};

TEST_F(SerializationTest, serializeNode) {
	const auto sNode{std::make_shared<raco::user_types::Node>("node", "node_id")};
	sNode->scale_->z.staticQuery<raco::data_storage::RangeAnnotation<double>>().max_ = 100.0;
	auto result = raco::serialization::serialize(sNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "Node.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "Node.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeRotated) {
	const auto sNode{commandInterface.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	commandInterface.set({sNode, {"rotation", "x"}}, 90.0);
	commandInterface.set({sNode, {"rotation", "y"}}, -90.0);
	commandInterface.set({sNode, {"rotation", "z"}}, 180.0);
	auto result = raco::serialization::serialize(sNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeRotated.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "NodeRotated.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeWithAnnotations) {
	const auto sNode{std::make_shared<raco::user_types::Node>("node", "node_id")};

	auto anno = std::make_shared<raco::core::ExternalReferenceAnnotation>();
	sNode->addAnnotation(anno);
	anno->projectID_ = "base_id";

	auto result = raco::serialization::serialize(sNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeWithAnnotations.json").string(), result);

	ASSERT_EQ(raco::utils::file::read((cwd_path() / "expectations" / "NodeWithAnnotations.json").string()), result);
}
TEST_F(SerializationTest, serializeMeshNode) {
	const auto sMeshNode{std::make_shared<raco::user_types::MeshNode>("mesh_node", "mesh_node_id")};
	auto result = raco::serialization::serialize(sMeshNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshNode.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "MeshNode.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshNodeWithMesh) {
	const auto sMeshNode{commandInterface.createObject(raco::user_types::MeshNode::typeDescription.typeName, "mesh_node", "mesh_node_id")};
	const auto sMesh{commandInterface.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(cwd_path_relative() / "testData" / "duck.glb").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMeshNode, {"mesh"}}, sMesh);

	auto result = raco::serialization::serialize(sMeshNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshNodeWithMesh.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "MeshNodeWithMesh.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeWithChildMeshNode) {
	const auto sMeshNode{commandInterface.createObject(raco::user_types::MeshNode::typeDescription.typeName, "mesh_node", "mesh_node_id")};
	const auto sNode{commandInterface.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	commandInterface.moveScenegraphChildren({sMeshNode}, sNode);
	auto result = raco::serialization::serialize(sNode);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "NodeWithChildMeshNode.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "NodeWithChildMeshNode.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScript) {
	const auto sLuaScript{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto result = raco::serialization::serialize(sLuaScript);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScript.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScript.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInFloat) {
	const auto sLuaScript{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri { (cwd_path_relative() / "testData" / "in-float.lua").string() };
	commandInterface.set(raco::core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = raco::serialization::serialize(sLuaScript);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInFloat.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptInFloat.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInFloatArray) {
	const auto sLuaScript{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(cwd_path_relative() / "testData" / "in-float-array.lua").string()};
	commandInterface.set(raco::core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = raco::serialization::serialize(sLuaScript);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInFloatArray.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptInFloatArray.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInStruct) {
	const auto sLuaScript{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(cwd_path_relative() / "testData" / "in-struct.lua").string()};
	commandInterface.set(raco::core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = raco::serialization::serialize(sLuaScript);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptInStruct.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptInStruct.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptInSpecificPropNames) {
	const auto sLuaScript{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	auto uri{(cwd_path_relative() / "testData" / "in-specific-prop-names.lua").string()};
	commandInterface.set(raco::core::ValueHandle{sLuaScript, {"uri"}}, uri);
	auto result = raco::serialization::serialize(sLuaScript);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptSpecificPropNames.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptSpecificPropNames.json").string(), result);
}

TEST_F(SerializationTest, serializeMesh) {
	const auto sMesh{commandInterface.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(cwd_path_relative() / "testData" / "duck.glb").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	auto result = raco::serialization::serialize(sMesh);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "Mesh.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "Mesh.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshglTFSubmesh) {
	const auto sMesh{commandInterface.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(cwd_path_relative() / "testData" / "ToyCar.gltf").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMesh, {"bakeMeshes"}}, false);
	commandInterface.set({sMesh, {"meshIndex"}}, 2);
	auto result = raco::serialization::serialize(sMesh);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshGLTFSubmesh.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "MeshGLTFSubmesh.json").string(), result);
}

TEST_F(SerializationTest, serializeMeshglTFBakedSubmeshes) {
	const auto sMesh{commandInterface.createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id")};
	auto uri{(cwd_path_relative() / "testData" / "ToyCar.gltf").string()};
	commandInterface.set({sMesh, {"uri"}}, uri);
	commandInterface.set({sMesh, {"meshIndex"}}, 2);
	commandInterface.set({sMesh, {"bakeMeshes"}}, true);
	auto result = raco::serialization::serialize(sMesh);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "MeshGLTFBaked.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "MeshGLTFBaked.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithRefToUserTypeWithAnnotation) {
	const auto editorObject{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "mesh", "mesh_id")};
	raco::user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<raco::user_types::LuaScript>(editorObject)};
	sLuaScript->luaInputs_->addProperty("ref", new raco::data_storage::Property<raco::user_types::SLuaScript, raco::user_types::DisplayNameAnnotation>({}, {"BLUBB"}));

	auto result = raco::serialization::serialize(editorObject);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptWithRefToUserTypeWithAnnotation.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithURI) {
	const auto editorObject{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	raco::user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<raco::user_types::LuaScript>(editorObject)};
	sLuaScript->luaInputs_->addProperty("uri", new raco::data_storage::Property<std::string, raco::user_types::URIAnnotation>("", {}));

	auto result = raco::serialization::serialize(editorObject);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithURI.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptWithURI.json").string(), result);
}

TEST_F(SerializationTest, serializeLuaScriptWithAnnotatedDouble) {
	const auto editorObject{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	raco::user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<raco::user_types::LuaScript>(editorObject)};
	sLuaScript->luaInputs_->addProperty("double", new raco::data_storage::Property<double, raco::user_types::DisplayNameAnnotation, raco::user_types::RangeAnnotation<double>>({}, {"Double"}, {-10.0, 10.0}));

	auto result = raco::serialization::serialize(editorObject);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);
}

TEST_F(SerializationTest, serializeNodeAndScript_withLink) {
	const auto editorObject{commandInterface.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	raco::user_types::SLuaScript sLuaScript{std::dynamic_pointer_cast<raco::user_types::LuaScript>(editorObject)};
	sLuaScript->luaInputs_->addProperty("double", new raco::data_storage::Property<double, raco::user_types::DisplayNameAnnotation, raco::user_types::RangeAnnotation<double>>({}, {"Double"}, {-10.0, 10.0}));

	auto result = raco::serialization::serialize(editorObject);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptWithAnnotatedDouble.json").string(), result);
}

TEST_F(SerializationTest, serializeObjects_luaScriptLinkedToNode) {
	auto objs{raco::createLinkedScene(*this)};
	std::map<std::string, raco::serialization::ExternalProjectInfo> externalProjectsMap;
	std::map<std::string, std::string> originFolders;

	auto result = raco::serialization::serialize<raco::core::SEditorObject, raco::core::SLink>(
		{std::get<0>(objs), std::get<1>(objs)}, 
		{std::get<0>(objs)->objectID(), std::get<1>(objs)->objectID()},
		{std::get<2>(objs)}, "", "", "", "", externalProjectsMap, originFolders);
	if (WRITE_RESULT) raco::utils::file::write((std::filesystem::path{CMAKE_CURRENT_SOURCE_DIR} / "expectations" / "LuaScriptLinkedToNode.json").string(), result);

    assertFileContentEqual((cwd_path() / "expectations" / "LuaScriptLinkedToNode.json").string(), result);
}
