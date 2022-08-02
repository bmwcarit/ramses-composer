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

#include "ramses_adaptor/LuaInterfaceAdaptor.h"

using namespace raco::user_types;

class LuaInterfaceAdaptorFixture : public RamsesBaseFixture<> {
public:
	TextFile defaultFile() {
		return makeFile("interface.lua",
			R"___(
function interface(INOUT)
	INOUT.float = Type:Float()
	INOUT.vector2f = Type:Vec2f()
	INOUT.vector3f = Type:Vec3f()
	INOUT.vector4f = Type:Vec4f()
	INOUT.integer = Type:Int32()
	INOUT.integer64 = Type:Int64()
	INOUT.vector2i = Type:Vec2i()
	INOUT.vector3i = Type:Vec3i()
	INOUT.vector4i = Type:Vec4i()
	INOUT.bool = Type:Bool()
	INOUT.string = Type:String()
end
)___");
	}

	TextFile emptyInterfaceFile() {
		return makeFile("interface.lua",
			R"___(
function interface(INOUT)
end
)___");
	}

	TextFile emptyScriptFile() {
		return makeFile("script.lua",
			R"___(
function interface(IN, OUT)
end
function run(IN, OUT)
end
)___");
	}
};

TEST_F(LuaInterfaceAdaptorFixture, defaultConstruction) {
	context.createObject(LuaInterface::typeDescription.typeName, "Module");

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaInterface>().size(), 0);
}


TEST_F(LuaInterfaceAdaptorFixture, invalid_text) {
	auto interfaceFile = makeFile("interface.lua",
		R"___(
invalid interface definition
)___");

	auto interface = create_lua_interface("interface", interfaceFile);

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaInterface>().size(), 0);
}

TEST_F(LuaInterfaceAdaptorFixture, valid_text) {
	auto interfaceFile = defaultFile();
	auto interface = create_lua_interface("interface", interfaceFile);

	dispatch();
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaInterface>().size(), 1);

	auto engineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("interface-" + interface->objectID()).c_str());
	ASSERT_TRUE(engineObject != nullptr);
	ASSERT_EQ(engineObject->getUserId(), interface->objectIDAsRamsesLogicID());
}

TEST_F(LuaInterfaceAdaptorFixture, change_name) {
	auto interfaceFile = defaultFile();
	auto interface = create_lua_interface("interface", interfaceFile);

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaInterface>().size(), 1);
	auto engineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("interface-" + interface->objectID()).c_str());
	ASSERT_TRUE(engineObject != nullptr);
	ASSERT_EQ(engineObject->getUserId(), interface->objectIDAsRamsesLogicID());

	commandInterface.set({interface, &LuaInterface::objectName_}, std::string("newName"));
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::LuaInterface>().size(), 1);
	ASSERT_EQ(select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("interface-" + interface->objectID()).c_str()), nullptr);
	engineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("newName-" + interface->objectID()).c_str());
	ASSERT_TRUE(engineObject != nullptr);
	ASSERT_EQ(engineObject->getUserId(), interface->objectIDAsRamsesLogicID());
}

TEST_F(LuaInterfaceAdaptorFixture, change_property_value) {
	auto interfaceFile = defaultFile();
	auto interface = create_lua_interface("interface", interfaceFile);
	dispatch();

	auto engineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("interface-" + interface->objectID()).c_str());

	ASSERT_EQ(engineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(engineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(engineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{0});
	ASSERT_EQ(engineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(engineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(engineObject->getInputs()->getChild("string")->get<std::string>(), std::string());

	commandInterface.set({interface, {"inputs", "float"}}, 1.0);
	commandInterface.set({interface, {"inputs", "vector2f", "y"}}, 2.0);
	commandInterface.set({interface, {"inputs", "vector3f", "z"}}, 3.0);
	commandInterface.set({interface, {"inputs", "vector4f", "w"}}, 4.0);
	commandInterface.set({interface, {"inputs", "integer"}}, 5);
	commandInterface.set({interface, {"inputs", "integer64"}}, int64_t{6});
	commandInterface.set({interface, {"inputs", "vector2i", "i2"}}, 7);
	commandInterface.set({interface, {"inputs", "vector3i", "i3"}}, 8);
	commandInterface.set({interface, {"inputs", "vector4i", "i4"}}, 9);
	commandInterface.set({interface, {"inputs", "bool"}}, true);
	commandInterface.set({interface, {"inputs", "string"}}, std::string("asdf"));

	dispatch();
	// check inputs
	ASSERT_EQ(engineObject->getInputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(engineObject->getInputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(engineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{6});
	ASSERT_EQ(engineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(engineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(engineObject->getInputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(engineObject->getInputs()->getChild("string")->get<std::string>(), std::string("asdf"));

	// check outputs
	ASSERT_EQ(engineObject->getOutputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(engineObject->getOutputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(engineObject->getOutputs()->getChild("integer64")->get<int64_t>(), int64_t{6});
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(engineObject->getOutputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(engineObject->getOutputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(engineObject->getOutputs()->getChild("string")->get<std::string>(), std::string("asdf"));
}

TEST_F(LuaInterfaceAdaptorFixture, link_invalid_interface_to_invalid_script) {
	auto start = create<LuaInterface>("start");
	auto end = create<LuaScript>("end");
	
	commandInterface.addLink({start, {"inputs"}}, {end, {"inputs"}});
	checkLinks({{{start, {"inputs"}}, {end, {"inputs"}}}});

	// test shouldn't crash here
	dispatch();

	auto ramsesStart = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto ramsesEnd = select<rlogic::LuaScript>(sceneContext.logicEngine(), "end");
	EXPECT_EQ(ramsesStart, nullptr);
	EXPECT_EQ(ramsesEnd, nullptr);

	// TODO check that logicengine contains no links
	// needs logicengine support functions!
}

TEST_F(LuaInterfaceAdaptorFixture, link_invalid_interface_to_invalid_interface) {
	auto start = create<LuaInterface>("start");
	auto end = create<LuaInterface>("end");

	commandInterface.addLink({start, {"inputs"}}, {end, {"inputs"}});
	checkLinks({{{start, {"inputs"}}, {end, {"inputs"}}}});

	// test shouldn't crash here
	dispatch();

	auto ramsesStart = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto ramsesEnd = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("end-" + end->objectID()).c_str());
	EXPECT_EQ(ramsesStart, nullptr);
	EXPECT_EQ(ramsesEnd, nullptr);

	// TODO check that logicengine contains no links
	// needs logicengine support functions!
}

TEST_F(LuaInterfaceAdaptorFixture, link_empty_interface_to_empty_script) {
	auto interfaceFile = emptyInterfaceFile();
	auto scriptFile = emptyScriptFile();
	auto start = create_lua_interface("start", interfaceFile);
	auto end = create_lua("end", scriptFile);

	commandInterface.addLink({start, {"inputs"}}, {end, {"inputs"}});
	checkLinks({{{start, {"inputs"}}, {end, {"inputs"}}}});

	// test shouldn't crash here
	dispatch();

	auto ramsesStart = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto ramsesEnd = select<rlogic::LuaScript>(sceneContext.logicEngine(), "end");
	EXPECT_NE(ramsesStart, nullptr);
	EXPECT_NE(ramsesEnd, nullptr);

	// TODO check that logicengine contains no links
	// needs logicengine support functions!
}

TEST_F(LuaInterfaceAdaptorFixture, link_empty_interface_to_empty_interface) {
	auto interfaceFile = emptyInterfaceFile();
	auto start = create_lua_interface("start", interfaceFile);
	auto end = create_lua_interface("end", interfaceFile);

	commandInterface.addLink({start, {"inputs"}}, {end, {"inputs"}});
	checkLinks({{{start, {"inputs"}}, {end, {"inputs"}}}});

	// test shouldn't crash here
	dispatch();

	auto ramsesStart = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto ramsesEnd = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("end-" + end->objectID()).c_str());
	EXPECT_NE(ramsesStart, nullptr);
	EXPECT_NE(ramsesEnd, nullptr);

	// TODO check that logicengine contains no links
	// needs logicengine support functions!
}

TEST_F(LuaInterfaceAdaptorFixture, link_individual_propagate_values) {
	auto interfaceFile = defaultFile();
	auto start = create_lua_interface("start", interfaceFile);
	auto end = create_lua_interface("end", interfaceFile);

	commandInterface.set({start, {"inputs", "float"}}, 1.0);
	commandInterface.set({start, {"inputs", "vector2f", "y"}}, 2.0);
	commandInterface.set({start, {"inputs", "vector3f", "z"}}, 3.0);
	commandInterface.set({start, {"inputs", "vector4f", "w"}}, 4.0);
	commandInterface.set({start, {"inputs", "integer"}}, 5);
	commandInterface.set({start, {"inputs", "integer64"}}, int64_t{6});
	commandInterface.set({start, {"inputs", "vector2i", "i2"}}, 7);
	commandInterface.set({start, {"inputs", "vector3i", "i3"}}, 8);
	commandInterface.set({start, {"inputs", "vector4i", "i4"}}, 9);
	commandInterface.set({start, {"inputs", "bool"}}, true);
	commandInterface.set({start, {"inputs", "string"}}, std::string("asdf"));

	dispatch();
	auto startEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto endEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("end-" + end->objectID()).c_str());
	ASSERT_TRUE(startEngineObject != nullptr);
	ASSERT_TRUE(endEngineObject != nullptr);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{0});
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());


	commandInterface.addLink({start, {"inputs", "float"}}, {end, {"inputs", "float"}});
	commandInterface.addLink({start, {"inputs", "vector2f"}}, {end, {"inputs", "vector2f"}});
	commandInterface.addLink({start, {"inputs", "vector3f"}}, {end, {"inputs", "vector3f"}});
	commandInterface.addLink({start, {"inputs", "vector4f"}}, {end, {"inputs", "vector4f"}});
	commandInterface.addLink({start, {"inputs", "integer"}}, {end, {"inputs", "integer"}});
	commandInterface.addLink({start, {"inputs", "integer64"}}, {end, {"inputs", "integer64"}});
	commandInterface.addLink({start, {"inputs", "vector2i"}}, {end, {"inputs", "vector2i"}});
	commandInterface.addLink({start, {"inputs", "vector3i"}}, {end, {"inputs", "vector3i"}});
	commandInterface.addLink({start, {"inputs", "vector4i"}}, {end, {"inputs", "vector4i"}});
	commandInterface.addLink({start, {"inputs", "bool"}}, {end, {"inputs", "bool"}});
	commandInterface.addLink({start, {"inputs", "string"}}, {end, {"inputs", "string"}});
	dispatch();

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{6});
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string("asdf"));


	commandInterface.set({start, {"inputs", "float"}}, 2.0);
	dispatch();

	ASSERT_EQ(startEngineObject->getInputs()->getChild("float")->get<float>(), 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 2.0);
}


TEST_F(LuaInterfaceAdaptorFixture, link_container_propagate_values) {
	auto interfaceFile = defaultFile();
	auto start = create_lua_interface("start", interfaceFile);
	auto end = create_lua_interface("end", interfaceFile);

	commandInterface.set({start, {"inputs", "float"}}, 1.0);
	commandInterface.set({start, {"inputs", "vector2f", "y"}}, 2.0);
	commandInterface.set({start, {"inputs", "vector3f", "z"}}, 3.0);
	commandInterface.set({start, {"inputs", "vector4f", "w"}}, 4.0);
	commandInterface.set({start, {"inputs", "integer"}}, 5);
	commandInterface.set({start, {"inputs", "integer64"}}, int64_t{6});
	commandInterface.set({start, {"inputs", "vector2i", "i2"}}, 7);
	commandInterface.set({start, {"inputs", "vector3i", "i3"}}, 8);
	commandInterface.set({start, {"inputs", "vector4i", "i4"}}, 9);
	commandInterface.set({start, {"inputs", "bool"}}, true);
	commandInterface.set({start, {"inputs", "string"}}, std::string("asdf"));

	dispatch();
	auto startEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto endEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("end-" + end->objectID()).c_str());
	ASSERT_TRUE(startEngineObject != nullptr);
	ASSERT_TRUE(endEngineObject != nullptr);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{0});
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());

	commandInterface.addLink({start, &LuaInterface::inputs_}, {end, &LuaInterface::inputs_});
	dispatch();

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{6});
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string("asdf"));

	commandInterface.set({start, {"inputs", "float"}}, 2.0);
	dispatch();

	ASSERT_EQ(startEngineObject->getInputs()->getChild("float")->get<float>(), 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 2.0);
}