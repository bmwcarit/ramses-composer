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
#include "ramses_adaptor/LuaScriptModuleAdaptor.h"

using namespace raco::user_types;

class LuaScriptModuleAdaptorTest : public RamsesBaseFixture<> {
protected:
	TextFile generateModule(const std::string& moduleName) {
		return makeFile(fmt::format("{}.lua", moduleName),
			"local " + moduleName + "Module = {}\n\nreturn " + moduleName + "Module");
	}

	TextFile generateLuaScript(const std::string& fileName) {
		return makeFile(fmt::format("{}.lua", fileName),
			R"(
modules("neededModule")

function interface(IN,OUT)	
end

function run(IN,OUT)
end
)");
	}
};

TEST_F(LuaScriptModuleAdaptorTest, defaultConstruction) {
	context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, invalidModule) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateLuaScript("script");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError(module));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_NE(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module"), nullptr);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_unassign) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_rename_obj) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	context.set({module, &LuaScriptModule::objectName_}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_NE(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Changed"), nullptr);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Changed")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_validLua_moduleAssigned_rename_obj) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script = context.createObject(LuaScript::typeDescription.typeName, "Script");
	auto scriptFile = generateLuaScript("script");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	commandInterface.set({script, {"luaModules", "neededModule"}}, module);
	dispatch();

	context.set({module, {"objectName"}}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_NE(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Changed"), nullptr);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Changed")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_validLua_moduleAssigned_delete_uri_noLuaModules) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script = context.createObject(LuaScript::typeDescription.typeName, "Script");
	auto scriptFile = generateLuaScript("script");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	commandInterface.set({script, {"luaModules", "neededModule"}}, module);
	dispatch();

	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_variousLuaScripts_noModuleCopies) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script1 = context.createObject(LuaScript::typeDescription.typeName, "Script1");
	auto script2 = context.createObject(LuaScript::typeDescription.typeName, "Script2");
	auto script3 = context.createObject(LuaScript::typeDescription.typeName, "Script3");
	auto scriptFile1 = generateLuaScript("script1");
	auto scriptFile2 = generateLuaScript("script2");
	auto scriptFile3 = generateLuaScript("script3");
	commandInterface.set({script1, &raco::user_types::LuaScript::uri_}, scriptFile1);
	commandInterface.set({script2, &raco::user_types::LuaScript::uri_}, scriptFile2);
	commandInterface.set({script3, &raco::user_types::LuaScript::uri_}, scriptFile3);

	dispatch();

	commandInterface.set({script1, {"luaModules", "neededModule"}}, module);
	commandInterface.set({script2, {"luaModules", "neededModule"}}, module);
	commandInterface.set({script3, {"luaModules", "neededModule"}}, module);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_invalidLuaScript_syntaxError) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script = context.createObject(LuaScript::typeDescription.typeName, "Script");
	auto scriptFile = generateLuaScript("script1");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);

	commandInterface.set({script, {"luaModules", "neededModule"}}, module);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 1);

	auto errorScriptFile = makeFile("error.lua",
		R"(
modules("neededModule")

function interface(IN,OUT)
error
end

function run(IN,OUT)
end
)");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, errorScriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);

	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_invalidLuaScript_runtimeError) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script = context.createObject(LuaScript::typeDescription.typeName, "Script");
	auto scriptFile = generateLuaScript("script1");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);

	commandInterface.set({script, {"luaModules", "neededModule"}}, module);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 1);

	auto errorScriptFile = makeFile("error.lua",
		R"(
modules("neededModule")

function interface(IN,OUT)
IN.val = Type:Vec3f()
OUT.val = Type:Int32()
end

function run(IN,OUT)
OUT.val = IN.val
end
)");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, errorScriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 1);

	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_invalidLuaScript_thenNoModuleAndValidScriptruntimeError) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script = context.createObject(LuaScript::typeDescription.typeName, "Script");
	auto scriptFile = generateLuaScript("script1");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);

	commandInterface.set({script, {"luaModules", "neededModule"}}, module);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 1);

	auto errorScriptFile = makeFile("error.lua",
		R"(
modules("neededModule")

function interface(IN,OUT)
IN.val = Type:Vec3f()
OUT.val = Type:Int32()
end

function run(IN,OUT)
OUT.val = IN.val
end
)");
	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, errorScriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 1);

	commandInterface.set({module, &raco::user_types::LuaScriptModule::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);


	commandInterface.set({script, &raco::user_types::LuaScript::uri_}, scriptFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaModule>().size(), 0);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaScript>().size(), 0);
}