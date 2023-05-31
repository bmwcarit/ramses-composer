/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"
#include "user_types/MeshNode.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class MeshNodeTest : public TestEnvironmentCore {
protected:
	void setMaterialOptions(raco::user_types::SMaterial& material) {
		commandInterface.set({material, {"options", "blendOperationColor"}}, 3);
		commandInterface.set({material, {"options", "blendOperationAlpha"}}, 2);
		commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 8);
		commandInterface.set({material, {"options", "blendFactorDestColor"}}, 10);
		commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 5);
		commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 4);
		commandInterface.set({material, {"options", "blendColor", "x"}}, 0.2);
		commandInterface.set({material, {"options", "blendColor", "y"}}, 0.4);
		commandInterface.set({material, {"options", "blendColor", "z"}}, 0.6);
		commandInterface.set({material, {"options", "blendColor", "w"}}, 0.8);
		commandInterface.set({material, {"options", "depthwrite"}}, false);
		commandInterface.set({material, {"options", "depthFunction"}}, 0);
		commandInterface.set({material, {"options", "cullmode"}}, 1);
	}
};

TEST_F(MeshNodeTest, private_material_options_get_taken_over_from_shared_material) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}

TEST_F(MeshNodeTest, private_material_new_options_get_taken_from_shared_material_after_reprivatizing) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);


	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);

	commandInterface.set({material, {"options", "blendOperationColor"}}, 4);
	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 3);
	commandInterface.set({material, {"options", "blendFactorDestColor"}}, 5);
	commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 0);
	commandInterface.set({material, {"options", "blendColor", "x"}}, 0.1);
	commandInterface.set({material, {"options", "blendColor", "y"}}, 0.3);
	commandInterface.set({material, {"options", "blendColor", "z"}}, 0.5);
	commandInterface.set({material, {"options", "blendColor", "w"}}, 0.6);
	commandInterface.set({material, {"options", "depthwrite"}}, true);
	commandInterface.set({material, {"options", "depthFunction"}}, 1);
	commandInterface.set({material, {"options", "cullmode"}}, 0);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 4);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 0);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.1);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.3);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.5);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.6);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), true);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 0);
}

TEST_F(MeshNodeTest, private_material_options_get_overwritten_from_shared_material_after_reprivatizing) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);

	commandInterface.set(privateMaterialOptionsHandle.get("blendOperationColor"), 4);
	commandInterface.set(privateMaterialOptionsHandle.get("blendOperationAlpha"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorSrcColor"), 3);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorDestColor"), 5);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorSrcAlpha"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorDestAlpha"), 0);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("x"), 0.1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("y"), 0.3);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("z"), 0.5);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("w"), 0.6);
	commandInterface.set(privateMaterialOptionsHandle.get("depthwrite"), true);
	commandInterface.set(privateMaterialOptionsHandle.get("depthFunction"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("cullmode"), 0);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}

TEST_F(MeshNodeTest, private_material_options_do_not_get_updated_from_shared_material_to_active_private_material) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	commandInterface.set({material, {"options", "blendOperationColor"}}, 4);
	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 3);
	commandInterface.set({material, {"options", "blendFactorDestColor"}}, 5);
	commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 0);
	commandInterface.set({material, {"options", "blendColor", "x"}}, 0.1);
	commandInterface.set({material, {"options", "blendColor", "y"}}, 0.3);
	commandInterface.set({material, {"options", "blendColor", "z"}}, 0.5);
	commandInterface.set({material, {"options", "blendColor", "w"}}, 0.6);
	commandInterface.set({material, {"options", "depthwrite"}}, true);
	commandInterface.set({material, {"options", "depthFunction"}}, 1);
	commandInterface.set({material, {"options", "cullmode"}}, 0);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}

TEST_F(MeshNodeTest, struct_uniform_value_caching) {
	TextFile vertShader = makeFile("shader.vert", R"(
#version 300 es
precision mediump float;
in vec3 a_Position;
uniform mat4 u_MVPMatrix;
void main() {
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}
)");

	TextFile fragShader = makeFile("shader.frag", R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
struct point {
	float x;
	float y;
};
uniform float a;
uniform point p;
void main() {
})");

	std::string altFragShader = makeFile("altshader.frag", R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
struct point {
	float x;
	float y;
};
uniform point p;
void main() {
})");

	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("material", vertShader, fragShader);
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);
	
	commandInterface.set(raco::core::ValueHandle{material, {"uniforms", "p", "x"}}, 0.5);
	commandInterface.set(raco::core::ValueHandle{meshnode, {"materials", "material", "uniforms", "p", "x"}}, 2.0);
	
	commandInterface.set({material, &Material::uriFragment_}, altFragShader);

	EXPECT_FALSE(raco::core::ValueHandle(meshnode, {"materials", "material", "uniforms"}).hasProperty("a"));
	// Check tha we keep the uniform value of the meshnode and don't copy the value from the material instead:
	EXPECT_EQ(raco::core::ValueHandle(meshnode, {"materials", "material", "uniforms", "p", "x"}).asDouble(), 2.0);
}