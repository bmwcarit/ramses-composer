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
#include "ramses_adaptor/MaterialAdaptor.h"

class MaterialAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(MaterialAdaptorTest, context_scene_effect_name_change) {
	auto node = context.createObject(raco::user_types::Material::typeDescription.typeName, "Material Name");

	dispatch();

	auto effects{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Effect)};
	EXPECT_EQ(effects.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray("Material Name", effects));

	auto appearances{select<ramses::Appearance>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Appearance)};
	EXPECT_EQ(appearances.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray("Material Name_Appearance", appearances));

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	effects = select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Effect);
	EXPECT_STREQ("Changed", effects[0]->getName());
	ASSERT_TRUE(isRamsesNameInArray("Changed", effects));

	appearances = select<ramses::Appearance>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Appearance);
	EXPECT_EQ(appearances.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray("Changed_Appearance", appearances));
}

TEST_F(MaterialAdaptorTest, set_get_scalar_uniforms) {
	auto material = create_material("mat", "shaders/uniform-scalar.vert", "shaders/uniform-scalar.frag");

	commandInterface.set({material, {"uniforms", "i"}}, 2);
	commandInterface.set({material, {"uniforms", "f"}}, 3.0);

	commandInterface.set({material, {"uniforms", "v2", "y"}}, 4.0);
	commandInterface.set({material, {"uniforms", "v3", "z"}}, 5.0);
	commandInterface.set({material, {"uniforms", "v4", "w"}}, 6.0);

	commandInterface.set({material, {"uniforms", "iv2", "i1"}}, 7);
	commandInterface.set({material, {"uniforms", "iv3", "i2"}}, 8);
	commandInterface.set({material, {"uniforms", "iv4", "i3"}}, 9);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	ramses::UniformInput input;

	int32_t ivalue;
	appearance->getEffect().findUniformInput("i", input);
	appearance->getInputValueInt32(input, ivalue);
	EXPECT_EQ(ivalue, 2);

	float fvalue;
	appearance->getEffect().findUniformInput("f", input);
	appearance->getInputValueFloat(input, fvalue);
	EXPECT_EQ(fvalue, 3.0);

	std::array<float, 2> value2f;
	appearance->getEffect().findUniformInput("v2", input);
	appearance->getInputValueVector2f(input, value2f[0], value2f[1]);
	EXPECT_EQ(value2f[1], 4.0);

	std::array<float, 3> value3f;
	appearance->getEffect().findUniformInput("v3", input);
	appearance->getInputValueVector3f(input, value3f[0], value3f[1], value3f[2]);
	EXPECT_EQ(value3f[2], 5.0);

	std::array<float, 4> value4f;
	appearance->getEffect().findUniformInput("v4", input);
	appearance->getInputValueVector4f(input, value4f[0], value4f[1], value4f[2], value4f[3]);
	EXPECT_EQ(value4f[3], 6.0);

	std::array<int32_t, 2> value2i;
	appearance->getEffect().findUniformInput("iv2", input);
	appearance->getInputValueVector2i(input, value2i[0], value2i[1]);
	EXPECT_EQ(value2i[0], 7);

	std::array<int32_t, 3> value3i;
	appearance->getEffect().findUniformInput("iv3", input);
	appearance->getInputValueVector3i(input, value3i[0], value3i[1], value3i[2]);
	EXPECT_EQ(value3i[1], 8);

	std::array<int32_t, 4> value4i;
	appearance->getEffect().findUniformInput("iv4", input);
	appearance->getInputValueVector4i(input, value4i[0], value4i[1], value4i[2], value4i[3]);
	EXPECT_EQ(value4i[2], 9);
}

TEST_F(MaterialAdaptorTest, link_get_scalar_uniforms) {
	auto material = create_material("mat", "shaders/uniform-scalar.vert", "shaders/uniform-scalar.frag");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	link(lua, {"outputs", "ointeger"}, material, {"uniforms", "i"});
	link(lua, {"outputs", "ofloat"}, material, {"uniforms", "f"});

	link(lua, {"outputs", "ovector2f"}, material, {"uniforms", "v2"});
	link(lua, {"outputs", "ovector3f"}, material, {"uniforms", "v3"});
	link(lua, {"outputs", "ovector4f"}, material, {"uniforms", "v4"});

	link(lua, {"outputs", "ovector2i"}, material, {"uniforms", "iv2"});
	link(lua, {"outputs", "ovector3i"}, material, {"uniforms", "iv3"});
	link(lua, {"outputs", "ovector4i"}, material, {"uniforms", "iv4"});

	dispatch();

	commandInterface.set({lua, {"inputs", "integer"}}, 7);
	commandInterface.set({lua, {"inputs", "float"}}, 9.0);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	ramses::UniformInput input;

	int32_t ivalue;
	appearance->getEffect().findUniformInput("i", input);
	appearance->getInputValueInt32(input, ivalue);
	EXPECT_EQ(ivalue, 14);

	float fvalue;
	appearance->getEffect().findUniformInput("f", input);
	appearance->getInputValueFloat(input, fvalue);
	EXPECT_EQ(fvalue, 9.0);

	std::array<float, 2> value2f;
	appearance->getEffect().findUniformInput("v2", input);
	appearance->getInputValueVector2f(input, value2f[0], value2f[1]);
	EXPECT_EQ(value2f[1], 18.0);

	std::array<float, 3> value3f;
	appearance->getEffect().findUniformInput("v3", input);
	appearance->getInputValueVector3f(input, value3f[0], value3f[1], value3f[2]);
	EXPECT_EQ(value3f[2], 27.0);

	std::array<float, 4> value4f;
	appearance->getEffect().findUniformInput("v4", input);
	appearance->getInputValueVector4f(input, value4f[0], value4f[1], value4f[2], value4f[3]);
	EXPECT_EQ(value4f[3], 36.0);

	std::array<int32_t, 2> value2i;
	appearance->getEffect().findUniformInput("iv2", input);
	appearance->getInputValueVector2i(input, value2i[0], value2i[1]);
	EXPECT_EQ(value2i[0], 7);

	std::array<int32_t, 3> value3i;
	appearance->getEffect().findUniformInput("iv3", input);
	appearance->getInputValueVector3i(input, value3i[0], value3i[1], value3i[2]);
	EXPECT_EQ(value3i[1], 14);

	std::array<int32_t, 4> value4i;
	appearance->getEffect().findUniformInput("iv4", input);
	appearance->getInputValueVector4i(input, value4i[0], value4i[1], value4i[2], value4i[3]);
	EXPECT_EQ(value4i[2], 21);
}

TEST_F(MaterialAdaptorTest, set_get_array_uniforms) {
	auto material = create_material("mat", "shaders/uniform-array.vert", "shaders/uniform-array.frag");
	
	commandInterface.set({material, {"uniforms", "ivec", "2"}}, 2);
	commandInterface.set({material, {"uniforms", "fvec", "3"}}, 3.0);

	commandInterface.set({material, {"uniforms", "avec2", "3", "y"}}, 4.0);
	commandInterface.set({material, {"uniforms", "avec3", "3", "y"}}, 5.0);
	commandInterface.set({material, {"uniforms", "avec4", "3", "y"}}, 6.0);

	commandInterface.set({material, {"uniforms", "aivec2", "3", "i1"}}, 7);
	commandInterface.set({material, {"uniforms", "aivec3", "3", "i1"}}, 8);
	commandInterface.set({material, {"uniforms", "aivec4", "3", "i1"}}, 9);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	ramses::UniformInput input;

	std::array<int32_t, 2> ivalues;
	appearance->getEffect().findUniformInput("ivec", input);
	appearance->getInputValueInt32(input, 2, ivalues.data());
	EXPECT_EQ(ivalues[1], 2);

	std::array<float, 5> fvalues;
	appearance->getEffect().findUniformInput("fvec", input);
	appearance->getInputValueFloat(input, 5, fvalues.data());
	EXPECT_EQ(fvalues[2], 3.0);

	std::array<float, 8> a2fvalues;
	appearance->getEffect().findUniformInput("avec2", input);
	appearance->getInputValueVector2f(input, 4, a2fvalues.data());
	EXPECT_EQ(a2fvalues[5], 4.0);

	std::array<float, 15> a3fvalues;
	appearance->getEffect().findUniformInput("avec3", input);
	appearance->getInputValueVector3f(input, 5, a3fvalues.data());
	EXPECT_EQ(a3fvalues[7], 5.0);

	std::array<float, 24> a4fvalues;
	appearance->getEffect().findUniformInput("avec4", input);
	appearance->getInputValueVector4f(input, 6, a4fvalues.data());
	EXPECT_EQ(a4fvalues[9], 6.0);

	std::array<int32_t, 8> a2ivalues;
	appearance->getEffect().findUniformInput("aivec2", input);
	appearance->getInputValueVector2i(input, 4, a2ivalues.data());
	EXPECT_EQ(a2ivalues[4], 7);

	std::array<int32_t, 15> a3ivalues;
	appearance->getEffect().findUniformInput("aivec3", input);
	appearance->getInputValueVector3i(input, 5, a3ivalues.data());
	EXPECT_EQ(a3ivalues[6], 8);

	std::array<int32_t, 24> a4ivalues;
	appearance->getEffect().findUniformInput("aivec4", input);
	appearance->getInputValueVector4i(input, 6, a4ivalues.data());
	EXPECT_EQ(a4ivalues[8], 9);
}

TEST_F(MaterialAdaptorTest, link_get_array_uniforms) {
	auto material = create_material("mat", "shaders/uniform-array.vert", "shaders/uniform-array.frag");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	link(lua, {"outputs", "ointeger"}, material, {"uniforms", "ivec", "2"});
	link(lua, {"outputs", "ofloat"}, material, {"uniforms", "fvec", "3"});
	
	link(lua, {"outputs", "ovector2f"}, material, {"uniforms", "avec2", "3"});
	link(lua, {"outputs", "ovector3f"}, material, {"uniforms", "avec3", "3"});
	link(lua, {"outputs", "ovector4f"}, material, {"uniforms", "avec4", "3"});
	
	link(lua, {"outputs", "ovector2i"}, material, {"uniforms", "aivec2", "3"});
	link(lua, {"outputs", "ovector3i"}, material, {"uniforms", "aivec3", "3"});
	link(lua, {"outputs", "ovector4i"}, material, {"uniforms", "aivec4", "3"});

	dispatch();

	commandInterface.set({lua, {"inputs", "integer"}}, 1);
	commandInterface.set({lua, {"inputs", "float"}}, 1.0);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	ramses::UniformInput input;

	std::array<int32_t, 2> ivalues;
	appearance->getEffect().findUniformInput("ivec", input);
	appearance->getInputValueInt32(input, 2, ivalues.data());
	EXPECT_EQ(ivalues[1], 2);

	std::array<float, 5> fvalues;
	appearance->getEffect().findUniformInput("fvec", input);
	appearance->getInputValueFloat(input, 5, fvalues.data());
	EXPECT_EQ(fvalues[2], 1.0);

	std::array<float, 8> a2fvalues;
	appearance->getEffect().findUniformInput("avec2", input);
	appearance->getInputValueVector2f(input, 4, a2fvalues.data());
	EXPECT_EQ(a2fvalues[5], 2.0);

	std::array<float, 15> a3fvalues;
	appearance->getEffect().findUniformInput("avec3", input);
	appearance->getInputValueVector3f(input, 5, a3fvalues.data());
	EXPECT_EQ(a3fvalues[7], 2.0);

	std::array<float, 24> a4fvalues;
	appearance->getEffect().findUniformInput("avec4", input);
	appearance->getInputValueVector4f(input, 6, a4fvalues.data());
	EXPECT_EQ(a4fvalues[9], 2.0);

	std::array<int32_t, 8> a2ivalues;
	appearance->getEffect().findUniformInput("aivec2", input);
	appearance->getInputValueVector2i(input, 4, a2ivalues.data());
	EXPECT_EQ(a2ivalues[4], 1);

	std::array<int32_t, 15> a3ivalues;
	appearance->getEffect().findUniformInput("aivec3", input);
	appearance->getInputValueVector3i(input, 5, a3ivalues.data());
	EXPECT_EQ(a3ivalues[6], 1);

	std::array<int32_t, 24> a4ivalues;
	appearance->getEffect().findUniformInput("aivec4", input);
	appearance->getInputValueVector4i(input, 6, a4ivalues.data());
	EXPECT_EQ(a4ivalues[8], 1);
}