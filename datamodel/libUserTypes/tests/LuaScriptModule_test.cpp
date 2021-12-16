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

#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class LuaScriptModuleTest : public TestEnvironmentCore {};

TEST_F(LuaScriptModuleTest, URI_setValidURI) {
	auto module{commandInterface.createObject(LuaScriptModule::typeDescription.typeName)};
	ValueHandle m{module};
	ValueHandle m_uri{m.get("uri")};
	commandInterface.set(m_uri, cwd_path().append("scripts/moduleDefinition.lua").string());
	ASSERT_EQ(m_uri.asString(), cwd_path().append("scripts/moduleDefinition.lua").generic_string());
}

TEST_F(LuaScriptModuleTest, URI_emptyURI_error) {
	auto module{commandInterface.createObject(LuaScriptModule::typeDescription.typeName)};
	ValueHandle uriHandle{module, {"uri"}};

	ASSERT_TRUE(commandInterface.errors().hasError(uriHandle));
	ASSERT_EQ(commandInterface.errors().getError(uriHandle).level(), raco::core::ErrorLevel::WARNING);
}

TEST_F(LuaScriptModuleTest, URI_setInvalidURI_error) {
	auto module{commandInterface.createObject(LuaScriptModule::typeDescription.typeName)};
	ValueHandle uriHandle{module, {"uri"}};
	commandInterface.set(uriHandle, std::string("blah.lua"));

	ASSERT_TRUE(commandInterface.errors().hasError(uriHandle));
	ASSERT_EQ(commandInterface.errors().getError(uriHandle).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(LuaScriptModuleTest, URI_setValidURI_noError) {
	auto module{commandInterface.createObject(LuaScriptModule::typeDescription.typeName)};
	ValueHandle uriHandle{module, {"uri"}};
	commandInterface.set(uriHandle, cwd_path().append("scripts/moduleDefinition.lua").string());

	ASSERT_FALSE(commandInterface.errors().hasError(uriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(module));
}

TEST_F(LuaScriptModuleTest, URI_setValidURI_noModules_error) {
	auto module{commandInterface.createObject(LuaScriptModule::typeDescription.typeName)};
	ValueHandle uriHandle{module, {"uri"}};
	commandInterface.set(uriHandle, cwd_path().append("scripts/struct.lua").string());

	ASSERT_TRUE(commandInterface.errors().hasError(module));
}