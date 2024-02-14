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

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, invalidModule) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateLuaScript("script");
	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError(module));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 1);
	ASSERT_NE(select<ramses::LuaModule>(sceneContext.logicEngine(), "Module"), nullptr);
	ASSERT_EQ(select<ramses::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_unassign) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, std::string());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 0);
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_rename_obj) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");

	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	context.set({module, &LuaScriptModule::objectName_}, std::string("Changed"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 1);
	ASSERT_NE(select<ramses::LuaModule>(sceneContext.logicEngine(), "Changed"), nullptr);
	ASSERT_EQ(select<ramses::LuaModule>(sceneContext.logicEngine(), "Changed")->getUserId(), module->objectIDAsRamsesLogicID());
}

TEST_F(LuaScriptModuleAdaptorTest, validModule_variousLuaScripts_noModuleCopies) {
	auto module = context.createObject(LuaScriptModule::typeDescription.typeName, "Module");
	dispatch();

	auto moduleFile = generateModule("coalas");
	commandInterface.set({module, &user_types::LuaScriptModule::uri_}, moduleFile);
	dispatch();

	auto script1 = context.createObject(LuaScript::typeDescription.typeName, "Script1");
	auto script2 = context.createObject(LuaScript::typeDescription.typeName, "Script2");
	auto script3 = context.createObject(LuaScript::typeDescription.typeName, "Script3");
	auto scriptFile1 = generateLuaScript("script1");
	auto scriptFile2 = generateLuaScript("script2");
	auto scriptFile3 = generateLuaScript("script3");
	commandInterface.set({script1, &user_types::LuaScript::uri_}, scriptFile1);
	commandInterface.set({script2, &user_types::LuaScript::uri_}, scriptFile2);
	commandInterface.set({script3, &user_types::LuaScript::uri_}, scriptFile3);

	dispatch();

	commandInterface.set({script1, {"luaModules", "neededModule"}}, module);
	commandInterface.set({script2, {"luaModules", "neededModule"}}, module);
	commandInterface.set({script3, {"luaModules", "neededModule"}}, module);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::LuaModule>().size(), 1);
	ASSERT_EQ(select<ramses::LuaModule>(sceneContext.logicEngine(), "Module")->getUserId(), module->objectIDAsRamsesLogicID());
}
