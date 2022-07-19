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
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), raco::core::ErrorLevel::WARNING);
}

TEST_F(LuaInterfaceTest, uri_error_set_invalid_uri) {
	auto interface = create<LuaInterface>("interface");

	ValueHandle uriHandle{interface, &LuaInterface::uri_};
	commandInterface.set(uriHandle, std::string("invalid"));

	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), raco::core::ErrorLevel::ERROR);
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
