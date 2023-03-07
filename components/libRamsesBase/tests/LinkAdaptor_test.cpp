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
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "testing/TestUtil.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/LuaScript.h"

#include "utils/FileUtils.h"
#include "core/PropertyDescriptor.h"

using namespace raco;
using namespace raco::ramses_adaptor;
using namespace raco::user_types;

class LinkAdaptorFixture : public RamsesBaseFixture<> {
public:
	void checkRamsesTranslation(const ramses::Node& node, const std::array<float, 3>& value) {
		EXPECT_EQ(Translation::from(node), value);
	}

	void checkRamsesNodeTranslation(const std::string& name, const std::array<float, 3>& value) {
		auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), name.c_str());
		EXPECT_TRUE(ramsesNode != nullptr);
		checkRamsesTranslation(*ramsesNode, value);
	}
};

TEST_F(LinkAdaptorFixture, linkCreationOneLink) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (test_path() / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {0.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);

	context.set({luaScript, {"inputs", "x"}}, 5.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {5.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);
}

#if (!defined (__linux__))
// awaitPreviewDirty does not work in Linux as expected. See RAOS-692

TEST_F(LinkAdaptorFixture, linkWorksIfScriptContentChanges) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (test_path() / "lua_script.lua").string());
	auto link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {0.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);

	context.set({luaScript, {"inputs", "x"}}, 5.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {5.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
    OUT.translation = { IN.x, 5.0, 0.0 }
end
	)");

	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, luaScript));

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {5.0, 5.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);
}
#endif

TEST_F(LinkAdaptorFixture, linkUnlinkLink) {
	const auto luaScript{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
    OUT.translation = { IN.x, 0.0, 0.0 }
end
	)");
	context.set({luaScript, {"uri"}}, (test_path() / "lua_script.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());

	context.set({luaScript, {"inputs", "x"}}, 5.0);
	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {0.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 0);

	auto link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {5.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);

	context.removeLink(link->endProp());
	context.set({luaScript, {"inputs", "x"}}, 10.0);

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {5.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 0);

	link = context.addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});

	ASSERT_NO_FATAL_FAILURE(dispatch());
	checkRamsesNodeTranslation("node", {10.0, 0.0, 0.0});
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);
}

TEST_F(LinkAdaptorFixture, linkStruct) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto luaScriptIn{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_in", "lua_script_in_id")};
	raco::utils::file::write((test_path() / "lua_script_out.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	OUT.a = {
		a = Type:Float(),
		b = Type:Float()
	}
end
function run(IN,OUT)
    OUT.a.a = IN.x
    OUT.a.b = IN.x
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out.lua").string());
	raco::utils::file::write((test_path() / "lua_script_in.lua").string(), R"(
function interface(IN,OUT)
	OUT.a = Type:Float()
	OUT.b = Type:Float()
	IN.a = {
		a = Type:Float(),
		b = Type:Float()
	}
end
function run(IN,OUT)
    OUT.a = IN.a.a
    OUT.b = IN.a.b
end
)");
	context.set({luaScriptIn, {"uri"}}, (test_path() / "lua_script_in.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto link = context.addLink({luaScriptOut, {"outputs", "a"}}, {luaScriptIn, {"inputs","a"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
}

TEST_F(LinkAdaptorFixture, linkQuaternion) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out.lua").string(), R"(
function interface(IN,OUT)
	IN.x = Type:Vec4f()
	OUT.x = Type:Vec4f()
end
function run(IN,OUT)
    OUT.x = IN.x    
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "x"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"inputs", "x", "x"}}, 0.3);
	context.set({luaScriptOut, {"inputs", "x", "y"}}, -0.3);
	context.set({luaScriptOut, {"inputs", "x", "z"}}, 1.0);
	context.set({luaScriptOut, {"inputs", "x", "w"}}, -1.0);
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_EQ(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
	auto rotationProperty = nodeBinding->getInputs()->getChild("rotation");
	ASSERT_EQ(rotationProperty->getType(), rlogic::EPropertyType::Vec4f);
	auto rotationArray = rotationProperty->get<rlogic::vec4f>();
	ASSERT_EQ(rotationArray, rlogic::vec4f({0.3, -0.3, 1.0, -1.0}));
}

TEST_F(LinkAdaptorFixture, linkEulerAfterQuaternion) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out.lua").string(), R"(
function interface(IN,OUT)
	IN.vec4 = Type:Vec4f()
	IN.vec3 = Type:Vec3f()
	OUT.vec4 = Type:Vec4f()
	OUT.vec3 = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec4 = IN.vec4
    OUT.vec3 = IN.vec3
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec4"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"inputs", "vec4", "x"}}, 0.3);
	context.set({luaScriptOut, {"inputs", "vec4", "y"}}, -0.3);
	context.set({luaScriptOut, {"inputs", "vec4", "z"}}, 1.0);
	context.set({luaScriptOut, {"inputs", "vec4", "w"}}, -1.0);
	context.set({luaScriptOut, {"inputs", "vec3", "x"}}, 90.0);
	context.set({luaScriptOut, {"inputs", "vec3", "y"}}, 180.0);
	context.set({luaScriptOut, {"inputs", "vec3", "z"}}, 270.0);
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec3"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_NE(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
	auto rotationProperty = nodeBinding->getInputs()->getChild("rotation");
	ASSERT_EQ(rotationProperty->getType(), rlogic::EPropertyType::Vec3f);
	auto rotationArray = rotationProperty->get<rlogic::vec3f>();
	ASSERT_EQ(rotationArray, rlogic::vec3f({90, 180, 270}));
}

TEST_F(LinkAdaptorFixture, linkQuaternionAfterEuler) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out.lua").string(), R"(
function interface(IN,OUT)
	IN.vec4 = Type:Vec4f()
	IN.vec3 = Type:Vec3f()
	OUT.vec4 = Type:Vec4f()
	OUT.vec3 = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec4 = IN.vec4
    OUT.vec3 = IN.vec3
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec3"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"inputs", "vec4", "x"}}, 0.3);
	context.set({luaScriptOut, {"inputs", "vec4", "y"}}, -0.3);
	context.set({luaScriptOut, {"inputs", "vec4", "z"}}, 1.0);
	context.set({luaScriptOut, {"inputs", "vec4", "w"}}, -1.0);
	context.set({luaScriptOut, {"inputs", "vec3", "x"}}, 90.0);
	context.set({luaScriptOut, {"inputs", "vec3", "y"}}, 180.0);
	context.set({luaScriptOut, {"inputs", "vec3", "z"}}, 270.0);
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec4"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_EQ(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
	auto rotationProperty = nodeBinding->getInputs()->getChild("rotation");
	ASSERT_EQ(rotationProperty->getType(), rlogic::EPropertyType::Vec4f);
	auto rotationArray = rotationProperty->get<rlogic::vec4f>();
	ASSERT_EQ(rotationArray, rlogic::vec4f({0.3, -0.3, 1.0, -1.0}));
}

TEST_F(LinkAdaptorFixture, linkQuaternionToEulerByURIChange) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");

	raco::utils::file::write((test_path() / "lua_script_out2.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec3f()
	OUT.vec = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out2.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_NE(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
}

TEST_F(LinkAdaptorFixture, linkEulerToQuaternionByURIChange) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");

	raco::utils::file::write((test_path() / "lua_script_out2.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec3f()
	OUT.vec = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out2.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_EQ(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
}

TEST_F(LinkAdaptorFixture, linkQuaternionStaysAfterTranslationLinkRemoval) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
	IN.transl = Type:Vec3f()
	OUT.transl = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
	OUT.transl = IN.transl
end
)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec"}}, {node, {"rotation"}});
	context.addLink({luaScriptOut, {"outputs", "transl"}}, {node, {"translation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.removeLink(raco::core::ValueHandle{node, {"translation"}}.getDescriptor());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_EQ(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
}


TEST_F(LinkAdaptorFixture, linkQuaternionChangeToEulerAfterInvalidLinkIsValid) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)	
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
end

function run(IN,OUT)
	OUT.vec = IN.vec
end

)");

	raco::utils::file::write((test_path() / "lua_script_out1b.lua").string(), R"(
function interface(IN,OUT)	
	IN.vec_b = Type:Vec4f()
	OUT.vec_b = Type:Vec4f()
end

function run(IN,OUT)
	OUT.vec_b = IN.vec_b
end

)");

	raco::utils::file::write((test_path() / "lua_script_out2.lua").string(), R"(
function interface(IN,OUT)	
	IN.vec = Type:Vec3f()
	OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
	OUT.vec = IN.vec
end

)");
	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1b.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
	ASSERT_FALSE((*commandInterface.project()->links().begin())->isValid());
	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_NE(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out2.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
	ASSERT_TRUE((*commandInterface.project()->links().begin())->isValid());
	nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_NE(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
}

TEST_F(LinkAdaptorFixture, linkQuaternionChangeInvalidToValid) {
	const auto luaScriptOut{context.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script_out", "lua_script_out_id")};
	const auto node{context.createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};

	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)	
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
end

function run(IN,OUT)
	OUT.vec = IN.vec
end

)");

	raco::utils::file::write((test_path() / "lua_script_out1b.lua").string(), R"(
function interface(IN,OUT)	
	IN.vec_b = Type:Vec4f()
	OUT.vec_b = Type:Vec4f()
end

function run(IN,OUT)
	OUT.vec_b = IN.vec_b
end

)");


	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.addLink({luaScriptOut, {"outputs", "vec"}}, {node, {"rotation"}});
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1b.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
	ASSERT_FALSE((*commandInterface.project()->links().begin())->isValid());
	auto nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_NE(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);

	context.set({luaScriptOut, {"uri"}}, (test_path() / "lua_script_out1.lua").string());
	ASSERT_NO_FATAL_FAILURE(dispatch());
	ASSERT_TRUE(backend.logicEngine().update());
	ASSERT_TRUE((*commandInterface.project()->links().begin())->isValid());
	nodeBinding = backend.logicEngine().findByName<rlogic::RamsesNodeBinding>("node_NodeBinding");
	ASSERT_EQ(nodeBinding->getRotationType(), rlogic::ERotationType::Quaternion);
}

TEST_F(LinkAdaptorFixture, link_weak) {
	auto scriptFile = makeFile("lua_script.lua", R"(
function interface(IN,OUT)
	IN.x = Type:Float()
	IN.y = Type:Float()
	OUT.x = Type:Float()
	OUT.y = Type:Float()
end
function run(IN,OUT)
	OUT.x = IN.x + 1
	OUT.y = IN.y
end
	)");
	auto start = create_lua("start", scriptFile);
	auto end = create_lua("end", scriptFile);

	commandInterface.addLink({start, {"outputs", "x"}}, {end, {"inputs", "x"}}, false);
	commandInterface.addLink({end, {"outputs", "x"}}, {start, {"inputs", "y"}}, true);
	commandInterface.set({start, {"inputs", "x"}}, 2.0);

	ASSERT_TRUE(dispatch());
	
	auto startEngineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "start");
	auto endEngineObj = select<rlogic::LuaScript>(sceneContext.logicEngine(), "end");

	ASSERT_EQ(startEngineObj->getInputs()->getChild("x")->get<float>(), 2.0);
	ASSERT_EQ(startEngineObj->getInputs()->getChild("y")->get<float>(), 4.0);
	ASSERT_EQ(startEngineObj->getOutputs()->getChild("x")->get<float>(), 3.0);
	ASSERT_EQ(startEngineObj->getOutputs()->getChild("y")->get<float>(), 0.0);

	ASSERT_TRUE(dispatch());

	ASSERT_EQ(startEngineObj->getInputs()->getChild("x")->get<float>(), 2.0);
	ASSERT_EQ(startEngineObj->getInputs()->getChild("y")->get<float>(), 4.0);
	ASSERT_EQ(startEngineObj->getOutputs()->getChild("x")->get<float>(), 3.0);
	ASSERT_EQ(startEngineObj->getOutputs()->getChild("y")->get<float>(), 4.0);
}

TEST_F(LinkAdaptorFixture, prefab_move_in_out) {
	auto scriptFile = makeFile("lua_script.lua", R"(
function interface(IN,OUT)
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node");
	auto child = create<Node>("child", node);
	auto lua = create_lua("lua", scriptFile, node);

	commandInterface.addLink({lua, {"outputs", "v"}}, {child, {"translation"}});
	ASSERT_TRUE(dispatch());
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);

	commandInterface.moveScenegraphChildren({node}, prefab);
	ASSERT_TRUE(dispatch());
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 0);

	commandInterface.moveScenegraphChildren({node}, nullptr);
	ASSERT_TRUE(dispatch());
	EXPECT_EQ(backend.logicEngine().getPropertyLinks().size(), 1);
}
