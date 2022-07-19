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

class LuaLinkOptimizationFixture : public RamsesBaseFixture<> {
public:
	LuaLinkOptimizationFixture() : RamsesBaseFixture(true) {}

	TextFile scalarInterface() {
		return makeFile("interface-scalar.lua",
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

	TextFile scalarScript() {
		return makeFile("script-scalar.lua",
			R"___(
function interface(IN, OUT)
	IN.float = Type:Float()
	IN.vector2f = Type:Vec2f()
	IN.vector3f = Type:Vec3f()
	IN.vector4f = Type:Vec4f()
	IN.integer = Type:Int32()
	IN.integer64 = Type:Int64()
	IN.vector2i = Type:Vec2i()
	IN.vector3i = Type:Vec3i()
	IN.vector4i = Type:Vec4i()
	IN.bool = Type:Bool()
	IN.string = Type:String()
end
function run(IN, OUT)
end
)___");
	}

	TextFile structInterface() {
		return makeFile("interface-struct.lua",
			R"___(
function interface(INOUT)
	INOUT.complex = {
		x = Type:Float(),
		y = Type:Float()
	}
end
)___");
	}

	TextFile structScript() {
		return makeFile("script-struct.lua",
			R"___(
function interface(IN, OUT)
	IN.complex = {
		x = Type:Float(),
		y = Type:Float()
	}
end
function run(IN, OUT)
end
)___");
	}
};

TEST_F(LuaLinkOptimizationFixture, link_opt_level_up) {
	auto interfaceFile = scalarInterface();
	auto scriptFile = scalarScript();
	auto start = create_lua_interface("start", interfaceFile);
	auto mid = create_lua_interface("mid", interfaceFile);
	auto end = create_lua("end", scriptFile);

	commandInterface.set({ start, {"inputs", "float"} }, 1.0);
	commandInterface.set({ start, {"inputs", "vector2f", "y"} }, 2.0);
	commandInterface.set({ start, {"inputs", "vector3f", "z"} }, 3.0);
	commandInterface.set({ start, {"inputs", "vector4f", "w"} }, 4.0);
	commandInterface.set({ start, {"inputs", "integer"} }, 5);
	commandInterface.set({ start, {"inputs", "integer64"} }, int64_t{ 6 });
	commandInterface.set({ start, {"inputs", "vector2i", "i2"} }, 7);
	commandInterface.set({ start, {"inputs", "vector3i", "i3"} }, 8);
	commandInterface.set({ start, {"inputs", "vector4i", "i4"} }, 9);
	commandInterface.set({ start, {"inputs", "bool"} }, true);
	commandInterface.set({ start, {"inputs", "string"} }, std::string("asdf"));

	dispatch();
	auto startEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto midEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("mid-" + mid->objectID()).c_str());
	auto endEngineObject = select<rlogic::LuaScript>(sceneContext.logicEngine(), std::string("end").c_str());
	ASSERT_TRUE(startEngineObject != nullptr);
	ASSERT_TRUE(midEngineObject != nullptr);
	ASSERT_TRUE(endEngineObject != nullptr);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 0 });
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());

	commandInterface.addLink({ start, &LuaInterface::inputs_ }, { mid, &LuaInterface::inputs_ });

	for (auto propName : { "float", "vector2f", "vector3f", "vector4f", "integer", "integer64", "vector2i", "vector3i", "vector4i", "bool", "string" }) {
		commandInterface.addLink({ mid, {"inputs", propName} }, { end, {"inputs", propName} });
	}

	dispatch();

	ASSERT_EQ(midEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(midEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 0 });
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(midEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 6 });
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string("asdf"));

}

TEST_F(LuaLinkOptimizationFixture, link_opt_level_same) {
	auto interfaceFile = scalarInterface();
	auto scriptFile = scalarScript();
	auto start = create_lua_interface("start", interfaceFile);
	auto mid = create_lua_interface("mid", interfaceFile);
	auto end = create_lua("end", scriptFile);

	commandInterface.set({ start, {"inputs", "float"} }, 1.0);
	commandInterface.set({ start, {"inputs", "vector2f", "y"} }, 2.0);
	commandInterface.set({ start, {"inputs", "vector3f", "z"} }, 3.0);
	commandInterface.set({ start, {"inputs", "vector4f", "w"} }, 4.0);
	commandInterface.set({ start, {"inputs", "integer"} }, 5);
	commandInterface.set({ start, {"inputs", "integer64"} }, int64_t{ 6 });
	commandInterface.set({ start, {"inputs", "vector2i", "i2"} }, 7);
	commandInterface.set({ start, {"inputs", "vector3i", "i3"} }, 8);
	commandInterface.set({ start, {"inputs", "vector4i", "i4"} }, 9);
	commandInterface.set({ start, {"inputs", "bool"} }, true);
	commandInterface.set({ start, {"inputs", "string"} }, std::string("asdf"));

	dispatch();
	auto startEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start-" + start->objectID()).c_str());
	auto midEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("mid-" + mid->objectID()).c_str());
	auto endEngineObject = select<rlogic::LuaScript>(sceneContext.logicEngine(), std::string("end").c_str());
	ASSERT_TRUE(startEngineObject != nullptr);
	ASSERT_TRUE(midEngineObject != nullptr);
	ASSERT_TRUE(endEngineObject != nullptr);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 0 });
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());


	for (auto propName : { "float", "vector2f", "vector3f", "vector4f", "integer", "integer64", "vector2i", "vector3i", "vector4i", "bool", "string" }) {
		commandInterface.addLink({ start, {"inputs", propName} }, { mid, {"inputs", propName} });
		commandInterface.addLink({ mid, {"inputs", propName} }, { end, {"inputs", propName} });
	}

	dispatch();

	ASSERT_EQ(midEngineObject->getInputs()->getChild("float")->get<float>(), 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 0.0);

	ASSERT_EQ(midEngineObject->getInputs()->getChild("integer")->get<int>(), 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 0 });
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 0);

	ASSERT_EQ(midEngineObject->getInputs()->getChild("bool")->get<bool>(), false);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string());

	ASSERT_EQ(endEngineObject->getInputs()->getChild("float")->get<float>(), 1.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2f")->get<rlogic::vec2f>().value()[1], 2.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3f")->get<rlogic::vec3f>().value()[2], 3.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4f")->get<rlogic::vec4f>().value()[3], 4.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer")->get<int>(), 5);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("integer64")->get<int64_t>(), int64_t{ 6 });
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector2i")->get<rlogic::vec2i>().value()[1], 7);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector3i")->get<rlogic::vec3i>().value()[2], 8);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("vector4i")->get<rlogic::vec4i>().value()[3], 9);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("bool")->get<bool>(), true);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("string")->get<std::string>(), std::string("asdf"));
}

TEST_F(LuaLinkOptimizationFixture, link_opt_level_down) {
	auto scalarInterfaceFile = scalarInterface();
	auto structInterfaceFile = structInterface();
	auto structScriptFile = structScript();
	auto start_a = create_lua_interface("start_a", scalarInterfaceFile);
	auto start_b = create_lua_interface("start_b", scalarInterfaceFile);
	auto mid = create_lua_interface("mid", structInterfaceFile);
	auto end = create_lua("end", structScriptFile);

	commandInterface.set({ start_a, {"inputs", "float"} }, 1.0);
	commandInterface.set({ start_b, {"inputs", "float"} }, 2.0);

	dispatch();
	auto startAEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start_a-" + start_a->objectID()).c_str());
	auto startBEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("start_b-" + start_b->objectID()).c_str());
	auto midEngineObject = select<rlogic::LuaInterface>(sceneContext.logicEngine(), std::string("mid-" + mid->objectID()).c_str());
	auto endEngineObject = select<rlogic::LuaScript>(sceneContext.logicEngine(), std::string("end").c_str());
	ASSERT_TRUE(startAEngineObject != nullptr);
	ASSERT_TRUE(startBEngineObject != nullptr);
	ASSERT_TRUE(midEngineObject != nullptr);
	ASSERT_TRUE(endEngineObject != nullptr);

	commandInterface.addLink({ start_a, {"inputs", "float"} }, { mid, {"inputs", "complex", "x"} });
	commandInterface.addLink({ start_b, {"inputs", "float"} }, { mid, {"inputs", "complex", "y"} });
	commandInterface.addLink({ mid, {"inputs", "complex"} }, { end, {"inputs", "complex"} });

	dispatch();
	ASSERT_EQ(midEngineObject->getInputs()->getChild("complex")->getChild("x")->get<float>(), 0.0);
	ASSERT_EQ(midEngineObject->getInputs()->getChild("complex")->getChild("y")->get<float>(), 0.0);

	ASSERT_EQ(endEngineObject->getInputs()->getChild("complex")->getChild("x")->get<float>(), 1.0);
	ASSERT_EQ(endEngineObject->getInputs()->getChild("complex")->getChild("y")->get<float>(), 2.0);
}