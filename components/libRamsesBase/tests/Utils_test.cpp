/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_base/Utils.h"
#include "RamsesBaseFixture.h"
#include <gtest/gtest.h>

using namespace raco::ramses_base;
using raco::core::EnginePrimitive;

class UtilsTest : public RamsesBaseFixture<> {};

TEST_F(UtilsTest, parseLuaScript_struct) {
	const std::string script = R"(
function interface()
	IN.struct = {
        a = FLOAT,
        b = FLOAT
    }
end

function run()
end
)";
	std::string error;
	raco::core::PropertyInterfaceList in;
	raco::core::PropertyInterfaceList out;
	parseLuaScript(backend.logicEngine(), script, in, out, error);

	EXPECT_EQ(1, in.size());
	const auto& structProperty = in.at(0);
	EXPECT_EQ("struct", structProperty.name);
	EXPECT_EQ(EnginePrimitive::Struct, structProperty.type);
	EXPECT_EQ(2, structProperty.children.size());

	const auto& a{structProperty.children.at(0)};
	EXPECT_EQ("a", a.name);
	EXPECT_EQ(EnginePrimitive::Double, a.type);
	EXPECT_EQ(0, a.children.size());

	const auto& b{structProperty.children.at(1)};
	EXPECT_EQ("b", b.name);
	EXPECT_EQ(EnginePrimitive::Double, b.type);
	EXPECT_EQ(0, b.children.size());
}

TEST_F(UtilsTest, parseLuaScript_arrayNT) {
	const std::string script = R"(
function interface()
	IN.vec = ARRAY(5, FLOAT)
end

function run()
end
)";
	std::string error;
	raco::core::PropertyInterfaceList in;
	raco::core::PropertyInterfaceList out;
	parseLuaScript(backend.logicEngine(), script, in, out, error);

	EXPECT_EQ(1, in.size());
	EXPECT_EQ(EnginePrimitive::Array, in.at(0).type);
	EXPECT_EQ(5, in.at(0).children.size());
	for (size_t i{0}; i < 5; i++) {
		EXPECT_EQ(EnginePrimitive::Double, in.at(0).children.at(i).type);
	}
}
