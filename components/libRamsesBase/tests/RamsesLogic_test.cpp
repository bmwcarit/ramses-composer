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
#include <gtest/gtest-spi.h>

#include "RamsesBaseFixture.h"
#include "testing/RacoBaseTest.h"
#include "ramses_base/Utils.h"
#include "ramses_adaptor/utilities.h"

#include <ramses/client/logic/LogicEngine.h>
#include <ramses/client/logic/LuaScript.h>
#include <ramses/client/logic/Property.h>
#include <ramses/client/logic/NodeBinding.h>
#include <ramses/framework/RamsesFramework.h>
#include <ramses/client/RamsesClient.h>
#include <ramses/client/Scene.h>
#include <ramses/client/Node.h>

#include <cmath>

/*
 * Basic test for third_party dependiences: ramses-logic.
 * Should be used to verify stability of dependency.
 */

using namespace raco::ramses_adaptor;

class RamsesLogicTest : public RacoBaseTest<> {
public:
	RamsesLogicTest() 
		: ramsesFramework(ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}),
		  client(ramsesFramework.createClient("example client")),
		  scene(client->createScene(ramses::sceneId_t(123u), "some scene")),
		  logicEngine(*scene->createLogicEngine("logic engine"))	{
	}

	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient* client;
	ramses::Scene* scene;
	ramses::LogicEngine& logicEngine;
};

TEST_F(RamsesLogicTest, repeat_create_destroy_simple_script) {
	std::string scriptText = R"(
function interface(IN,OUT)
    OUT.out_float = Type:Float()
end

function run(IN,OUT)
end
)";
	for (unsigned i = 0; i < 10000; i++) {
		auto* script = logicEngine.createLuaScript(scriptText);
		ASSERT_TRUE(script != nullptr);
		ASSERT_TRUE(logicEngine.destroy(*script));
	}
}

TEST_F(RamsesLogicTest, relink_scriptToScript) {
	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
    OUT.out_float = Type:Float()
end

function run(IN,OUT)
end
)");
	auto* inScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
    IN.in_float = Type:Float()
end

function run(IN,OUT)
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

TEST_F(RamsesLogicTest, relink_scriptToNode) {
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
	IN.in_float = Type:Float()
    OUT.out_vec3f = Type:Vec3f()
end

function run(IN,OUT)
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "some node binding");

	ramses::Property* nodeTranslation {nullptr};
	for (size_t i{0}; i < binding->getInputs()->getChildCount(); i++) {
		if (binding->getInputs()->getChild(i)->getName() == "translation") {
			nodeTranslation = binding->getInputs()->getChild(i);
		}
	}
	ASSERT_TRUE(nodeTranslation != nullptr);
	ramses::Property* scriptOutVec3f {nullptr};
	for (size_t i {0}; i < outScript->getOutputs()->getChildCount(); i++) {
		if (outScript->getOutputs()->getChild(i)->getName() == "out_vec3f") {
			scriptOutVec3f = outScript->getOutputs()->getChild(i);
		}
	}
	ASSERT_TRUE(scriptOutVec3f != nullptr);
	ramses::Property* scriptInFloat{nullptr};
	for (size_t i{0}; i < outScript->getInputs()->getChildCount(); i++) {
		if (outScript->getInputs()->getChild(i)->getName() == "in_float") {
			scriptInFloat = outScript->getInputs()->getChild(i);
		}
	}
	ASSERT_TRUE(scriptInFloat != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	scriptInFloat->set(5.0f);
	ASSERT_TRUE(logicEngine.update());

	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(5.0, 0.0, 0.0));

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	scriptInFloat->set(10.0f);
	ASSERT_TRUE(logicEngine.update());

	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(5.0, 0.0, 0.0));

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.update());

	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(10.0, 0.0, 0.0));
}

TEST_F(RamsesLogicTest, linkUnlink_nodeBinding_single) {
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
	IN.in_float = Type:Float()
    OUT.out_vec3f = Type:Vec3f()
end

function run(IN,OUT)
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "some node binding");

	ramses::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	ramses::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	outScript->getInputs()->getChild("in_float")->set(1.0f);

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(1.0, 0.0, 0.0));

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));

	node->setTranslation({2.0, 3.0, 4.0});
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(2.0, 3.0, 4.0));

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(2.0, 3.0, 4.0));
}

TEST_F(RamsesLogicTest, linkUnlink_nodeBinding_multi) {
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
	IN.in_float = Type:Float()
    OUT.out_vec3f = Type:Vec3f()
end

function run(IN,OUT)
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "some node binding");

	ramses::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	ramses::Property* nodeScale{binding->getInputs()->getChild("scaling")};
	ASSERT_TRUE(nodeScale != nullptr);
	ramses::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeScale));
	outScript->getInputs()->getChild("in_float")->set(1.0f);

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(1.0, 0.0, 0.0));
	EXPECT_EQ(getRamsesScaling(node), glm::vec3(1.0, 0.0, 0.0));

	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	outScript->getInputs()->getChild("in_float")->set(0.5f);
	node->setTranslation({2.0, 3.0, 4.0});
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(2.0, 3.0, 4.0));
	EXPECT_EQ(getRamsesScaling(node), glm::vec3(1.0, 0.0, 0.0));

	ASSERT_TRUE(logicEngine.update());
	EXPECT_EQ(getRamsesTranslation(node), glm::vec3(2.0, 3.0, 4.0));
	EXPECT_EQ(getRamsesScaling(node), glm::vec3(0.5, 0.0, 0.0));
}

TEST_F(RamsesLogicTest, unlinkAndDestroy_nodeBinding) {
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
	IN.in_float = Type:Float()
    OUT.out_vec3f = Type:Vec3f()
end

function run(IN,OUT)
	OUT.out_vec3f = { IN.in_float, 0.0, 0.0 }
end
)");

	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "some node binding");

	ramses::Property* nodeTranslation{binding->getInputs()->getChild("translation")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	ramses::Property* scriptOutVec3f{outScript->getOutputs()->getChild("out_vec3f")};
	ASSERT_TRUE(scriptOutVec3f != nullptr);

	ASSERT_TRUE(logicEngine.link(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.unlink(*scriptOutVec3f, *nodeTranslation));
	ASSERT_TRUE(logicEngine.destroy(*binding));
	ASSERT_TRUE(logicEngine.update());
}

TEST_F(RamsesLogicTest, unlinkAndDestroyScript_multipleLinks) {
	ramses::Node* node = scene->createNode("some node");

	auto* outScript = logicEngine.createLuaScript(R"(
function interface(IN,OUT)
    OUT.translation = Type:Vec3f()
    OUT.visibility = Type:Bool()
end

function run(IN,OUT)
end
)");

	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "some node binding");

	ramses::Property* nodeTranslation {binding->getInputs()->getChild("translation")};
	ramses::Property* nodeVisibility {binding->getInputs()->getChild("visibility")};
	ASSERT_TRUE(nodeTranslation != nullptr);
	ASSERT_TRUE(nodeVisibility != nullptr);

	ramses::Property* scriptTranslation {outScript->getOutputs()->getChild("translation")};
	ramses::Property* scriptVisiblity{outScript->getOutputs()->getChild("visibility")};
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

TEST_F(RamsesLogicTest, arrayOfStruct_linking) {
	auto scriptContent{
		R"(
function interface(IN,OUT)
	local FloatPair = { a = Type:Float(), b = Type:Float() }
	IN.arrayOfStruct = Type:Array(2, FloatPair)
	OUT.arrayOfStruct = Type:Array(2, FloatPair)
end

function run(IN,OUT)
	OUT.arrayOfStruct = IN.arrayOfStruct
end
)"};

	auto* startScript = logicEngine.createLuaScript(scriptContent);
	auto* endScript = logicEngine.createLuaScript(scriptContent);

	ramses::Property* outA{startScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")};
	ramses::Property* inA{endScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")};

	ASSERT_TRUE(logicEngine.link(*outA, *inA));
	ASSERT_TRUE(logicEngine.update());

	startScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->set(1.0f);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(1.0f, startScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
	ASSERT_EQ(1.0f, endScript->getInputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
	ASSERT_EQ(1.0f, endScript->getOutputs()->getChild("arrayOfStruct")->getChild(0)->getChild("a")->get<float>().value());
}

TEST_F(RamsesLogicTest, array_linkToComponent) {
	auto scriptContentFloatArray{
		R"(
function interface(IN,OUT)
	IN.float_array = Type:Array(5, Type:Float())
	OUT.float_array = Type:Array(5, Type:Float())
end

function run(IN,OUT)
	OUT.float_array = IN.float_array
end
)"};
	auto scriptContentFloat{
		R"(
function interface(IN,OUT)
	IN.float = Type:Float()
	OUT.float = Type:Float()
end

function run(IN,OUT)
	OUT.float = IN.float
end
)"};

	auto* startScript = logicEngine.createLuaScript(scriptContentFloat);
	auto* endScript = logicEngine.createLuaScript(scriptContentFloatArray);

	ramses::Property* outA{startScript->getOutputs()->getChild("float")};
	ramses::Property* inA{endScript->getInputs()->getChild("float_array")->getChild(0)};

	ASSERT_TRUE(logicEngine.link(*outA, *inA));
	ASSERT_TRUE(logicEngine.update());

	startScript->getInputs()->getChild("float")->set(1.0f);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(1.0f, endScript->getInputs()->getChild("float_array")->getChild(0)->get<float>().value());
}

TEST_F(RamsesLogicTest, standardModules) {
	auto scriptContentFloat{
		R"(
function interface(IN,OUT)
	IN.float = Type:Float()
	OUT.float = Type:Float()
end

function run(IN,OUT)
	OUT.float = math.cos(IN.float)
end
)"};

	auto luaConfig = ramses_base::defaultLuaConfig();
	auto* script = logicEngine.createLuaScript(scriptContentFloat, luaConfig);

	const float pi = static_cast<float>(std::acos(-1.0));
	script->getInputs()->getChild("float")->set(pi);
	ASSERT_TRUE(logicEngine.update());
	ASSERT_EQ(-1.0, script->getOutputs()->getChild("float")->get<float>().value());
}

TEST_F(RamsesLogicTest, weird_crash) {

	std::string scriptText = R"(
function interface(IN,OUT)
    local craneGimbal = {
        cam_Translation = Type:Vec3f(),
        POS_ORIGIN_Translation = Type:Vec3f(),
        PITCH_Rotation = Type:Vec3f(),
        YAW_Rotation = Type:Vec3f()
    }
    
    local viewport = {
        offsetX = Type:Int32(),
        offsetY = Type:Int32(),
        width = Type:Int32(),
        height = Type:Int32()
    }
    
    local frustum_persp = {
        nearPlane = Type:Float(),
        farPlane =  Type:Float(),
        fieldOfView = Type:Float(),
        aspectRatio = Type:Float()
    }
    
    local frustum_ortho = {
        nearPlane = Type:Float(),
        farPlane =  Type:Float(),
        leftPlane = Type:Float(),
        rightPlane = Type:Float(),
        bottomPlane = Type:Float(),
        topPlane = Type:Float()
    }

    OUT.CameraSettings = {
        CraneGimbal = craneGimbal,
        CraneGimbal_R = craneGimbal,
		scene_camera = {
            Viewport = viewport,
            Frustum = frustum_persp,
        },
        ui_camera = {
            Frustum = frustum_ortho,
            Viewport = viewport,
            translation = Type:Vec3f(), 
        }
	}
end

function run(IN,OUT)
end
)";
	for (unsigned i = 0; i < 10000; i++) {
		auto* script = logicEngine.createLuaScript(scriptText);
		ASSERT_TRUE(script != nullptr);
		ASSERT_TRUE(logicEngine.destroy(*script));
	}
}

TEST_F(RamsesLogicTest, node_binding_quaternion_rotation) {
	ramses::Node* node = scene->createNode("some node");
	auto* binding = logicEngine.createNodeBinding(*node, ramses::ERotationType::Quaternion, "some node binding");

	float delta = 1e-4;

	float one_over_sqrt_6 = 1.0f / sqrt(6.0f);
	float one_over_sqrt_2 = 1.0f / sqrt(2.0f);
	glm::vec4 q{one_over_sqrt_6, one_over_sqrt_6, one_over_sqrt_6, one_over_sqrt_2};
	float norm2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	ASSERT_TRUE(std::abs(norm2 - 1.0f) < delta);

	ramses::Property* nodeRotation{binding->getInputs()->getChild("rotation")};
	nodeRotation->set(q);

	logicEngine.update();

	glm::mat4x4 ramsesModelMatrix;
	node->getModelMatrix(ramsesModelMatrix);

	glm::mat4x4 refModelMatrix;

	// m11
	refModelMatrix[0][0] = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
	// m21
	refModelMatrix[0][1] = 2.0f * (q[0] * q[1] + q[2] * q[3]);
	// m31
	refModelMatrix[0][2] = 2.0f * (q[0] * q[2] - q[1] * q[3]);
	// m41
	refModelMatrix[0][3] = 0.0;

	// m12
	refModelMatrix[1][0] = 2.0f * (q[0] * q[1] - q[2] * q[3]);
	// m22
	refModelMatrix[1][1] = 1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]);
	// m32
	refModelMatrix[1][2] = 2.0f * (q[1] * q[2] + q[0] * q[3]);
	// m42
	refModelMatrix[1][3] = 0.0;

	// m13
	refModelMatrix[2][0] = 2.0f * (q[0] * q[2] + q[1] * q[3]);
	// m23
	refModelMatrix[2][1] = 2.0f * (q[1] * q[2] - q[0] * q[3]);
	// m33
	refModelMatrix[2][2] = 1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]);
	// m43
	refModelMatrix[2][3] = 0.0;

	// m14 .. m44
	refModelMatrix[3][0] = 0.0;
	refModelMatrix[3][1] = 0.0;
	refModelMatrix[3][2] = 0.0;
	refModelMatrix[3][3] = 1.0;

	bool comp = true;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (std::abs(refModelMatrix[i][j] - ramsesModelMatrix[i][j]) > delta) {
				comp = false;
			}
		}
	}
	EXPECT_TRUE(comp);
}

TEST_F(RamsesLogicTest, interface_using_modules_invalid_arg_pass_no_config) {
	auto text{
		R"(
modules(42)
function interface(IN,OUT)
end
)"};

	auto* interface = logicEngine.createLuaInterface(text, "interface");
	ASSERT_TRUE(interface == nullptr);
}

TEST_F(RamsesLogicTest, interface_using_valid_arg_modules_pass_no_config) {
	auto text{
		R"(
modules("mymodule")
function interface(IN,OUT)
end
)"};

	auto* interface = logicEngine.createLuaInterface(text, "interface");
	ASSERT_TRUE(interface == nullptr);
}

TEST_F(RamsesLogicTest, interface_using_modules_invalid_arg_pass_config) {
	auto text{
		R"(
modules(42)
function interface(IN,OUT)
end
)"};

	ramses::LuaConfig config;
	auto* interface = logicEngine.createLuaInterface(text, "interface", config);
	ASSERT_TRUE(interface == nullptr);
}

TEST_F(RamsesLogicTest, interface_using_valid_arg_modules_pass_config) {
	auto text{
		R"(
modules("mymodule")
function interface(IN,OUT)
end
)"};

	ramses::LuaConfig config;
	auto* interface = logicEngine.createLuaInterface(text, "interface", config);
	ASSERT_TRUE(interface == nullptr);
}

TEST_F(RamsesLogicTest, deserialize_node_binding_enable_visibility) {
	std::string ramsesFile = (test_path() / "test.ramses").string();
	client->destroy(*scene);

	for (bool enabled : {false, true}) {
		for (bool visibility : {false, true}) {
			ramses::EVisibilityMode mode = enabled ? (visibility ? ramses::EVisibilityMode::Visible : ramses::EVisibilityMode::Invisible) : ramses::EVisibilityMode::Off;

			{
				scene = client->createScene(ramses::sceneId_t(123u), "some scene");
				auto logicEngine = scene->createLogicEngine("logic engine");

				ramses::Node* node = scene->createNode("Node");
				auto* binding = logicEngine->createNodeBinding(*node, ramses::ERotationType::Euler_ZYX, "NodeBinding");

				ramses::Property* prop_enabled{binding->getInputs()->getChild("enabled")};
				ASSERT_TRUE(prop_enabled != nullptr);

				ramses::Property* prop_visibility{binding->getInputs()->getChild("visibility")};
				ASSERT_TRUE(prop_visibility != nullptr);

				prop_enabled->set(enabled);
				prop_visibility->set(visibility);
				logicEngine->update();

				EXPECT_EQ(node->getVisibility(), mode);
				EXPECT_EQ(prop_enabled->get<bool>(), enabled);
				EXPECT_EQ(prop_visibility->get<bool>(), visibility);

				ramses::SaveFileConfig config;

				ASSERT_TRUE(scene->saveToFile(ramsesFile.c_str()));

				client->destroy(*scene);
			}

			{
				scene = client->loadSceneFromFile(ramsesFile);
				ASSERT_TRUE(scene != nullptr);
				auto logicEngine = select<ramses::LogicEngine>(*scene, "logic engine");
				ASSERT_TRUE(logicEngine != nullptr);

				auto node = select<ramses::Node>(*scene, "Node");
				ASSERT_TRUE(node != nullptr);
				auto binding = select<ramses::NodeBinding>(*logicEngine, "NodeBinding");
				ASSERT_TRUE(binding != nullptr);

				const ramses::Property* prop_enabled{binding->getInputs()->getChild("enabled")};
				ASSERT_TRUE(prop_enabled != nullptr);

				const ramses::Property* prop_visibility{binding->getInputs()->getChild("visibility")};
				ASSERT_TRUE(prop_visibility != nullptr);

				EXPECT_EQ(node->getVisibility(), mode);
				EXPECT_EQ(prop_enabled->get<bool>().value(), enabled);
				ASSERT_EQ(prop_visibility->get<bool>().value(), visibility) << fmt::format(" enabled /  visibility = {} / {}", enabled, visibility);

				client->destroy(*scene);
			}
		}
	}
}
