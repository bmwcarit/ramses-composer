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
#include "testing/TestUtil.h"
#include "user_types/Material.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class MaterialTest : public TestEnvironmentCore {
public:
	std::string shaderWhitespaceOnlyFile;
	std::string shaderEmptyFile;
	std::string shaderDefinesFile;
	std::string shaderWithErrorIfDefineFile;

	SEditorObject material;

	ValueHandle vertexUriHandle;
	ValueHandle geometryUriHandle;
	ValueHandle fragmentUriHandle;
	ValueHandle definesUriHandle;

	void SetUp() {
		TestEnvironmentCore::SetUp();
		shaderWhitespaceOnlyFile = makeFile("whitespace.glsl", " ");
		shaderEmptyFile = makeFile("empty.glsl", "");
		shaderDefinesFile = makeFile("shaderdefines.def", "OINK\n\nBLUBB");
		shaderWithErrorIfDefineFile = makeFile("shaderwithdefineerror.glsl", "#ifdef OINK\n#error MyError\n#endif\nvoid main() {}");

		material = commandInterface.createObject(Material::typeDescription.typeName);

		vertexUriHandle = {material, &Material::uriVertex_};
		fragmentUriHandle = {material, &Material::uriFragment_};
		geometryUriHandle = {material, &Material::uriGeometry_};
		definesUriHandle = {material, &Material::uriDefines_};
	}

	void verifyFileWatcher(std::string_view relPath) {
		recorder.reset();

		// Modify shader file
		std::ofstream out{(test_path() / relPath).internalPath(), std::ifstream::out | std::ifstream::app};
		out << "// appended comment" << std::endl;
		out.close();

		// Expect file watcher was executed and shader was reloaded
		EXPECT_TRUE(raco::awaitPreviewDirty(recorder, material, 5));
	}
};

TEST_F(MaterialTest, validShader) {
	commandInterface.set(vertexUriHandle, (test_path() / "shaders/basic.vert").string());
	commandInterface.set(fragmentUriHandle, (test_path() / "shaders/basic.frag").string());
	ASSERT_FALSE(commandInterface.errors().hasError(material));
	ASSERT_FALSE(commandInterface.errors().hasError(vertexUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(fragmentUriHandle));
}

TEST_F(MaterialTest, uniform_array) {
	auto mat = create_material("material", (test_path() / "shaders/uniform-array.vert").string(), (test_path() / "shaders/uniform-array.frag").string());

	auto checkArray = [](const Table& container, std::string propName, int size, PrimitiveType type) {
		EXPECT_TRUE(container.hasProperty(propName));
		EXPECT_EQ(container.get(propName)->type(), PrimitiveType::Table);
		auto& array = container.get(propName)->asTable();
		EXPECT_EQ(array.size(), size);
		for (int index = 0; index < size; index++) {
			EXPECT_EQ(array.name(index), std::to_string(index + 1));
			EXPECT_EQ(array.get(index)->type(), type);
		}
	};

	checkArray(*mat->uniforms_, "ivec", 2, PrimitiveType::Int);
	checkArray(*mat->uniforms_, "fvec", 5, PrimitiveType::Double);

	checkArray(*mat->uniforms_, "avec2", 4, PrimitiveType::Struct);
	checkArray(*mat->uniforms_, "avec3", 5, PrimitiveType::Struct);
	checkArray(*mat->uniforms_, "avec4", 6, PrimitiveType::Struct);

	checkArray(*mat->uniforms_, "aivec2", 4, PrimitiveType::Struct);
	checkArray(*mat->uniforms_, "aivec3", 5, PrimitiveType::Struct);
	checkArray(*mat->uniforms_, "aivec4", 6, PrimitiveType::Struct);
}

TEST_F(MaterialTest, settingShaderShouldAutomaticallySetOtherShadersIfPresent) {
	auto def = makeFile("shaders/basic.def", "");

	commandInterface.set(fragmentUriHandle, (test_path() / "shaders/basic.frag").string());
	ASSERT_FALSE(commandInterface.errors().hasError(material));
	ASSERT_FALSE(commandInterface.errors().hasError(vertexUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(fragmentUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(definesUriHandle));

	ASSERT_EQ((test_path() / "shaders/basic.vert").string(), material->get("uriVertex")->asString());
	ASSERT_EQ((test_path() / "shaders/basic.frag").string(), material->get("uriFragment")->asString());
	ASSERT_EQ(def.path.string(), material->get("uriDefines")->asString());
	ASSERT_TRUE(material->get("uriGeometry")->asString().empty());
}

TEST_F(MaterialTest, settingShaderShouldAutomaticallySetOtherShadersIfPresentWithGLSLEndings) {
	auto vert = makeFile("testShader_vert.glsl", "");
	auto frag = makeFile("testShader_frag.glsl", "");
	auto geom = makeFile("testShader_geom.glsl", "");

	commandInterface.set(vertexUriHandle, vert);

	ASSERT_EQ(vert.path.string(), material->get("uriVertex")->asString());
	ASSERT_EQ(frag.path.string(), material->get("uriFragment")->asString());
	ASSERT_EQ(geom.path.string(), material->get("uriGeometry")->asString());
	ASSERT_EQ("", material->get("uriDefines")->asString());
}

TEST_F(MaterialTest, settingShaderShouldNotAutomaticallySetOtherShadersIfOneIsAlreadySet) {
	auto vertWithDifferentName = makeFile("testShader_bla.glsl", "");
	auto vert = makeFile("testShader_vert.glsl", "");
	auto frag = makeFile("testShader_frag.glsl", "");
	auto geom = makeFile("testShader_geom.glsl", "");

	commandInterface.set(vertexUriHandle, vertWithDifferentName);
	commandInterface.set(fragmentUriHandle, frag);

	ASSERT_EQ(vertWithDifferentName.path.string(), material->get("uriVertex")->asString());
	ASSERT_EQ(frag.path.string(), material->get("uriFragment")->asString());
	ASSERT_EQ("", material->get("uriGeometry")->asString());
}

TEST_F(MaterialTest, shortInvalidShaderUri) {
	std::string uri = "abc";
	commandInterface.set(vertexUriHandle, uri);
}

TEST_F(MaterialTest, errorEmptyVertexShader) {
	commandInterface.set(fragmentUriHandle, (test_path() / "shaders/basic.frag").string());
	commandInterface.set(vertexUriHandle, shaderEmptyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] Shader Program Linker Error:\nERROR: Linking vertex stage: Missing entry point: Each stage requires one entry point\n\n");
	commandInterface.set(vertexUriHandle, shaderWhitespaceOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] Shader Program Linker Error:\nERROR: Linking vertex stage: Missing entry point: Each stage requires one entry point\n\n");
}

TEST_F(MaterialTest, errorEmptyFragmentShader) {
	commandInterface.set(vertexUriHandle, (test_path() / "shaders/basic.vert").string());
	commandInterface.set(fragmentUriHandle, shaderEmptyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] Shader Program Linker Error:\nERROR: Linking fragment stage: Missing entry point: Each stage requires one entry point\n\n");
	commandInterface.set(fragmentUriHandle, shaderWhitespaceOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] Shader Program Linker Error:\nERROR: Linking fragment stage: Missing entry point: Each stage requires one entry point\n\n");
}

TEST_F(MaterialTest, errorEmptyGeometryShader) {
	commandInterface.set(vertexUriHandle, (test_path() / "shaders/basic.vert").string());
	commandInterface.set(fragmentUriHandle, (test_path() / "shaders/basic.frag").string());
	ASSERT_FALSE(commandInterface.errors().hasError(material));

	commandInterface.set(geometryUriHandle, shaderEmptyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] geometry shader Shader Parsing Error:\nERROR: #version: geometry shaders require es profile with version 310 or non-es profile with version 150 or above\nERROR: 0:1: '' : array size must be a positive integer\nERROR: 0:1: '' : compilation terminated \nINTERNAL ERROR: Unable to parse built-ins\nERROR: 1 compilation errors.  No code generated.\n\n\n");
	commandInterface.set(geometryUriHandle, shaderWhitespaceOnlyFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] geometry shader Shader Parsing Error:\nERROR: #version: geometry shaders require es profile with version 310 or non-es profile with version 150 or above\nERROR: 0:1: '' : array size must be a positive integer\nERROR: 0:1: '' : compilation terminated \nINTERNAL ERROR: Unable to parse built-ins\nERROR: 1 compilation errors.  No code generated.\n\n\n");
}

TEST_F(MaterialTest, shaderDefines) {
	commandInterface.set(vertexUriHandle, shaderWithErrorIfDefineFile);
	commandInterface.set(fragmentUriHandle, shaderWithErrorIfDefineFile);
	commandInterface.set(definesUriHandle, shaderEmptyFile);
	ASSERT_FALSE(commandInterface.errors().hasError(material));
	ASSERT_FALSE(commandInterface.errors().hasError(vertexUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(fragmentUriHandle));
	commandInterface.set(definesUriHandle, shaderDefinesFile);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_EQ(commandInterface.errors().getError(material).message(), "[GLSL Compiler] vertex shader Shader Parsing Error:\nERROR: 2:2: '#error' : MyError  \nERROR: 2:3: '' : missing #endif \nERROR: 2:3: '' : compilation terminated \nERROR: 3 compilation errors.  No code generated.\n\n\n");
	ASSERT_FALSE(commandInterface.errors().hasError(vertexUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(fragmentUriHandle));
	commandInterface.set(definesUriHandle, shaderWhitespaceOnlyFile);
	ASSERT_FALSE(commandInterface.errors().hasError(material));
	ASSERT_FALSE(commandInterface.errors().hasError(vertexUriHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(fragmentUriHandle));
}

// Verify that Material compiles preprocessed shaders of each type
TEST_F(MaterialTest, preprocessorHandlesShaderIncludes) {
	makeFile("file_error.glsl", "source code with error");
	auto vertInvalid = makeFile("testShader_vert.glsl", "#include \"file_error.glsl\"");
	auto fragInvalid = makeFile("testShader_frag.glsl", "#include \"file_error.glsl\"");
	auto geomInvalid = makeFile("testShader_geom.glsl", "#include \"file_error.glsl\"");
	auto vertValid = makeFile("valid_vert.glsl", "#include \"shaders/basic.vert\"");
	auto fragValid = makeFile("valid_frag.glsl", "#include \"shaders/basic.frag\"");

	// Preprocessor replaces #include directives with incorrect source code
	commandInterface.set(vertexUriHandle, vertInvalid);
	commandInterface.set(fragmentUriHandle, fragInvalid);
	commandInterface.set(fragmentUriHandle, geomInvalid);

	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_NE(commandInterface.errors().getError(material).message().find("[GLSL Compiler] vertex shader Shader Parsing Error"), std::string::npos);

	// Fix vertex shader
	commandInterface.set(vertexUriHandle, vertValid);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_NE(commandInterface.errors().getError(material).message().find("[GLSL Compiler] fragment shader Shader Parsing Error"), std::string::npos);

	// Fix fragment shader
	commandInterface.set(fragmentUriHandle, fragValid);
	ASSERT_TRUE(commandInterface.errors().hasError(material));
	ASSERT_NE(commandInterface.errors().getError(material).message().find("[GLSL Compiler] geometry shader Shader Parsing Error"), std::string::npos);

	// Fix geometry shader
	commandInterface.set(geometryUriHandle, std::string());
	ASSERT_FALSE(commandInterface.errors().hasError(material));
}

#if (!defined (__linux__))
// awaitPreviewDirty does not work in Linux as expected. See RAOS-692

TEST_F(MaterialTest, shaderFileWatcherTest) {
	const std::vector<std::string> files{
		"shaders/include/main_mat_adapter_test.vert",
		"shaders/include/main_mat_adapter_test.frag",
		"shaders/include/main_mat_adapter_test.geom",
		"shaders/include/uniforms_vert.glsl",
		"shaders/include/uniforms_frag.glsl",
		"shaders/include/uniforms_geom.glsl"
	};

	commandInterface.set(vertexUriHandle, (test_path() / files[0]).string());
	commandInterface.set(fragmentUriHandle, (test_path() / files[1]).string());
	commandInterface.set(geometryUriHandle, (test_path() / files[2]).string());

	ASSERT_FALSE(commandInterface.errors().hasError(material));

	for (const auto& file : files) {
		verifyFileWatcher(file);
	}
}

#endif
