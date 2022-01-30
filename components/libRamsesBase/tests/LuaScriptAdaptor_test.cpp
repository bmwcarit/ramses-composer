/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "user_types/LuaScript.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "utils/FileUtils.h"

using namespace raco;
using raco::ramses_adaptor::LuaScriptAdaptor;
using raco::ramses_adaptor::propertyByNames;
using raco::user_types::SLuaScript;
using raco::user_types::LuaScript;

class LuaScriptAdaptorFixture : public RamsesBaseFixture<> {};

TEST_F(LuaScriptAdaptorFixture, defaultConstruction) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
    ASSERT_TRUE(engineObj == nullptr);
}

TEST_F(LuaScriptAdaptorFixture, validScript) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");
	
	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	dispatch();
	
	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_TRUE(engineObj != nullptr);
}

TEST_F(LuaScriptAdaptorFixture, nameChange) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	{
		auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
		ASSERT_TRUE(engineObj != nullptr);
		ASSERT_EQ(engineObj->getName(), "LuaScript Name");
	}

	context.set({luaScript, {"objectName"}}, std::string("Changed"));
	dispatch();

	{
		auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "Changed")};
		ASSERT_TRUE(engineObj != nullptr);
		ASSERT_EQ(engineObj->getName(), "Changed");
	}
}

TEST_F(LuaScriptAdaptorFixture, inInt) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = INT
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value"}}, 5);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5, engineObj->getInputs()->getChild("value")->get<int>());
}

TEST_F(LuaScriptAdaptorFixture, inFloat) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value"}}, 5.0);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getInputs()->getChild("value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inBool) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = BOOL
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value"}}, true);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(true, engineObj->getInputs()->getChild("value")->get<bool>());
}

TEST_F(LuaScriptAdaptorFixture, inVec2f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = VEC2F
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value", "x" }}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getInputs()->getChild("value")->get<rlogic::vec2f>().value()[0]);
	ASSERT_EQ(0.0f, engineObj->getInputs()->getChild("value")->get<rlogic::vec2f>().value()[1]);
}

TEST_F(LuaScriptAdaptorFixture, inStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = {
		a = FLOAT
	}
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value", "a"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value", "a")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inNestedStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	s = { x = FLOAT, y = FLOAT }
	IN.value = {
		a = s
	}
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value", "a", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value", "a", "x")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inVec4f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = VEC4F
end

function run()
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value")->get<rlogic::vec4f>()->at(0));
}


TEST_F(LuaScriptAdaptorFixture, outInt) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = INT
	OUT.out_value = INT
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value"}}, 5);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5, engineObj->getOutputs()->getChild("out_value")->get<int>());
}

TEST_F(LuaScriptAdaptorFixture, outFloat) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = FLOAT
	OUT.out_value = FLOAT
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value"}}, 5.0);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getOutputs()->getChild("out_value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outBool) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = BOOL
	OUT.out_value = BOOL
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value"}}, true);
	dispatch();	

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(true, engineObj->getOutputs()->getChild("out_value")->get<bool>());
}

TEST_F(LuaScriptAdaptorFixture, outVec2f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = VEC2F
	OUT.out_value = VEC2F
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value", "x" }}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getOutputs()->getChild("out_value")->get<rlogic::vec2f>().value()[0]);
	ASSERT_EQ(0.0f,engineObj->getOutputs()->getChild("out_value")->get<rlogic::vec2f>().value()[1]);
}

TEST_F(LuaScriptAdaptorFixture, outStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = {
		a = FLOAT
	}

	OUT.out_value = {
		a = FLOAT
	}
end

function run()
	OUT.out_value.a = IN.in_value.a
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value", "a"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value", "a")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outNestedStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	s = { x = FLOAT, y = FLOAT }
	IN.in_value = {
		a = s
	}
	OUT.out_value = {
		b = s
	}
end

function run()
	OUT.out_value.b = IN.in_value.a
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value", "a", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value", "b", "x")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outVec4f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.in_value = VEC4F
	OUT.out_value = VEC4F
end

function run()
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "in_value", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value")->get<rlogic::vec4f>()->at(0));
}


TEST_F(LuaScriptAdaptorFixture, keep_global_lua_state) {
	auto luaScript = create<LuaScript>("LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
	OUT.value = FLOAT
end

counter = 0

function run()
	counter = counter + 1	
	OUT.value = IN.value + counter
end
)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"luaInputs", "value"}}, 5.0);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(6.0f, engineObj->getOutputs()->getChild("value")->get<float>());

	context.set({luaScript, {"luaInputs", "value"}}, 2.0);
	dispatch();
	ASSERT_EQ(4.0f, engineObj->getOutputs()->getChild("value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_moving) {
	auto luaScriptTopLevel = create<LuaScript>("LuaScript Name");
	auto luaScriptChild = create<LuaScript>("Child LuaScript Name");
	auto node = create<user_types::Node>("Node");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
	OUT.value = FLOAT
end

counter = 0

function run()
	counter = counter + 1
	OUT.value = IN.value + counter
end
)");
	commandInterface.set({prefabInst, {"template"}}, prefab);
	dispatch();

	commandInterface.set({luaScriptTopLevel, {"uri"}}, uriPath);
	commandInterface.set({luaScriptChild, {"uri"}}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({node}, prefab);
	commandInterface.moveScenegraphChildren({luaScriptChild}, node);
	commandInterface.moveScenegraphChildren({luaScriptTopLevel}, prefab);
	dispatch();

	auto engineObj{select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name")};
	ASSERT_TRUE(engineObj != nullptr);

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "Child LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);

	commandInterface.moveScenegraphChildren({luaScriptTopLevel}, {});
	dispatch();

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);

	commandInterface.moveScenegraphChildren({luaScriptChild}, prefab);
	dispatch();

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);

	commandInterface.moveScenegraphChildren({luaScriptChild}, node);
	dispatch();

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_updated) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
	OUT.value = FLOAT
end

counter = 0

function run()
	counter = counter + 1
	OUT.value = IN.value + counter
end
)");
	commandInterface.set({prefabInst, {"template"}}, prefab);
	dispatch();

	commandInterface.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({luaScript}, prefab);
	dispatch();

	commandInterface.set({prefabInst, {"objectName"}}, std::string("New PrefabInstance"));
	dispatch();

	auto engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_paste) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
	OUT.value = FLOAT
end

counter = 0

function run()
	counter = counter + 1
	OUT.value = IN.value + counter
end
)");
	commandInterface.set({prefabInst, {"template"}}, prefab);
	dispatch();

	commandInterface.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({luaScript}, prefab);
	dispatch();

	auto copiedObjs = commandInterface.copyObjects({prefabInst, luaScript}, false);
	commandInterface.pasteObjects(copiedObjs);
	dispatch();

	auto engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance (1).LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_undo_redo) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	raco::utils::file::write(uriPath, R"(
function interface()
	IN.value = FLOAT
	OUT.value = FLOAT
end

counter = 0

function run()
	counter = counter + 1
	OUT.value = IN.value + counter
end
)");
	commandInterface.set({prefabInst, {"template"}}, prefab);
	dispatch();

	commandInterface.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({luaScript}, prefab);
	dispatch();

	commandInterface.set({prefabInst, {"objectName"}}, std::string("New PrefabInstance"));
	dispatch();

	commandInterface.undoStack().undo();
	dispatch();

	auto engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);

	commandInterface.undoStack().redo();
	dispatch();

	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	engineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
}
