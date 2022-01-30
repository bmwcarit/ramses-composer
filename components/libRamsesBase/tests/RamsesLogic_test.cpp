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

#include "ramses_base/Utils.h"
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/Property.h>
#include <ramses-logic/RamsesNodeBinding.h>
#include <ramses-framework-api/RamsesFramework.h>
#include <ramses-client-api/RamsesClient.h>
#include <ramses-client-api/Scene.h>
#include <ramses-client-api/Node.h>

#include <cmath>

/*
* Basic test for third_party dependiences: ramses-logic.
* Should be used to verify stability of dependency.
*/


TEST(ramses_logic, repeat_create_destroy_simple_script) {
	rlogic::LogicEngine logicEngine;

	std::string scriptText = R"(
function interface()
    OUT.out_float = FLOAT
end

function run()
end
)";
	// logicengine bug: crashes in debug build with 10000 iterations although ok with 100 iterations
	// TODO: remove the EXPECT_THROW once the logicengine bug has been fixed
#if (!defined(__linux__) && !defined(NDEBUG))
	EXPECT_THROW(
		for (unsigned i = 0; i < 10000; i++) {
			auto* script = logicEngine.createLuaScript(scriptText);
			ASSERT_TRUE(script != nullptr);
			ASSERT_TRUE(logicEngine.destroy(*script));
		},
		std::exception);
#else
	for (unsigned i = 0; i < 10000; i++) {
		auto* script = logicEngine.createLuaScript(scriptText);
		ASSERT_TRUE(script != nullptr);
		ASSERT_TRUE(logicEngine.destroy(*script));
	}
#endif
}


TEST(ramses_logic, relink_scriptToScript) {
	rlogic::LogicEngine logicEngine;
	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
    OUT.out_float = FLOAT
end

function run()
end
)");
	auto* inScript = logicEngine.createLuaScript(R"(
function interface()
    IN.in_float = FLOAT
end

function run()
end
)");

	auto* outFloat = outScript->getOutputs()->getChild(0);
	auto* inFloat = inScript->getInputs()->getChild(0);

	ASSERT_EQ("out_float", outFloat->getName());
	ASSERT_EQ("in_float", inFloat->getName());
	ASSERT_TRUE(logicEngine.update());

	ASSERT_TRUE(logicEngine.link(*outFloat, *inFloat));
	ASSERT_TRUE(logicEngine.update());

	ASSERT_TRUE(logicEngine.unlink(*outFloat, *inFloat));
	ASSERT_TRUE(logicEngine.update());

	ASSERT_TRUE(logicEngine.link(*outFloat, *inFloat));
	ASSERT_TRUE(logicEngine.update());
}

TEST(ramses_logic, relink_scriptToNode) {
	rlogic::LogicEngine logicEngine;
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "some scene");
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
	IN.in_float = FLOAT
    OUT.out_vec3f = VEC3F
end

function run()
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_ZYX, "some node binding");

	const rlogic::Property* nodeTranslation {nullptr};
	for (size_t i{0}; i < binding->getInputs()->getChildCount(); i++) {
		if (binding->getInputs()->getChild(i)->getName() == "translation") {
			nodeTranslation = binding->getInputs()->getChild(i);
		}
	}
	ASSERT_TRUE(nodeTranslation != nullptr);
	const rlogic::Property* scriptOutVec3f {nullptr};
	for (size_t i {0}; i < outScript->getOutputs()->getChildCount(); i++) {
		if (outScript->getOutputs()->getChild(i)->getName() == "out_vec3f") {
			scriptOutVec3f = outScript->getOutputs()->getChild(i);
		}
	}
	ASSERT_TRUE(scriptOutVec3f != nullptr);
	rlogic::Property* scriptInFloat { nullptr };
	for (size_t i {0}; i < outScript->getInputs()->getChildCount(); i++) {
		if (outScript->getInputs()->getChild(i)->getName() == "in_float") {
			scriptInFloat = outScript->getInputs()->getChild(i);
		}
	}
	ASSERT_TRUE(scriptInFloat != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	scriptInFloat->set(5.0f);
	ASSERT_TRUE(logicEngine.update());

	{
		float x,y,z;
		node->getTranslation(x,y,z);
		ASSERT_EQ(5.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	scriptInFloat->set(10.0f);
	ASSERT_TRUE(logicEngine.update());

	{
		float x,y,z;
		node->getTranslation(x,y,z);
		ASSERT_EQ(5.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.update());

	{
		float x,y,z;
		node->getTranslation(x,y,z);
		ASSERT_EQ(10.0f, x);
		ASSERT_EQ(0.0f, y);
		ASSERT_EQ(0.0f, z);
	}
}

std::vector<float> get_node_translation(ramses::Node* node) {
	float x, y, z;
	node->getTranslation(x, y, z);
	return {x, y, z};
}

std::vector<float> get_node_scale(ramses::Node* node) {
	float x, y, z;
	node->getScaling(x, y, z);
	return {x, y, z};
}
TEST(ramses_logic, linkUnlink_nodeBinding_single) {
	rlogic::LogicEngine logicEngine;
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "some scene");
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
	IN.in_float = FLOAT
    OUT.out_vec3f = VEC3F
end

function run()
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_ZYX, "some node binding");

	const rlogic::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	const rlogic::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	outScript->getInputs()->getChild("in_float")->set(1.0f);

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(get_node_translation(node), std::vector<float>({1.0, 0.0, 0.0}));

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));

	node->setTranslation(2.0, 3.0, 4.0);
	EXPECT_EQ(get_node_translation(node), std::vector<float>({2.0, 3.0, 4.0}));

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(get_node_translation(node), std::vector<float>({2.0, 3.0, 4.0}));
}

TEST(ramses_logic, linkUnlink_nodeBinding_multi) {
	rlogic::LogicEngine logicEngine;
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "some scene");
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
	IN.in_float = FLOAT
    OUT.out_vec3f = VEC3F
end

function run()
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_ZYX, "some node binding");

	const rlogic::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	const rlogic::Property* nodeScale{binding->getInputs()->getChild("scaling")};
	ASSERT_TRUE(nodeScale != nullptr);
	const rlogic::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeScale));
	outScript->getInputs()->getChild("in_float")->set(1.0f);

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(get_node_translation(node), std::vector<float>({1.0, 0.0, 0.0}));
	EXPECT_EQ(get_node_scale(node), std::vector<float>({1.0, 0.0, 0.0}));

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	outScript->getInputs()->getChild("in_float")->set(0.5f);
	node->setTranslation(2.0, 3.0, 4.0);
	EXPECT_EQ(get_node_translation(node), std::vector<float>({2.0, 3.0, 4.0}));

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(get_node_translation(node), std::vector<float>({2.0, 3.0, 4.0}));
	EXPECT_EQ(get_node_scale(node), std::vector<float>({0.5, 0.0, 0.0}));
}

TEST(ramses_logic, unlinkAndDestroy_nodeBinding) {
	rlogic::LogicEngine logicEngine;
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "some scene");
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
	IN.in_float = FLOAT
    OUT.out_vec3f = VEC3F
end

function run()
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_ZYX, "some node binding");

	const rlogic::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	const rlogic::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.destroy(*binding));
	ASSERT_TRUE(logicEngine.update());
}

TEST(ramses_logic, unlinkAndDestroyScript_multipleLinks) {
	rlogic::LogicEngine logicEngine;
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "some scene");
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface()
    OUT.translation = VEC3F
    OUT.visibility = BOOL
end

function run()
end
)");

	auto* binding = logicEngine.createRamsesNodeBinding(*node, rlogic::ERotationType::Euler_ZYX, "some node binding");

	const rlogic::Property* nodeTranslation {binding->getInputs()->getChild("translation")};
	const rlogic::Property* nodeVisibility {binding->getInputs()->getChild("visibility")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	ASSERT_TRUE(nodeVisibility != nullptr);

	const rlogic::Property* scriptTranslation {outScript->getOutputs()->getChild("translation")};
	const rlogic::Property* scriptVisiblity{outScript->getOutputs()->getChild("visibility")};
	ASSERT_TRUE(scriptTranslation != nullptr);
	ASSERT_TRUE(scriptVisiblity != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptVisiblity, *nodeVisibility));
	ASSERT_TRUE(logicEngine.link(*scriptTranslation, *nodeTranslation));

	ASSERT_TRUE(logicEngine.update());

	ASSERT_TRUE(logicEngine.unlink(*scriptVisiblity, *nodeVisibility));
	ASSERT_TRUE(logicEngine.unlink(*scriptTranslation, *nodeTranslation));
	ASSERT_TRUE(logicEngine.destroy(*outScript));

	ASSERT_TRUE(logicEngine.update());
}

TEST(ramses_logic, arrayOfStruct_linking) {
	rlogic::LogicEngine logicEngine;
	auto scriptContent{
		R"(
function interface()
	FloatPair = { a = FLOAT, b = FLOAT }
	IN.arrayOfStruct = ARRAY(2, FloatPair)
	OUT.arrayOfStruct = ARRAY(2, FloatPair)
end

function run()
	OUT.arrayOfStruct = IN.arrayOfStruct
end
)"};

	auto* startScript = logicEngine.createLuaScript(scriptContent);
	auto* endScript = logicEngine.createLuaScript(scriptContent);

	const rlogic::Property* outA{startScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")};
	const rlogic::Property* inA{endScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")};

	ASSERT_TRUE(logicEngine.link(*outA, *inA));
	ASSERT_TRUE(logicEngine.update());

	startScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->set(1.0f);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(1.0f, startScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
	ASSERT_EQ(1.0f, endScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
	ASSERT_EQ(1.0f, endScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
}

TEST(ramses_logic, array_linkToComponent) {
	rlogic::LogicEngine logicEngine;
	auto scriptContentFloatArray {
R"(
function interface()
	IN.float_array = ARRAY(5, FLOAT)
	OUT.float_array = ARRAY(5, FLOAT)
end

function run()
	OUT.float_array = IN.float_array
end
)"
	};
	auto scriptContentFloat {
R"(
function interface()
	IN.float = FLOAT
	OUT.float = FLOAT
end

function run()
	OUT.float = IN.float
end
)"
	};

	auto* startScript = logicEngine.createLuaScript(scriptContentFloat);
	auto* endScript = logicEngine.createLuaScript(scriptContentFloatArray);

	const rlogic::Property* outA{startScript->getOutputs()->getChild("float")};
	const rlogic::Property* inA{endScript->getInputs()->getChild("float_array")->getChild(0)};

	ASSERT_TRUE(logicEngine.link(*outA, *inA));
	ASSERT_TRUE(logicEngine.update());

	startScript->getInputs()->getChild("float")->set(1.0f);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(1.0f, endScript->getInputs()->getChild("float_array")->getChild(0)->get<float>().value());
}

TEST(ramses_logic, standardModules) {
	rlogic::LogicEngine logicEngine;
	auto scriptContentFloat{
		R"(
function interface()
	IN.float = FLOAT
	OUT.float = FLOAT
end

function run()
	OUT.float = math.cos(IN.float)
end
)"};

	auto luaConfig = raco::ramses_base::defaultLuaConfig();
	auto* script = logicEngine.createLuaScript(scriptContentFloat, luaConfig);

	const float pi = static_cast<float>(std::acos(-1.0));
	script->getInputs()->getChild("float")->set(pi);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(-1.0, script->getOutputs()->getChild("float")->get<float>().value());
}
