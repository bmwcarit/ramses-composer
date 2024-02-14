/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"

#include "user_types/LuaInterface.h"
#include "utils/FileUtils.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class LuaInterfaceTest : public TestEnvironmentCore {};

TEST_F(LuaInterfaceTest, uri_warning_empty_uri) {
	auto interface = create<LuaInterface>("interface");
	ValueHandle uriHandle{interface, &LuaInterface::uri_};

	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), core::ErrorLevel::WARNING);
}
	
TEST_F(LuaInterfaceTest, uri_error_set_invalid_uri) {
	auto interface = create<LuaInterface>("interface");

	ValueHandle uriHandle{interface, &LuaInterface::uri_};
	commandInterface.set(uriHandle, std::string("invalid"));

	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), core::ErrorLevel::ERROR);
}

TEST_F(LuaInterfaceTest, uri_error_compile_error) {
	auto interface = create<LuaInterface>("interface");

	auto interfaceFile = makeFile("interface.lua",
		R"___(
invalid interface definition
)___");

	ValueHandle uriHandle{interface, &LuaInterface::uri_};
	commandInterface.set(uriHandle, interfaceFile);

	EXPECT_TRUE(commandInterface.errors().hasError({interface}));
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));
}


TEST_F(LuaInterfaceTest, error_global) {
	TextFile interfaceFile = makeFile("interface.lua", R"(
nonsense
function interface(INOUT)
end
)");
	auto interface = create_lua_interface("script", interfaceFile);

	EXPECT_TRUE(commandInterface.errors().hasError({interface}));
}

TEST_F(LuaInterfaceTest, interface_using_modules) {
	TextFile interfaceFile = makeFile("interface.lua", R"(
modules("coalas")
function interface(INOUT)
end
)");
	auto interface = create_lua_interface("script", interfaceFile);

	EXPECT_FALSE(commandInterface.errors().hasError({interface}));
	ASSERT_TRUE(commandInterface.errors().hasError({interface, { "luaModules", "coalas" }}));
}

TEST_F(LuaInterfaceTest, error_in_interface) {
	TextFile interfaceFile = makeFile("interface.lua", R"(
function interface(INOUT)
error()
end
)");
	auto interface = create_lua_interface("script", interfaceFile);

	EXPECT_TRUE(commandInterface.errors().hasError({interface}));
}
TEST_F(LuaInterfaceTest, invalid_redeclare_std_module) {
	auto interfaceFile = makeFile("interface.lua",
		R"___(
modules("math")
function interface(INOUT)
end
)___");

	auto interface = create_lua_interface("lua", interfaceFile);
	EXPECT_TRUE(commandInterface.errors().hasError({interface}));

	commandInterface.set({interface, {"stdModules", "math"}}, false);
	EXPECT_TRUE(commandInterface.errors().hasError({interface}));
}

TEST_F(LuaInterfaceTest, valid_script) {
	auto interface = create<LuaInterface>("interface");

	auto interfaceFile = makeFile("interface.lua",
		R"___(
function interface(INOUT)
	INOUT.f = Type:Float()
end
)___");

	ValueHandle uriHandle{interface, &LuaInterface::uri_};
	commandInterface.set(uriHandle, interfaceFile);

	EXPECT_FALSE(commandInterface.errors().hasError({interface}));
	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));

	EXPECT_EQ(interface->inputs_->size(), 1);
	EXPECT_EQ(interface->inputs_->name(0), std::string("f"));
	EXPECT_EQ(interface->inputs_->get(0)->type(), PrimitiveType::Double);
}

TEST_F(LuaInterfaceTest, error_script_stdmodule_missing) {
	auto interface = create_lua_interface("lua", "scripts/interface-using-math.lua");

	EXPECT_FALSE(commandInterface.errors().hasError({interface}));

	commandInterface.set({interface, {"stdModules", "math"}}, false);
	EXPECT_TRUE(commandInterface.errors().hasError({interface}));

	commandInterface.set({interface, {"stdModules", "math"}}, true);
	commandInterface.set({interface, {"stdModules", "debug"}}, false);
	EXPECT_FALSE(commandInterface.errors().hasError({interface}));
}

TEST_F(LuaInterfaceTest, module_set_unset) {
	auto interface = create_lua_interface("lua", "scripts/interface-using-module.lua");
	auto module = create_lua_module("module", "scripts/moduleDefinition.lua");

	EXPECT_TRUE(commandInterface.errors().hasError({interface, {"luaModules", "coalas"}}));

	commandInterface.set({interface, {"luaModules", "coalas"}}, module);

	EXPECT_FALSE(commandInterface.errors().hasError({interface, {"luaModules", "coalas"}}));

	commandInterface.set({interface, {"luaModules", "coalas"}}, SEditorObject());
	EXPECT_TRUE(commandInterface.errors().hasError({interface, {"luaModules", "coalas"}}));
}

TEST_F(LuaInterfaceTest, module_set_invalid_module) {
	auto interfaceFile = makeFile("interface.lua", R"(
modules("coalas")
function interface(INOUT)	
end
)");

	auto moduleFile = makeFile("module.lua", R"(
local what = {}
error;
return what
)");

	auto interface = create_lua_interface("lua", interfaceFile);
	auto module = create_lua_module("module", moduleFile);

	ValueHandle modulesHandle(interface, &LuaInterface::luaModules_);
	commandInterface.set({interface, {"luaModules", "coalas"}}, module);

	ASSERT_FALSE(commandInterface.errors().hasError({interface}));
	ASSERT_TRUE(commandInterface.errors().hasError(modulesHandle.get("coalas")));
	ASSERT_EQ(commandInterface.errors().getError(modulesHandle.get("coalas")).level(), core::ErrorLevel::ERROR);
	ASSERT_EQ(commandInterface.errors().getError(modulesHandle.get("coalas")).message(), "Invalid LuaScriptModule 'module' assigned.");
}

TEST_F(LuaInterfaceTest, module_caching) {
	auto interfaceFile_1 = makeFile("interface1.lua",
		R"___(
modules("coalas")
function interface(INOUT)
end
)___");

	auto interfaceFile_2 = makeFile("interface2.lua",
		R"___(
modules("kangaroos")
function interface(INOUT)
end
)___");

	auto interface = create_lua_interface("lua", interfaceFile_1);
	auto module = create_lua_module("module", "scripts/moduleDefinition.lua");

	ValueHandle modulesHandle(interface, &LuaInterface::luaModules_);
	EXPECT_TRUE(modulesHandle.hasProperty("coalas"));
	EXPECT_EQ(modulesHandle.get("coalas").asRef(), SEditorObject());

	commandInterface.set({interface, {"luaModules", "coalas"}}, module);
	EXPECT_EQ(modulesHandle.get("coalas").asRef(), module);

	commandInterface.set({interface, &LuaInterface::uri_}, interfaceFile_2);
	EXPECT_FALSE(modulesHandle.hasProperty("coalas"));
	EXPECT_TRUE(modulesHandle.hasProperty("kangaroos"));
	EXPECT_EQ(modulesHandle.get("kangaroos").asRef(), SEditorObject());

	commandInterface.set({interface, &LuaInterface::uri_}, interfaceFile_1);
	EXPECT_TRUE(modulesHandle.hasProperty("coalas"));
	EXPECT_EQ(modulesHandle.get("coalas").asRef(), module);
	EXPECT_FALSE(modulesHandle.hasProperty("kangaroos"));
}
