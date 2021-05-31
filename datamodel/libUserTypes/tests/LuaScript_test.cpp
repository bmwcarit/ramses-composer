/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"
#include "user_types/LuaScript.h"
#include "utils/FileUtils.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class LuaScriptTest : public TestEnvironmentCore {};

TEST_F(LuaScriptTest, URI_setValidURI) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle m{script};
	ValueHandle m_uri{m.get("uri")};
	auto scriptPath = cwd_path().append("scripts/struct.lua").generic_string();
	commandInterface.set(m_uri, scriptPath);
	EXPECT_EQ(m_uri.asString(), scriptPath);
}

TEST_F(LuaScriptTest, URI_setInvalidURI_error) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle uriHandle{ script, { "uri" }};
	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_noError) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle uriHandle{ script, { "uri" }};
	commandInterface.set(uriHandle, cwd_path().append("scripts/struct.lua").string());
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_compileError) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle uriHandle{ script, { "uri" }};
	
	commandInterface.set(uriHandle, cwd_path().append("scripts/compile-error.lua").string());

	EXPECT_TRUE(commandInterface.errors().hasError(script));
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_trimFront) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	const auto FILE_NAME = cwd_path().append("scripts/compile-error.lua").generic_string();
	ValueHandle uriHandle{ValueHandle{script, {"uri"}}};
	commandInterface.set(uriHandle, "  " + FILE_NAME);
	EXPECT_EQ(uriHandle.asString(), FILE_NAME);
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_trimBack) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	const auto FILE_NAME = cwd_path().append("scripts/compile-error.lua").generic_string();
	ValueHandle uriHandle{ValueHandle{script, {"uri"}}};
	commandInterface.set(uriHandle, FILE_NAME + "    ");
	EXPECT_EQ(uriHandle.asString(), FILE_NAME);
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_sanitizeSlashes) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
#if (defined(__linux__))
	std::string unsanitizedPath{R"(Test/Folder////Blah//testData/folder5/stuff/mesh.gltf)"};
	std::string sanitizedPath{"Test/Folder/Blah/testData/folder5/stuff/mesh.gltf"};
#else
	std::string unsanitizedPath{R"(C:\\Test\\Folder\\\\Blah\testData\folder5/stuff/mesh.gltf)"};
	std::string sanitizedPath{"C:/Test/Folder/Blah/testData/folder5/stuff/mesh.gltf"};
#endif
	commandInterface.set({script, {"uri"}}, unsanitizedPath);

	ASSERT_EQ(raco::core::ValueHandle(script, {"uri"}).asString(), sanitizedPath);
}

TEST_F(LuaScriptTest, URI_sanitizeNetworkPath) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
#if (defined(__linux__))
	std::string unsanitizedPath{R"( /Network//Networkfolder   )"};
	std::string sanitizedPath{"/Network/Networkfolder"};
#else
	std::string unsanitizedPath{R"(/\Network\\Networkfolder   )"};
	std::string sanitizedPath{"//Network/Networkfolder"};
#endif
	commandInterface.set({script, {"uri"}}, unsanitizedPath);

	ASSERT_EQ(raco::core::ValueHandle(script, {"uri"}).asString(), sanitizedPath);
}

TEST_F(LuaScriptTest, inputs_are_correctly_built) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle s{script};
	ValueHandle uri{s.get("uri")};
	auto scriptPath = cwd_path().append("scripts/struct.lua").string();
	commandInterface.set(uri, scriptPath);
	
	ValueHandle luaInputs{s.get("luaInputs")};
	EXPECT_EQ(1, luaInputs.size());
	const auto structInput { luaInputs[0] };
	EXPECT_EQ("struct", structInput.getPropName());
	EXPECT_EQ(PrimitiveType::Table, structInput.type());
	EXPECT_EQ(2, structInput.size());
	EXPECT_EQ("a", structInput[0].getPropName());
	EXPECT_EQ(PrimitiveType::Double, structInput[0].type());
	EXPECT_EQ("b", structInput[1].getPropName());
	EXPECT_EQ(PrimitiveType::Double, structInput[1].type());
}

TEST_F(LuaScriptTest, arrayIsCorrectlyBuilt) {
	auto script = create<LuaScript>("foo");
	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.float_array = ARRAY(5, FLOAT)
	OUT.float_array = ARRAY(5, FLOAT)
end

function run()
	OUT.float_array = IN.float_array
end
)");

	TextFile scriptFile_2 = makeFile("script2.lua", R"(
function interface()
	IN.float_array = ARRAY(3, FLOAT)
	OUT.float_array = ARRAY(3, FLOAT)
end

function run()
	OUT.float_array = IN.float_array
end
)");

	commandInterface.set({script, {"uri"}}, scriptFile);
	
	ValueHandle luaInputs{script, {"luaInputs"}};
	EXPECT_EQ(1, luaInputs.size());
	const auto structInput { luaInputs[0] };
	EXPECT_EQ("float_array", structInput.getPropName());
	EXPECT_EQ(PrimitiveType::Table, structInput.type());
	EXPECT_EQ(5, structInput.size());

	EXPECT_EQ(script->luaInputs_->get("float_array")->asTable().propertyNames(), std::vector<std::string>({"1", "2", "3", "4", "5"}));

	commandInterface.set({script, {"uri"}}, scriptFile_2);

	EXPECT_EQ(script->luaInputs_->get("float_array")->asTable().propertyNames(), std::vector<std::string>({"1", "2", "3"}));
}

TEST_F(LuaScriptTest, outArrayOfStructs) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle s{script};
	ValueHandle uri{s.get("uri")};

	TextFile scriptFile = makeFile("script.lua" , R"(
function interface()
	FloatPair = { a = FLOAT, b = FLOAT }
	IN.array = ARRAY(5, FloatPair)
end

function run()
end

)");
	commandInterface.set(s.get("uri"), scriptFile);

	ValueHandle luaInputs{s.get("luaInputs")};
	EXPECT_EQ(1, luaInputs.size());
	ValueHandle array{luaInputs.get("array")};
	EXPECT_EQ(5, array.size());
	ValueHandle firstIndex { array[0] };
	EXPECT_EQ(2, firstIndex.size());
	EXPECT_EQ(PrimitiveType::Table, firstIndex.type());
	EXPECT_EQ(PrimitiveType::Double, firstIndex[0].type());
	EXPECT_EQ("a", firstIndex[0].getPropName());
	EXPECT_EQ(PrimitiveType::Double, firstIndex[1].type());
	EXPECT_EQ("b", firstIndex[1].getPropName());
}

TEST_F(LuaScriptTest, restore_cached_struct_member) {
	auto lua = create<LuaScript>("script");
	commandInterface.set({lua, {"uri"}}, (cwd_path() / "scripts/struct.lua").string());

	commandInterface.set({lua, {"luaInputs", "struct", "a"}}, 2.0);
	ValueHandle inputs{lua, {"luaInputs"}};

	commandInterface.set({lua, {"uri"}}, (cwd_path() / "scripts/nosuchfile.lua").string());
	ASSERT_EQ(inputs.size(), 0);

	commandInterface.set({lua, {"uri"}}, (cwd_path() / "scripts/struct.lua").string());
	ASSERT_EQ(inputs.size(), 1);
	ValueHandle in_struct = inputs.get("struct");
	ASSERT_TRUE(in_struct);
	ASSERT_EQ(in_struct.get("a").asDouble(), 2.0);
}
