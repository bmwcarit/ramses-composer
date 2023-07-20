/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utils/FileUtils.h"
#include "utils/ShaderPreprocessor.h"
#include "testing/RacoBaseTest.h"

#include <fstream>

#include "gtest/gtest.h"

using namespace raco::utils;
using shader::ShaderPreprocessor;

void fileWriteU8(const u8path& path, std::string_view utf8Content) {
	if (!path.parent_path().existsDirectory()) {
		std::filesystem::create_directory(path.parent_path());
	}
	std::ofstream out{path.internalPath(), std::ofstream::out | std::ofstream::binary};
	out << utf8Content;
	out.close();
}

class ShaderPreprocessorTest : public RacoBaseTest<> {
public:
	// Check that include directive in shaderCode was parsed or rejected
	[[nodiscard]] std::tuple<bool, std::string, std::string> testShaderCode(std::string_view shaderCode, bool expectIncludeParsed, const std::string& expectedIncludedFile = existingInclude_, bool isUtf8Encoded = false) const {
		const auto path = (test_path() / "shaders/tmp.glsl").string();
		if (isUtf8Encoded) {
			fileWriteU8(path, shaderCode);
		} else {
			file::write(path, std::string(shaderCode));
		}
		const auto preprocessor = ShaderPreprocessor(path);
		auto files = preprocessor.getIncludedFiles();
		EXPECT_EQ(files.size(), expectIncludeParsed ? 1 : 0);
		EXPECT_EQ(files.find((test_path() / expectedIncludedFile).string()) != files.end(), expectIncludeParsed);
		return {preprocessor.hasError(), preprocessor.getError(), preprocessor.getProcessedShader()};
	}

	static std::tuple<std::string, std::string, shader::EIncludeSearchResult> ShaderPreprocessor_findInclude(const std::string& line) {
		auto path = std::string{};
		auto error = std::string{};
		auto result = ShaderPreprocessor::findInclude(line, path, error);
		return {path, error, result};
	}

protected:
	static const char* existingInclude_;
};

const char* ShaderPreprocessorTest::existingInclude_ = "shaders/include/func1.glsl";

TEST_F(ShaderPreprocessorTest, noIncludesTest) {
	auto p = test_path();
	ASSERT_TRUE(test_path().exists());
	const auto mainShaderPath = (test_path() / "shaders/basic.frag").string();
	const auto preprocessor = ShaderPreprocessor(mainShaderPath);
	ASSERT_EQ(preprocessor.getProcessedShader(), file::read(mainShaderPath));
}

TEST_F(ShaderPreprocessorTest, basicIncludeTest) {
	ASSERT_TRUE(test_path().exists());
	const auto preprocessor = ShaderPreprocessor((test_path() / "shaders/include/main.glsl").string());
	ASSERT_EQ(preprocessor.getIncludedFiles().size(), 1);
	const auto path = *preprocessor.getIncludedFiles().begin();
	ASSERT_EQ(path, (test_path() / existingInclude_).string());
	ASSERT_NE(preprocessor.getProcessedShader().find("function1()"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, nestedIncludeTest) {
	ASSERT_TRUE(test_path().exists());
	const auto preprocessor = ShaderPreprocessor((test_path() / "shaders/include/main_nested.glsl").string());
	auto files = preprocessor.getIncludedFiles();
	ASSERT_EQ(files.size(), 2);
	ASSERT_NE(files.find((test_path() / existingInclude_).string()), files.end());
	ASSERT_NE(files.find((test_path() / "shaders/include/func2.glsl").string()), files.end());
	ASSERT_NE(preprocessor.getProcessedShader().find("function1()"), std::string::npos);
	ASSERT_NE(preprocessor.getProcessedShader().find("function2()"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, subdirectoryIncludeTest) {
	ASSERT_TRUE(test_path().exists());
	const auto preprocessor = ShaderPreprocessor((test_path() / "shaders/include/main_subdir.glsl").string());
	auto files = preprocessor.getIncludedFiles();
	ASSERT_EQ(files.size(), 1);
	ASSERT_NE(files.find((test_path() / "shaders/include/subdirectory/func3.glsl").string()), files.end());
	ASSERT_NE(preprocessor.getProcessedShader().find("function3()"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, missingIncludedFileTest) {
	auto missingIncludedFile = "missing_folder/include/func1.glsl";

	// Missing file is treated as included to set file watchers
	const auto [hasError, error, _] = testShaderCode(fmt::format("#include \"{}\"", missingIncludedFile), true, (u8path("shaders") / missingIncludedFile).string());
	ASSERT_TRUE(hasError);
	ASSERT_NE(error.find("Cannot open file"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, chinesePathLoaded) {
	const u8path chinesePath{std::filesystem::path(L"\u96E8\u4E2D_1.txt")}; // 雨中;
	const auto chineseFullPath{(test_path() / "shaders/include" / chinesePath).string()};
	file::write(chineseFullPath, "contents1");

	const auto preprocessor = ShaderPreprocessor(chineseFullPath);
	ASSERT_EQ(preprocessor.getIncludedFiles().size(), 0);
	ASSERT_FALSE(preprocessor.hasError());
	ASSERT_NE(preprocessor.getProcessedShader().find("contents1"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, chineseIncludeProcessed) {
	const u8path chinesePath{std::filesystem::path(L"\u96E8\u4E2D_2.txt")}; // 雨中;
	const auto chineseFullPath{(test_path() / "shaders/include" / chinesePath).string()};
	file::write(chineseFullPath, "contents2");

	const auto [hasError, error, preprocessedShader] = testShaderCode(fmt::format("#include \"include/{}\"", chinesePath.string()), true, chineseFullPath, true);
	ASSERT_FALSE(hasError);
	ASSERT_NE(preprocessedShader.find("contents2"), std::string::npos);
}

TEST_F(ShaderPreprocessorTest, includeDuplicateTest) {
	ASSERT_TRUE(test_path().exists());

	// Double include is allowed
	const auto [hasError, error, _] = testShaderCode(
		"\
#include \"include/func1.glsl\"\n\
#include \"include/func1.glsl\"",
		true);
	ASSERT_FALSE(hasError);
}

 TEST_F(ShaderPreprocessorTest, includeLoopTest) {
	// Basic loop is not allowed
	const auto preprocessor = ShaderPreprocessor((test_path() / "shaders/include/loop_main").string());
	// File should be in include list
	ASSERT_EQ(preprocessor.getIncludedFiles().size(), 1);
	// Loop in include files should cause an error
	ASSERT_TRUE(preprocessor.hasError());
	ASSERT_NE(preprocessor.getError().find("Include loop detected"), std::string::npos);
	ASSERT_NE(preprocessor.getError().find("loop_main"), std::string::npos);
}

 TEST_F(ShaderPreprocessorTest, includeDiamondTest) {
	// Basic loop is not allowed
	const auto preprocessor = ShaderPreprocessor((test_path() / "shaders/include/main_diamond.glsl").string());
	ASSERT_FALSE(preprocessor.hasError());
	// Files should be in include list
	auto includedFiles = preprocessor.getIncludedFiles();
	ASSERT_EQ(includedFiles.size(), 3);
	ASSERT_NE(includedFiles.find((test_path() / "shaders/include/func2.glsl").string()), includedFiles.end());
	ASSERT_NE(includedFiles.find((test_path() / "shaders/include/func2_diamond.glsl").string()), includedFiles.end());
	ASSERT_NE(includedFiles.find((test_path() / "shaders/include/func1.glsl").string()), includedFiles.end());
	// func1.glsl should be included twice and its content should be duplicated
	const auto& shader = preprocessor.getProcessedShader();
	const auto firstInclusion = shader.find("function1()");
	ASSERT_NE(firstInclusion, std::string::npos);
	ASSERT_TRUE(firstInclusion + 1 < shader.length());
	ASSERT_NE(shader.find("function1()", firstInclusion + 1), std::string::npos);
 }

TEST_F(ShaderPreprocessorTest, findIncludeTest) {
	auto [path, error, result] = ShaderPreprocessor_findInclude("");
	ASSERT_EQ(result, shader::EIncludeSearchResult::NotFound);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "word space")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "word space");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "word space")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "word space");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(	 #include "word"	)");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "word");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "word" //a)");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "word");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(//#include "word")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::NotFound);

	// Path is not validated when extracted from an #include directive
	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "not//comment")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "not//comment");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(	#include "../some\path_to/file 1.inc" /// all at once //)");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, R"(../some\path_to/file 1.inc)");

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "a1-+_ .")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, "a1-+_ .");

	// Some Chinese characters. Any but specifically prohibited characters in the path are allowed.
	std::tie(path, error, result) = ShaderPreprocessor_findInclude(u8"#include \"\u96E8\u4E2D\"");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Found);
	ASSERT_EQ(path, u8"\u96E8\u4E2D");
}

 TEST_F(ShaderPreprocessorTest, illFormedIncludeRejectionTest) {
	// Error is expected if a line contains ill-formed code which can be interpreted
	// as #include directive by shader compiler.
	auto [path, error, result] = ShaderPreprocessor_findInclude("#include no_quotes");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
	ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include"")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
	ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include ")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
	ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(a	 #include "word")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::NotFound);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include "word" a)");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
	ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);

	std::tie(path, error, result) = ShaderPreprocessor_findInclude(R"(#include //"word")");
	ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
	ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);

	// Non-space whitespace
	std::string prohibitedInPathCharacters{"<>:\"|?*\t"};
	for (const auto c : prohibitedInPathCharacters) {
		std::tie(path, error, result) = ShaderPreprocessor_findInclude(fmt::format("#include \"bad{}character\"", c));
		ASSERT_EQ(result, shader::EIncludeSearchResult::Error);
		ASSERT_NE(error.find("Ill-formed include directive"), std::string::npos);
	}
 }

TEST_F(ShaderPreprocessorTest, chineseWontCrashTrim) {
	// This caused debug assertion in std::isspace() due to per-character processing of UTF-8 encoded text
	auto [path, error, result] = ShaderPreprocessor_findInclude(u8"\u96E8\u4E2D arbitrary text without include directive");
	ASSERT_EQ(result, shader::EIncludeSearchResult::NotFound);
 }
