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
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class LuaScriptTest : public TestEnvironmentCore {};

TEST_F(LuaScriptTest, URI_setValidURI) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle m{script};
	ValueHandle m_uri{m.get("uri")};
	auto scriptPath = test_path().append("scripts/struct.lua").string();
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
	commandInterface.set(uriHandle, test_path().append("scripts/struct.lua").string());
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_compileError) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	ValueHandle uriHandle{ script, { "uri" }};
	
	commandInterface.set(uriHandle, test_path().append("scripts/compile-error.lua").string());

	EXPECT_TRUE(commandInterface.errors().hasError(script));
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_trimFront) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	const auto FILE_NAME = test_path().append("scripts/compile-error.lua").string();
	ValueHandle uriHandle{ValueHandle{script, {"uri"}}};
	commandInterface.set(uriHandle, "  " + FILE_NAME);
	EXPECT_EQ(uriHandle.asString(), FILE_NAME);
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(LuaScriptTest, URI_setValidURI_trimBack) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName)};
	const auto FILE_NAME = test_path().append("scripts/compile-error.lua").string();
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
	auto scriptPath = test_path().append("scripts/struct.lua").string();
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

TEST_F(LuaScriptTest, errorInterfaceMissing) {
	auto script{commandInterface.createObject(LuaScript::typeDescription.typeName, "myScript")};
	TextFile scriptRunOnlyFile = makeFile("script1.lua", R"(
function run()
end
)");
	TextFile scriptInterfaceOnlyFile = makeFile("script2.lua", R"(
function interface()
end
)");
	TextFile scriptWhitespaceOnlyFile = makeFile("script3.lua", " ");
	TextFile scriptEmptyFile = makeFile("script4.lua", "");

	ValueHandle uriHandle{ValueHandle{script, {"uri"}}};
	commandInterface.set(uriHandle, scriptRunOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(script));
	ASSERT_EQ(commandInterface.errors().getError(script).message(), "[myScript] No 'interface' function defined!");
	commandInterface.set(uriHandle, scriptInterfaceOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(script));
	ASSERT_EQ(commandInterface.errors().getError(script).message(), "[myScript] No 'run' function defined!");
	commandInterface.set(uriHandle, scriptWhitespaceOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(script));
	ASSERT_EQ(commandInterface.errors().getError(script).message(), "[myScript] No 'interface' function defined!");
	commandInterface.set(uriHandle, scriptEmptyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(script));
	ASSERT_EQ(commandInterface.errors().getError(script).message(), "[myScript] No 'interface' function defined!");
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
	local FloatPair = { a = FLOAT, b = FLOAT }
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
	commandInterface.set({lua, {"uri"}}, (test_path() / "scripts/struct.lua").string());

	commandInterface.set({lua, {"luaInputs", "struct", "a"}}, 2.0);
	ValueHandle inputs{lua, {"luaInputs"}};

	commandInterface.set({lua, {"uri"}}, (test_path() / "scripts/nosuchfile.lua").string());
	ASSERT_EQ(inputs.size(), 0);

	commandInterface.set({lua, {"uri"}}, (test_path() / "scripts/struct.lua").string());
	ASSERT_EQ(inputs.size(), 1);
	ValueHandle in_struct = inputs.get("struct");
	ASSERT_TRUE(in_struct);
	ASSERT_EQ(in_struct.get("a").asDouble(), 2.0);
}


TEST_F(LuaScriptTest, properties_are_correctly_sorted) {
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
function interface()
	IN.zz = INT
	IN.aa = FLOAT

	OUT.zz = INT
	OUT.aa = FLOAT
end

function run()
end

)");
	commandInterface.set(s.get("uri"), scriptFile);

	ASSERT_EQ(newScript->luaInputs_->propertyNames(), std::vector<std::string>({"aa", "zz"}));
	ASSERT_EQ(newScript->luaOutputs_->propertyNames(), std::vector<std::string>({"aa", "zz"}));

	auto scriptFile2 = makeFile("script2.lua", R"(
function interface()
	IN.zz = INT
	IN.aa = FLOAT
	IN.abc = INT
	IN.za = STRING
	IN.ff = BOOL

	OUT.zz = INT
	OUT.aa = FLOAT
	OUT.zzz = FLOAT
	OUT.e = STRING
end

function run()
end

)");

	commandInterface.set(s.get("uri"), scriptFile2);
	ASSERT_EQ(newScript->luaInputs_->propertyNames(), std::vector<std::string>({"aa", "abc", "ff", "za", "zz"}));
	ASSERT_EQ(newScript->luaOutputs_->propertyNames(), std::vector<std::string>({"aa", "e", "zz", "zzz"}));
}

TEST_F(LuaScriptTest, modules_in_uri_are_rejected) {
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	commandInterface.set(s.get("uri"), test_path().append("scripts/moduleDefinition.lua").string());

	ASSERT_TRUE(commandInterface.errors().hasError({newScript}));
	ASSERT_EQ(commandInterface.errors().getError({newScript}).level(), raco::core::ErrorLevel::ERROR);
	ASSERT_EQ(newScript->luaModules_->size(), 0);
}


TEST_F(LuaScriptTest, module_loaded_without_assigned_module_objects) {
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("coalas", "module", "anothermodule")

function interface()
	IN.zz = INT
	IN.aa = FLOAT
	IN.abc = INT
	IN.za = STRING
	IN.ff = BOOL

	OUT.zz = INT
	OUT.aa = FLOAT
	OUT.zzz = FLOAT
	OUT.e = STRING
end

function run()
end
)");
	commandInterface.set(s.get("uri"), scriptFile);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));
	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas", "module", "anothermodule"}));
	ASSERT_TRUE(newScript->luaInputs_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaOutputs_->propertyNames().empty());
}

TEST_F(LuaScriptTest, module_loaded_after_assigned_module_objects) {
	std::array<SLuaScriptModule, 3> modules = { create<LuaScriptModule>("m1"),
		create<LuaScriptModule>("m2"),
		create<LuaScriptModule>("m3") };
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("coalas", "module", "anothermodule")

function interface()
	IN.zz = INT
	IN.aa = FLOAT
	IN.abc = INT
	IN.za = STRING
	IN.ff = BOOL

	OUT.zz = INT
	OUT.aa = FLOAT
	OUT.zzz = FLOAT
	OUT.e = STRING
end

function run()
end
)");
	auto moduleFile1  = makeFile("module1.lua", R"(
local coalaModule = {}

return coalaModule
)");

	auto moduleFile2 = makeFile("module2.lua", R"(
local mymodule = {}

return mymodule
)");

	auto moduleFile3 = makeFile("module3.lua", R"(
local anothermodule = {}

return anothermodule
)");

	commandInterface.set(s.get("uri"), scriptFile);
	commandInterface.set(s.get("luaModules").get("coalas"), modules[0]);
	commandInterface.set(s.get("luaModules").get("module"), modules[1]);
	commandInterface.set(s.get("luaModules").get("anothermodule"), modules[2]);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));

	commandInterface.set({modules[0], &raco::user_types::LuaScriptModule::uri_}, moduleFile1);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));

	commandInterface.set({modules[1], &raco::user_types::LuaScriptModule::uri_}, moduleFile2);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));

	commandInterface.set({modules[2], &raco::user_types::LuaScriptModule::uri_}, moduleFile3);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));

	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas", "module", "anothermodule"}));
	ASSERT_EQ(newScript->luaInputs_->propertyNames(), std::vector<std::string>({"aa", "abc", "ff", "za", "zz"}));
	ASSERT_EQ(newScript->luaOutputs_->propertyNames(), std::vector<std::string>({"aa", "e", "zz", "zzz"}));

	commandInterface.set({modules[2], &raco::user_types::LuaScriptModule::uri_}, std::string());

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("module")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("anothermodule")));

	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas", "module", "anothermodule"}));
	ASSERT_TRUE(newScript->luaInputs_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaOutputs_->propertyNames().empty());
}

TEST_F(LuaScriptTest, module_caching) {
	std::array<SLuaScriptModule, 3> modules = {create<LuaScriptModule>("m1"),
		create<LuaScriptModule>("m2"),
		create<LuaScriptModule>("m3")};
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	auto scriptFile1 = makeFile("script1.lua", R"(
modules("coalas", "module", "anothermodule")

function interface()
end

function run()
end
)");

	auto scriptFile2 = makeFile("script2.lua", R"(
modules("coalas")

function interface()
end

function run()
end
)");

	auto scriptFile3 = makeFile("script3.lua", R"(
modules("coalas", "module", "anothermodule", "fourthmodule")

function interface()
end

function run()
end
)");

	auto moduleFile1 = makeFile("module1.lua", R"(
local coalaModule = {}

return coalaModule
)");

	auto moduleFile2 = makeFile("module2.lua", R"(
local mymodule = {}

return mymodule
)");

	auto moduleFile3 = makeFile("module3.lua", R"(
local anothermodule = {}

return anothermodule
)");

	commandInterface.set(s.get("uri"), scriptFile1);
	commandInterface.set(s.get("luaModules").get("coalas"), modules[0]);
	commandInterface.set(s.get("luaModules").get("module"), modules[1]);
	commandInterface.set(s.get("luaModules").get("anothermodule"), modules[2]);
	commandInterface.set({modules[0], &raco::user_types::LuaScriptModule::uri_}, moduleFile1);
	commandInterface.set({modules[1], &raco::user_types::LuaScriptModule::uri_}, moduleFile2);
	commandInterface.set({modules[2], &raco::user_types::LuaScriptModule::uri_}, moduleFile3);

	commandInterface.set(s.get("uri"), scriptFile2);
	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas"}));
	ASSERT_EQ(s.get("luaModules").get("coalas").asRef(), modules[0]);

	commandInterface.set(s.get("uri"), scriptFile3);
	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas", "module", "anothermodule", "fourthmodule"}));
	ASSERT_EQ(s.get("luaModules").get("coalas").asRef(), modules[0]);
	ASSERT_EQ(s.get("luaModules").get("module").asRef(), modules[1]);
	ASSERT_EQ(s.get("luaModules").get("anothermodule").asRef(), modules[2]);
	ASSERT_EQ(s.get("luaModules").get("fourthmodule").asRef(), nullptr);
}

TEST_F(LuaScriptTest, module_loaded_with_redeclaration_of_standard_lua_module) {
	auto module = create<LuaScriptModule>("m");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("math", "coalas")

function interface()
	IN.zz = INT
	IN.aa = FLOAT
	IN.abc = INT
	IN.za = STRING
	IN.ff = BOOL

	OUT.zz = INT
	OUT.aa = FLOAT
	OUT.zzz = FLOAT
	OUT.e = STRING
end

function run()
end
)");

	auto scriptFile2 = makeFile("script2.lua", R"(
modules("coalas")

function interface()
	IN.zz = INT
	IN.aa = FLOAT
	IN.abc = INT
	IN.za = STRING
	IN.ff = BOOL

	OUT.zz = INT
	OUT.aa = FLOAT
	OUT.zzz = FLOAT
	OUT.e = STRING
end

function run()
end
)");

	auto moduleFile = makeFile("module.lua", R"(
local coalaModule = {}

return coalaModule
)");

	commandInterface.set(raco::core::ValueHandle{module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	commandInterface.set(scriptUri, scriptFile);
	ASSERT_TRUE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(newScript->luaModules_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaInputs_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaOutputs_->propertyNames().empty());

	commandInterface.set(scriptUri, scriptFile2);
	commandInterface.set(s.get("luaModules").get("coalas"), module);
	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_EQ(newScript->luaModules_->propertyNames(), std::vector<std::string>({"coalas"}));
	ASSERT_EQ(newScript->luaInputs_->propertyNames(), std::vector<std::string>({"aa", "abc", "ff", "za", "zz"}));
	ASSERT_EQ(newScript->luaOutputs_->propertyNames(), std::vector<std::string>({"aa", "e", "zz", "zzz"}));

	commandInterface.set(scriptUri, scriptFile);
	ASSERT_TRUE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(newScript->luaModules_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaInputs_->propertyNames().empty());
	ASSERT_TRUE(newScript->luaOutputs_->propertyNames().empty());
}

TEST_F(LuaScriptTest, module_amount_to_zero) {
	std::array<SLuaScriptModule, 3> modules = {create<LuaScriptModule>("m1"),
		create<LuaScriptModule>("m2"),
		create<LuaScriptModule>("m3")};
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle uri{s.get("uri")};

	auto scriptFile1 = makeFile("script1.lua", R"(
modules("coalas", "module", "anothermodule")

function interface()
end

function run()
end
)");

	auto scriptFile2 = makeFile("script2.lua", R"(
function interface()
end

function run()
end
)");


	commandInterface.set(uri, scriptFile1);

	commandInterface.set(uri, scriptFile2);
	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_EQ(newScript->luaModules_.asTable().size(), 0);
}

TEST_F(LuaScriptTest, module_error_messages_invalid_assigned) {
	auto moduleInvalid = create<LuaScriptModule>("invalid");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("test", "coalas")

function interface()	
end

function run()
end
)");

	auto moduleInvalidFile = makeFile("moduleInvalid.lua", R"(
local what = {}
error;
return what
)");

	commandInterface.set(scriptUri, scriptFile);
	commandInterface.set(raco::core::ValueHandle{moduleInvalid, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);
	commandInterface.set(s.get("luaModules").get("coalas"), moduleInvalid);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));

	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("coalas")).level(), raco::core::ErrorLevel::ERROR);
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("coalas")).message(), "Invalid LuaScriptModule 'invalid' assigned.");

	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("test")));
}

TEST_F(LuaScriptTest, module_error_messages_two_invalid_assigned) {
	auto moduleInvalid1 = create<LuaScriptModule>("invalid1");
	auto moduleInvalid2 = create<LuaScriptModule>("invalid2");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("test", "coalas")

function interface()	
end

function run()
end
)");

	auto moduleInvalidFile = makeFile("moduleInvalid.lua", R"(
local what = {}
error;
return what
)");

	commandInterface.set(scriptUri, scriptFile);
	commandInterface.set(raco::core::ValueHandle{moduleInvalid1, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);
	commandInterface.set(raco::core::ValueHandle{moduleInvalid2, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);
	commandInterface.set(s.get("luaModules").get("coalas"), moduleInvalid1);
	commandInterface.set(s.get("luaModules").get("test"), moduleInvalid2);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("coalas")).level(), raco::core::ErrorLevel::ERROR);
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("coalas")).message(), "Invalid LuaScriptModule 'invalid1' assigned.");

	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("test")));
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("test")).level(), raco::core::ErrorLevel::ERROR);
	ASSERT_EQ(commandInterface.errors().getError(s.get("luaModules").get("test")).message(), "Invalid LuaScriptModule 'invalid2' assigned.");
}

TEST_F(LuaScriptTest, module_error_messages_invalid_then_valid) {
	auto moduleValid = create<LuaScriptModule>("valid");
	auto moduleInvalid = create<LuaScriptModule>("invalid");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("test", "coalas")

function interface()	
end

function run()
end
)");

	auto moduleInvalidFile = makeFile("moduleInvalid.lua", R"(
local what = {}
error;
return what
)");

	auto moduleValidFile = makeFile("moduleValid.lua", R"(
local coalaModule = {}

return coalaModule
)");

	commandInterface.set(scriptUri, scriptFile);
	commandInterface.set(raco::core::ValueHandle{moduleValid, &raco::user_types::LuaScriptModule::uri_}, moduleValidFile);
	commandInterface.set(raco::core::ValueHandle{moduleInvalid, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);

	commandInterface.set(s.get("luaModules").get("coalas"), moduleValid);
	commandInterface.set(s.get("luaModules").get("test"), moduleValid);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("test")));

	// make invalid by switching LuaScriptModule
	commandInterface.set(s.get("luaModules").get("coalas"), moduleInvalid);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("test")));

	// make invalid by changing URIs
	commandInterface.set(raco::core::ValueHandle{moduleValid, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("test")));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
}

TEST_F(LuaScriptTest, module_error_messages_invalid_then_empty) {
	auto moduleValid = create<LuaScriptModule>("valid");
	auto moduleInvalid = create<LuaScriptModule>("invalid");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("test", "coalas")

function interface()	
end

function run()
end
)");

	auto moduleInvalidFile = makeFile("moduleInvalid.lua", R"(
local what = {}
error;
return what
)");

	auto moduleValidFile = makeFile("moduleValid.lua", R"(
local coalaModule = {}

return coalaModule
)");

	commandInterface.set(scriptUri, scriptFile);
	commandInterface.set(raco::core::ValueHandle{moduleValid, &raco::user_types::LuaScriptModule::uri_}, moduleValidFile);
	commandInterface.set(raco::core::ValueHandle{moduleInvalid, &raco::user_types::LuaScriptModule::uri_}, moduleInvalidFile);

	commandInterface.set(s.get("luaModules").get("coalas"), moduleInvalid);
	commandInterface.set(s.get("luaModules").get("test"), moduleValid);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("test")));

	commandInterface.set(s.get("luaModules").get("coalas"), SEditorObject());

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("coalas")));
	ASSERT_FALSE(commandInterface.errors().hasError(s.get("luaModules").get("test")));
}

TEST_F(LuaScriptTest, module_error_messages_no_module_uri) {
	auto moduleInvalid = create<LuaScriptModule>("invalid");
	auto newScript = create<LuaScript>("script");
	ValueHandle s{newScript};
	ValueHandle scriptUri{s.get("uri")};

	auto scriptFile = makeFile("script.lua", R"(
modules("test")

function interface()	
end

function run()
end
)");

	commandInterface.set(scriptUri, scriptFile);

	commandInterface.set(s.get("luaModules").get("test"), moduleInvalid);

	ASSERT_FALSE(commandInterface.errors().hasError({newScript}));
	ASSERT_TRUE(commandInterface.errors().hasError(s.get("luaModules").get("test")));
}

TEST_F(LuaScriptTest, module_invalid_module_statement) {
	auto script = create<LuaScript>("script");
	ValueHandle uriHandle{script, {"uri"}};

	auto scriptFile = makeFile("script.lua", R"(
modules("test")
function interface()	
end
function run()
end
)");
	auto scriptFile2 = makeFile("script2.lua", R"(
modules(123)
function interface()	
end
function run()
end
)");

	commandInterface.set(uriHandle, scriptFile);
	ASSERT_EQ(script->luaModules_->propertyNames(), std::vector<std::string>({"test"}));

	commandInterface.set(uriHandle, scriptFile2);
	ASSERT_EQ(script->luaModules_->size(), 0);
}
