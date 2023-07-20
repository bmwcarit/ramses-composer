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

#include "AdaptorTestUtils.h"
#include "MaterialAdaptorTestBase.h"

#include "ramses_adaptor/MaterialAdaptor.h"

#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/TextureExternal.h"

using namespace raco::user_types;

class MaterialAdaptorTest : public MaterialAdaptorTestBase {};

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

	setStructComponents({material, {"uniforms"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, {}, default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_scalar_uniforms) {
	auto material = create_material("mat", "shaders/uniform-scalar.vert", "shaders/uniform-scalar.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	linkStructComponents({interface, {"inputs", "s_prims"}}, {material, {"uniforms"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms"}}, appearance, {}, default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_array_uniforms) {
	auto material = create_material("mat", "shaders/uniform-array.vert", "shaders/uniform-array.frag");

	commandInterface.set({material, {"uniforms", "ivec", "2"}}, 2);
	commandInterface.set({material, {"uniforms", "fvec", "3"}}, 3.0);

	commandInterface.set({material, {"uniforms", "avec2", "4", "y"}}, 4.0);
	commandInterface.set({material, {"uniforms", "avec3", "5", "z"}}, 5.0);
	commandInterface.set({material, {"uniforms", "avec4", "6", "w"}}, 6.0);

	commandInterface.set({material, {"uniforms", "aivec2", "4", "i2"}}, 7);
	commandInterface.set({material, {"uniforms", "aivec3", "5", "i3"}}, 8);
	commandInterface.set({material, {"uniforms", "aivec4", "6", "i4"}}, 9);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkUniformVector<int32_t, 2>(appearance, "ivec", 1, 2);
	checkUniformVector<float, 5>(appearance, "fvec", 2, 3.0);

	checkUniformVector<std::array<float, 2>, 4>(appearance, "avec2", 3, {0.0, 4.0});
	checkUniformVector<std::array<float, 3>, 5>(appearance, "avec3", 4, {0.0, 0.0, 5.0});
	checkUniformVector<std::array<float, 4>, 6>(appearance, "avec4", 5, {0.0, 0.0, 0.0, 6.0});

	checkUniformVector<std::array<int32_t, 2>, 4>(appearance, "aivec2", 3, {0, 7});
	checkUniformVector<std::array<int32_t, 3>, 5>(appearance, "aivec3", 4, {0, 0, 8});
	checkUniformVector<std::array<int32_t, 4>, 6>(appearance, "aivec4", 5, {0, 0, 0, 9});
}

TEST_F(MaterialAdaptorTest, link_get_array_uniforms_components) {
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

	checkUniformVector<int32_t, 2>(appearance, "ivec", 1, 2);
	checkUniformVector<float, 5>(appearance, "fvec", 2, 1.0);

	checkUniformVector<std::array<float, 2>, 4>(appearance, "avec2", 2, {1.0, 2.0});
	checkUniformVector<std::array<float, 3>, 5>(appearance, "avec3", 2, {1.0, 2.0, 3.0});
	checkUniformVector<std::array<float, 4>, 6>(appearance, "avec4", 2, {1.0, 2.0, 3.0, 4.0});

	checkUniformVector<std::array<int32_t, 2>, 4>(appearance, "aivec2", 2, {1, 2});
	checkUniformVector<std::array<int32_t, 3>, 5>(appearance, "aivec3", 2, {1, 2, 3});
	checkUniformVector<std::array<int32_t, 4>, 6>(appearance, "aivec4", 2, {1, 2, 3, 4});
}

TEST_F(MaterialAdaptorTest, link_get_array_uniforms_array) {
	auto material = create_material("mat", "shaders/uniform-array.vert", "shaders/uniform-array.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-array.lua");

	link(interface, {"inputs", "ivec"}, material, {"uniforms", "ivec"});
	link(interface, {"inputs", "fvec"}, material, {"uniforms", "fvec"});

	link(interface, {"inputs", "avec2"}, material, {"uniforms", "avec2"});
	link(interface, {"inputs", "avec3"}, material, {"uniforms", "avec3"});
	link(interface, {"inputs", "avec4"}, material, {"uniforms", "avec4"});

	link(interface, {"inputs", "aivec2"}, material, {"uniforms", "aivec2"});
	link(interface, {"inputs", "aivec3"}, material, {"uniforms", "aivec3"});
	link(interface, {"inputs", "aivec4"}, material, {"uniforms", "aivec4"});

	commandInterface.set({interface, {"inputs", "ivec", "2"}}, 2);
	commandInterface.set({interface, {"inputs", "fvec", "3"}}, 3.0);

	commandInterface.set({interface, {"inputs", "avec2", "4", "y"}}, 4.0);
	commandInterface.set({interface, {"inputs", "avec3", "5", "z"}}, 5.0);
	commandInterface.set({interface, {"inputs", "avec4", "6", "w"}}, 6.0);

	commandInterface.set({interface, {"inputs", "aivec2", "4", "i2"}}, 7);
	commandInterface.set({interface, {"inputs", "aivec3", "5", "i3"}}, 8);
	commandInterface.set({interface, {"inputs", "aivec4", "6", "i4"}}, 9);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkUniformVector<int32_t, 2>(appearance, "ivec", 1, 2);
	checkUniformVector<float, 5>(appearance, "fvec", 2, 3.0);

	checkUniformVector<std::array<float, 2>, 4>(appearance, "avec2", 3, {0.0, 4.0});
	checkUniformVector<std::array<float, 3>, 5>(appearance, "avec3", 4, {0.0, 0.0, 5.0});
	checkUniformVector<std::array<float, 4>, 6>(appearance, "avec4", 5, {0.0, 0.0, 0.0, 6.0});

	checkUniformVector<std::array<int32_t, 2>, 4>(appearance, "aivec2", 3, {0, 7});
	checkUniformVector<std::array<int32_t, 3>, 5>(appearance, "aivec3", 4, {0, 0, 8});
	checkUniformVector<std::array<int32_t, 4>, 6>(appearance, "aivec4", 5, {0, 0, 0, 9});
}

TEST_F(MaterialAdaptorTest, set_get_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-samplers.vert", "shaders/uniform-samplers.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, set_get_sampler_external_uniforms) {
	auto material = create_material("mat", "shaders/texture-external.vert", "shaders/texture-external.frag");
	auto texture = create<TextureExternal>("texture");

	commandInterface.set({material, {"uniforms", "utex"}}, texture);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSamplerExternal>(*sceneContext.scene(), "texture");

	ramses::UniformInput input;

	const ramses::TextureSamplerExternal* u_sampler_texture;
	EXPECT_EQ(ramses::StatusOK, appearance->getEffect().findUniformInput("utex", input));
	EXPECT_EQ(ramses::StatusOK, appearance->getInputTextureExternal(input, u_sampler_texture));
	EXPECT_EQ(u_sampler_texture, engineTexture);
}

TEST_F(MaterialAdaptorTest, set_get_struct_prim_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	setStructComponents({material, {"uniforms", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_prim_uniforms_link_struct) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_prims"}, material, {"uniforms", "s_prims"});

	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_prims"}}, appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_prim_uniforms_link_members) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	linkStructComponents({interface, {"inputs", "s_prims"}}, {material, {"uniforms", "s_prims"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_prims"}}, appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_struct_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "s_samplers", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "s_samplers", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "s_samplers", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "s_samplers", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_samplers.s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_samplers.s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_samplers.s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "s_samplers.s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, set_get_nested_struct_prim_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	setStructComponents({material, {"uniforms", "nested", "prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, "nested.prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_nested_struct_prim_uniforms_link_struct) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_prims"}, material, {"uniforms", "nested", "prims"});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "nested", "prims"}}, appearance, "nested.prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_nested_struct_prim_uniforms_link_members) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	linkStructComponents({interface, {"inputs", "s_prims"}}, {material, {"uniforms", "nested", "prims"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "nested", "prims"}}, appearance, "nested.prims.", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_nested_struct_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "nested", "samplers", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "nested", "samplers", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "nested", "samplers", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "nested", "samplers", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "nested.samplers.s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "nested.samplers.s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "nested.samplers.s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "nested.samplers.s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, set_get_array_struct_prim_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	setStructComponents({material, {"uniforms", "a_s_prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, "a_s_prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_array_struct_prim_uniforms_link_array) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "a_s_prims"}, material, {"uniforms", "a_s_prims"});
	setStructComponents({interface, {"inputs", "a_s_prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "a_s_prims", "2"}}, appearance, "a_s_prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_array_struct_prim_uniforms_link_struct) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_prims"}, material, {"uniforms", "a_s_prims", "2"});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "a_s_prims", "2"}}, appearance, "a_s_prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_array_struct_prim_uniforms_link_members) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	linkStructComponents({interface, {"inputs", "s_prims"}}, {material, {"uniforms", "a_s_prims", "2"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "a_s_prims", "2"}}, appearance, "a_s_prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_array_struct_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "a_s_samplers", "2", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "a_s_samplers", "2", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "a_s_samplers", "2", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "a_s_samplers", "2", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_samplers[1].s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_samplers[1].s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_samplers[1].s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "a_s_samplers[1].s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, set_get_struct_of_array_of_prim_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	commandInterface.set({material, {"uniforms", "s_a_prims", "ivec", "2"}}, 2);
	commandInterface.set({material, {"uniforms", "s_a_prims", "fvec", "3"}}, 3.0);

	commandInterface.set({material, {"uniforms", "s_a_prims", "avec2", "4", "y"}}, 4.0);
	commandInterface.set({material, {"uniforms", "s_a_prims", "avec3", "5", "z"}}, 5.0);
	commandInterface.set({material, {"uniforms", "s_a_prims", "avec4", "6", "w"}}, 6.0);

	commandInterface.set({material, {"uniforms", "s_a_prims", "aivec2", "4", "i2"}}, 7);
	commandInterface.set({material, {"uniforms", "s_a_prims", "aivec3", "5", "i3"}}, 8);
	commandInterface.set({material, {"uniforms", "s_a_prims", "aivec4", "6", "i4"}}, 9);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkUniformVector<int32_t, 2>(appearance, "s_a_prims.ivec", 1, 2);
	checkUniformVector<float, 5>(appearance, "s_a_prims.fvec", 2, 3.0);

	checkUniformVector<std::array<float, 2>, 4>(appearance, "s_a_prims.avec2", 3, {0.0, 4.0});
	checkUniformVector<std::array<float, 3>, 5>(appearance, "s_a_prims.avec3", 4, {0.0, 0.0, 5.0});
	checkUniformVector<std::array<float, 4>, 6>(appearance, "s_a_prims.avec4", 5, {0.0, 0.0, 0.0, 6.0});

	checkUniformVector<std::array<int32_t, 2>, 4>(appearance, "s_a_prims.aivec2", 3, {0, 7});
	checkUniformVector<std::array<int32_t, 3>, 5>(appearance, "s_a_prims.aivec3", 4, {0, 0, 8});
	checkUniformVector<std::array<int32_t, 4>, 6>(appearance, "s_a_prims.aivec4", 5, {0, 0, 0, 9});
}

TEST_F(MaterialAdaptorTest, set_get_struct_of_array_of_struct_prims_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	setStructComponents({material, {"uniforms", "s_a_struct_prim", "prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, "s_a_struct_prim.prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_of_array_of_struct_prims_uniforms_link_struct_outer) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_a_struct_prim"}, material, {"uniforms", "s_a_struct_prim"});
	setStructComponents({interface, {"inputs", "s_a_struct_prim", "prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_a_struct_prim", "prims", "2"}}, appearance, "s_a_struct_prim.prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_of_array_of_struct_prims_uniforms_link_array) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_a_struct_prim", "prims"}, material, {"uniforms", "s_a_struct_prim", "prims"});
	setStructComponents({interface, {"inputs", "s_a_struct_prim", "prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_a_struct_prim", "prims", "2"}}, appearance, "s_a_struct_prim.prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_of_array_of_struct_prims_uniforms_link_struct_inner) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	link(interface, {"inputs", "s_prims"}, material, {"uniforms", "s_a_struct_prim", "prims", "2"});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_a_struct_prim", "prims", "2"}}, appearance, "s_a_struct_prim.prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, link_get_struct_of_array_of_struct_prims_uniforms_link_members) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	linkStructComponents({interface, {"inputs", "s_prims"}}, {material, {"uniforms", "s_a_struct_prim", "prims", "2"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents({material, {"uniforms", "s_a_struct_prim", "prims", "2"}}, appearance, "s_a_struct_prim.prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_struct_of_array_of_struct_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "s_a_struct_samplers", "samplers", "2", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "s_a_struct_samplers", "samplers", "2", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "s_a_struct_samplers", "samplers", "2", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "s_a_struct_samplers", "samplers", "2", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_a_struct_samplers.samplers[1].s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_a_struct_samplers.samplers[1].s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "s_a_struct_samplers.samplers[1].s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "s_a_struct_samplers.samplers[1].s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, set_get_array_of_struct_of_array_of_struct_prims_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");

	setStructComponents({material, {"uniforms", "a_s_a_struct_prim", "2", "prims", "2"}}, default_struct_prim_values);
	
	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, "a_s_a_struct_prim[1].prims[1].", default_struct_prim_values);
}

TEST_F(MaterialAdaptorTest, set_get_array_of_struct_of_array_of_struct_sampler_uniforms) {
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto texture = create_texture("texture", "images/DuckCM.png");
	auto buffer = create<RenderBuffer>("buffer");
	auto buffer_ms = create<RenderBufferMS>("buffer_ms");
	auto cubeMap = create_cubemap("cubemap", "images/blue_1024.png");

	commandInterface.set({material, {"uniforms", "a_s_a_struct_samplers", "2", "samplers", "2", "s_texture"}}, texture);
	commandInterface.set({material, {"uniforms", "a_s_a_struct_samplers", "2", "samplers", "2", "s_cubemap"}}, cubeMap);
	commandInterface.set({material, {"uniforms", "a_s_a_struct_samplers", "2", "samplers", "2", "s_buffer"}}, buffer);
	commandInterface.set({material, {"uniforms", "a_s_a_struct_samplers", "2", "samplers", "2", "s_buffer_ms"}}, buffer_ms);

	dispatch();

	auto appearance = select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance");
	auto engineTexture = select<ramses::TextureSampler>(*sceneContext.scene(), "texture");
	auto engine_cubeMap = select<ramses::TextureSampler>(*sceneContext.scene(), "cubemap");
	auto engine_buffer = select<ramses::TextureSampler>(*sceneContext.scene(), "buffer");
	auto engine_buffer_ms = select<ramses::TextureSamplerMS>(*sceneContext.scene(), "buffer_ms");

	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_a_struct_samplers[1].samplers[1].s_texture", engineTexture);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_a_struct_samplers[1].samplers[1].s_cubemap", engine_cubeMap);
	checkUniformScalar<const ramses::TextureSampler*>(appearance, "a_s_a_struct_samplers[1].samplers[1].s_buffer", engine_buffer);
	checkUniformScalar<const ramses::TextureSamplerMS*>(appearance, "a_s_a_struct_samplers[1].samplers[1].s_buffer_ms", engine_buffer_ms);
}

TEST_F(MaterialAdaptorTest, shaders_are_preprocessed) {
	auto material = create_material("mat",
		"shaders/include/main_mat_adapter_test.vert",
		"shaders/include/main_mat_adapter_test.frag",
		"shaders/include/main_mat_adapter_test.geom");

	setStructComponents({material, {"uniforms"}}, default_struct_prim_values);

	dispatch();

	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};

	checkStructComponents(appearance, {}, default_struct_prim_values);
}
