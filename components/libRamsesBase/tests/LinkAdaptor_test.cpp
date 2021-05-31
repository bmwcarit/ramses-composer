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
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "testing/TestUtil.h"
#include "user_types/LuaScript.h"
#include "utils/FileUtils.h"
#include "core/PropertyDescriptor.h"

using namespace raco;
using raco::ramses_adaptor::LuaScriptAdaptor;
using raco::ramses_adaptor::propertyByNames;
using raco::user_types::LuaScript;
using raco::user_types::SLuaScript;

class LinkAdaptorFixture : public RamsesBaseFixture<> {};

TEST_F(LinkAdaptorFixture, linkCreationOneLink) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.x = FLOAT
	OUT.translation = VEC3F
end
function run()
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (cwd_path() / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScript, {"luaInputs", "x"}}, 5.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "node");
	float x, y, z;
	ramsesNode->getTranslation(x, y, z);
	ASSERT_EQ(5.0f, x);
	ASSERT_EQ(0.0f, y);
	ASSERT_EQ(0.0f, z);
}

#if (!defined (__linux__))
// TODO: Investigate this on Linux

TEST_F(LinkAdaptorFixture, linkWorksIfScriptContentChanges) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.x = FLOAT
	OUT.translation = VEC3F
end
function run()
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (cwd_path() / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScript, {"luaInputs", "x"}}, 5.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.x = FLOAT
	OUT.translation = VEC3F
end
function run()
    OUT.translation = { IN.x, 5.0, 0.0 }
end
	)");

	raco::awaitPreviewDirty(recorder, luaScript);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "node");
	float x, y, z;
	ramsesNode->getTranslation(x, y, z);
	ASSERT_EQ(5.0f, x);
	ASSERT_EQ(5.0f, y);
	ASSERT_EQ(0.0f, z);
}
#endif

TEST_F(LinkAdaptorFixture, linkUnlinkLink) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.x = FLOAT
	OUT.translation = VEC3F
end
function run()
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (cwd_path() / "lua_script.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	context.set({luaScript, {"luaInputs", "x"}}, 5.0);
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto link = context.addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	{
		auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "node");
		float x, y, z;
		ramsesNode->getTranslation(x, y, z);
		ASSERT_EQ(5.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}

	context.removeLink(link->endProp());
	context.set({luaScript, {"luaInputs", "x"}}, 10.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	{
		auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "node");
		float x, y, z;
		ramsesNode->getTranslation(x, y, z);
		ASSERT_EQ(5.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}

	link = context.addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	{
		auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "node");
		float x, y, z;
		ramsesNode->getTranslation(x, y, z);
		ASSERT_EQ(10.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}
}

TEST_F(LinkAdaptorFixture, linkStruct) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto luaScriptIn{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_in", "lua_script_in_id")};
	raco::utils::file::write((cwd_path() / "lua_script_out.lua").string(), R"(
function interface()
	IN.x = FLOAT
	OUT.a = {
		a = FLOAT,
		b = FLOAT
	}
end
function run()
    OUT.a.a = IN.x
    OUT.a.b = IN.x
end
)");
	context.set({luaScriptOut, {"uri"}}, (cwd_path() / "lua_script_out.lua").string());
	raco::utils::file::write((cwd_path() / "lua_script_in.lua").string(), R"(
function interface()
	OUT.a = FLOAT
	OUT.b = FLOAT
	IN.a = {
		a = FLOAT,
		b = FLOAT
	}
end
function run()
    OUT.a = IN.a.a
    OUT.b = IN.a.b
end
)");
	context.set({luaScriptIn, {"uri"}}, (cwd_path() / "lua_script_in.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto link = context.addLink({luaScriptOut, {"luaOutputs", "a"}}, {luaScriptIn, {"luaInputs","a"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
}
