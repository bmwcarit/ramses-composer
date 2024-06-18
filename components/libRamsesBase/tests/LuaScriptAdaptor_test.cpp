/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
using ramses_adaptor::LuaScriptAdaptor;
using ramses_adaptor::propertyByNames;
using user_types::SLuaScript;
using user_types::LuaScript;

class LuaScriptAdaptorFixture : public RamsesBaseFixture<> {};

TEST_F(LuaScriptAdaptorFixture, defaultConstruction) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
    ASSERT_TRUE(engineObj == nullptr);
}

TEST_F(LuaScriptAdaptorFixture, validScript) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");
	
	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	dispatch();
	
	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), luaScript->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptAdaptorFixture, invalid_global) {
	TextFile scriptFile = makeFile("script.lua", R"(
nonsense
function interface(IN, OUT)
end
function run(IN,OUT)
end
)");
	auto script = create_lua("script", scriptFile);
	dispatch();

	EXPECT_TRUE(commandInterface.errors().hasError({script}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaScript>().size(), 0);
}

TEST_F(LuaScriptAdaptorFixture, invalid_error_in_interface) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
	error()
end
function run(IN,OUT)
end
)");
	auto script = create_lua("script", scriptFile);
	dispatch();

	EXPECT_TRUE(commandInterface.errors().hasError({script}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaScript>().size(), 0);
}

TEST_F(LuaScriptAdaptorFixture, innvalid_error_in_run) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto script = create_lua("script", scriptFile);
	dispatch();
	
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaScript>().size(), 1);
}

TEST_F(LuaScriptAdaptorFixture, nameChange) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	{
		auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
		ASSERT_TRUE(engineObj != nullptr);
		ASSERT_EQ(engineObj->getName(), "LuaScript Name");
		ASSERT_EQ(engineObj->getUserId(), luaScript->objectIDAsRamsesLogicID());
	}

	context.set({luaScript, {"objectName"}}, std::string("Changed"));
	dispatch();

	{
		auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "Changed")};
		ASSERT_TRUE(engineObj != nullptr);
		ASSERT_EQ(engineObj->getName(), "Changed");
		ASSERT_EQ(engineObj->getUserId(), luaScript->objectIDAsRamsesLogicID());
	}
}

TEST_F(LuaScriptAdaptorFixture, inInt) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Int32()
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value"}}, 5);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5, engineObj->getInputs()->getChild("value")->get<int>());
}

TEST_F(LuaScriptAdaptorFixture, inFloat) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Float()
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value"}}, 5.0);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getInputs()->getChild("value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inBool) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Bool()
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value"}}, true);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(true, engineObj->getInputs()->getChild("value")->get<bool>());
}

TEST_F(LuaScriptAdaptorFixture, inVec2f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Vec2f()
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value", "x" }}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getInputs()->getChild("value")->get<ramses::vec2f>().value()[0]);
	ASSERT_EQ(0.0f, engineObj->getInputs()->getChild("value")->get<ramses::vec2f>().value()[1]);
}

TEST_F(LuaScriptAdaptorFixture, inStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = {
		a = Type:Float()
	}
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value", "a"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value", "a")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inNestedStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	local s = { x = Type:Float(), y = Type:Float() }
	IN.value = {
		a = s
	}
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value", "a", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value", "a", "x")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, inVec4f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Vec4f()
end

function run(IN,OUT)
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getInputs(), "value")->get<ramses::vec4f>().value().x);
}


TEST_F(LuaScriptAdaptorFixture, outInt) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Int32()
	OUT.out_value = Type:Int32()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value"}}, 5);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5, engineObj->getOutputs()->getChild("out_value")->get<int>());
}

TEST_F(LuaScriptAdaptorFixture, outFloat) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Float()
	OUT.out_value = Type:Float()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value"}}, 5.0);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getOutputs()->getChild("out_value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outBool) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Bool()
	OUT.out_value = Type:Bool()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value"}}, true);
	dispatch();	

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(true, engineObj->getOutputs()->getChild("out_value")->get<bool>());
}

TEST_F(LuaScriptAdaptorFixture, outVec2f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Vec2f()
	OUT.out_value = Type:Vec2f()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value", "x" }}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, engineObj->getOutputs()->getChild("out_value")->get<ramses::vec2f>().value()[0]);
	ASSERT_EQ(0.0f,engineObj->getOutputs()->getChild("out_value")->get<ramses::vec2f>().value()[1]);
}

TEST_F(LuaScriptAdaptorFixture, outStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = {
		a = Type:Float()
	}

	OUT.out_value = {
		a = Type:Float()
	}
end

function run(IN,OUT)
	OUT.out_value.a = IN.in_value.a
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value", "a"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value", "a")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outNestedStruct) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	local s = { x = Type:Float(), y = Type:Float() }
	IN.in_value = {
		a = s
	}
	OUT.out_value = {
		b = s
	}
end

function run(IN,OUT)
	OUT.out_value.b = IN.in_value.a
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value", "a", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value", "b", "x")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, outVec4f) {
	auto luaScript = context.createObject(LuaScript::typeDescription.typeName, "LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.in_value = Type:Vec4f()
	OUT.out_value = Type:Vec4f()
end

function run(IN,OUT)
	OUT.out_value = IN.in_value
end

)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "in_value", "x"}}, 5.0f);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(5.0f, propertyByNames(engineObj->getOutputs(), "out_value")->get<ramses::vec4f>().value().x);
}


TEST_F(LuaScriptAdaptorFixture, keep_global_lua_state) {
	auto luaScript = create<LuaScript>("LuaScript Name");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
	IN.value = Type:Float()
	OUT.value = Type:Float()
end

function init()
	GLOBAL.counter = 0
end

function run(IN,OUT)
	GLOBAL.counter = GLOBAL.counter + 1
	OUT.value = IN.value + GLOBAL.counter
end
)");
	context.set({luaScript, {"uri"}}, uriPath);
	context.set({luaScript, {"inputs", "value"}}, 5.0);
	dispatch();

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name")};
	ASSERT_EQ(6.0f, engineObj->getOutputs()->getChild("value")->get<float>());

	context.set({luaScript, {"inputs", "value"}}, 2.0);
	dispatch();
	ASSERT_EQ(4.0f, engineObj->getOutputs()->getChild("value")->get<float>());
}

TEST_F(LuaScriptAdaptorFixture, prefab_move_out) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile, prefab);
	dispatch();

	dontFindLogic<ramses::LuaScript>("script");

	commandInterface.moveScenegraphChildren({script}, {});
	dispatch();

	selectCheckLogic<ramses::LuaScript>(script);
}

TEST_F(LuaScriptAdaptorFixture, prefab_delete_then_move_out) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile, prefab);
	auto node = create<user_types::Node>("node");
	dispatch();

	selectCheck<ramses::Node>(node);
	dontFindLogic<ramses::LuaScript>("script");

	commandInterface.deleteObjects({node});
	dispatch();

	dontFind<ramses::Node>("node");
	dontFindLogic<ramses::LuaScript>("script");

	commandInterface.moveScenegraphChildren({script}, {});
	dispatch();

	dontFind<ramses::Node>("node");
	selectCheckLogic<ramses::LuaScript>(script);
}

TEST_F(LuaScriptAdaptorFixture, prefab_delete_and_move_out) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile, prefab);
	auto node = create<user_types::Node>("node");
	dispatch();

	selectCheck<ramses::Node>(node);
	dontFindLogic<ramses::LuaScript>("script");

	commandInterface.deleteObjects({node});
	commandInterface.moveScenegraphChildren({script}, {});
	dispatch();

	dontFind<ramses::Node>("node");
	selectCheckLogic<ramses::LuaScript>(script);
}

TEST_F(LuaScriptAdaptorFixture, prefab_move_in) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile);
	dispatch();

	selectCheckLogic<ramses::LuaScript>(script);

	commandInterface.moveScenegraphChildren({script}, prefab);
	dispatch();

	dontFindLogic<ramses::LuaScript>("script");
}

TEST_F(LuaScriptAdaptorFixture, prefab_delete_then_move_in) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile);
	auto node = create<user_types::Node>("node");
	dispatch();

	selectCheck<ramses::Node>(node);
	selectCheckLogic<ramses::LuaScript>(script);

	commandInterface.deleteObjects({node});
	dispatch();

	dontFind<ramses::Node>("node");
	selectCheckLogic<ramses::LuaScript>(script);

	commandInterface.moveScenegraphChildren({script}, prefab);
	dispatch();

	dontFind<ramses::Node>("node");
	dontFindLogic<ramses::LuaScript>("script");
}

TEST_F(LuaScriptAdaptorFixture, prefab_delete_and_move_in) {
	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN, OUT)
end
function run(IN,OUT)
	error()
end
)");
	auto prefab = create<user_types::Prefab>("prefab");
	auto script = create_lua("script", scriptFile);
	auto node = create<user_types::Node>("node");
	dispatch();

	selectCheck<ramses::Node>(node);
	selectCheckLogic<ramses::LuaScript>(script);

	commandInterface.deleteObjects({node});
	commandInterface.moveScenegraphChildren({script}, prefab);
	dispatch();

	dontFind<ramses::Node>("node");
	dontFindLogic<ramses::LuaScript>("script");
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_moving) {
	auto luaScriptTopLevel = create<LuaScript>("LuaScript Name");
	auto luaScriptChild = create<LuaScript>("Child LuaScript Name");
	auto node = create<user_types::Node>("Node");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
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

	auto engineObj{select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name")};
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "Child LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().front()->children_->asVector<core::SEditorObject>().front()->objectIDAsRamsesLogicID());

	commandInterface.moveScenegraphChildren({luaScriptTopLevel}, {});
	dispatch();

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), luaScriptTopLevel->objectIDAsRamsesLogicID());

	commandInterface.moveScenegraphChildren({luaScriptChild}, prefab);
	dispatch();

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());

	commandInterface.moveScenegraphChildren({luaScriptChild}, node);
	dispatch();

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.Child LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_updated) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
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

	auto engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_paste) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
end
)");
	commandInterface.set({prefabInst, {"template"}}, prefab);
	dispatch();

	commandInterface.set({luaScript, {"uri"}}, uriPath);
	dispatch();

	commandInterface.moveScenegraphChildren({luaScript}, prefab);
	dispatch();

	auto copiedObjs = commandInterface.copyObjects({prefabInst, luaScript}, false);
	auto pastedObjs = commandInterface.pasteObjects(copiedObjs);
	dispatch();

	auto engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance (1).LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), pastedObjs.front()->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());
}


TEST_F(LuaScriptAdaptorFixture, prefab_instance_top_level_script_engine_name_gets_changed_after_undo_redo) {
	auto luaScript = create<LuaScript>("LuaScript Name");
	auto prefab = create<user_types::Prefab>("Prefab");
	auto prefabInst = create<user_types::PrefabInstance>("PrefabInstance");

	std::string uriPath{(test_path() / "script.lua").string()};
	utils::file::write(uriPath, R"(
function interface(IN,OUT)
end

function run(IN,OUT)
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

	auto engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());

	commandInterface.undoStack().redo();
	dispatch();

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "New PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj != nullptr);
	ASSERT_EQ(engineObj->getUserId(), prefabInst->children_->asVector<core::SEditorObject>().back()->objectIDAsRamsesLogicID());

	engineObj = select<ramses::LuaScript>(sceneContext.logicEngine(), "PrefabInstance.LuaScript Name");
	ASSERT_TRUE(engineObj == nullptr);
}
