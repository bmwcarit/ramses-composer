/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "utils/ShaderPreprocessor.h"

#include <fstream>
#include <sstream>
#include <regex>

namespace raco::utils::shader {

namespace {

class RecursionScopeGuard {
public:
	RecursionScopeGuard(std::unordered_set<std::string>& visitedPaths, const std::string& path)
		: path_(path),
		  visitedFiles_(visitedPaths) {
		if (visitedPaths.count(path)) {
			isLoopDetected_ = true;
			return;
		}
		visitedFiles_.emplace(path);
	}

	~RecursionScopeGuard() {
		if (!isLoopDetected_) {
			visitedFiles_.erase(path_);
		}
	}

	[[nodiscard]] bool isLoopDetected() const {
		return isLoopDetected_;
	}

private:
	bool isLoopDetected_ = false;
	std::string path_;
	std::unordered_set<std::string>& visitedFiles_;
};

}  // namespace

ShaderPreprocessor::ShaderPreprocessor(const std::string& mainShaderPath) {
	processShaderFile(mainShaderPath);
}

const std::string& ShaderPreprocessor::getProcessedShader() const {
	return processedShader_;
}

const std::set<std::string>& ShaderPreprocessor::getIncludedFiles() const {
	return includedFiles_;
}

EIncludeSearchResult ShaderPreprocessor::findInclude(const std::string& line, std::string& foundPath, std::string& error) {
	foundPath = "";
	error = "";

	// Check if this line has a non-commented #include directive.
	auto trimmedLine{line};
	trimmedLine.erase(trimmedLine.begin(), std::find_if(trimmedLine.begin(), trimmedLine.end(), [](const char c) {
		return !std::isspace(static_cast<unsigned char>(c));
	}));

	if (trimmedLine.find("#include") != 0) {
		// No #include directives in this line.
		return EIncludeSearchResult::NotFound;
	}

	// ^\s* - any number of whitespace in the beginning
	// #include\s* - directive followed by optional whitespaces
	// "( - include path inside a group () started
	// [^\<\>:"\|\?\*\t]+ - any character except <>:"|?*\t - at least once
	// )" - include path inside a group () finished
	// \s* - optional whitespaces after path
	// (//.*)?$ - zero or one comments starting with // and followed by any characters
	const std::regex r(R"lit(^\s*#include\s*"([^\<\>:"\|\?\*\t]+)"\s*(//.*)?$)lit",
		std::regex_constants::ECMAScript | std::regex_constants::icase);

	std::smatch match;
	if (std::regex_match(line, match, r) && !match.empty()) {
		// Group 1 contains included path
		foundPath = match[1];
		return EIncludeSearchResult::Found;
	}

	// #include directive did not match the regex and is ill-formed
	error = "Ill-formed include directive: " + trimmedLine;
	return EIncludeSearchResult::Error;
}

std::string ShaderPreprocessor::getIncludedFilePath(const std::string& mainFilePath, const std::string& includePath) {
	auto currentPath = u8path(mainFilePath);
	auto currentDir = currentPath.remove_filename();
	return currentPath.append(includePath).normalized().string();
}

bool ShaderPreprocessor::traverseIncludedFiles(const std::string& filePath, std::ostringstream& os, std::unordered_set<std::string>& visitedPaths) {
	const auto step = RecursionScopeGuard{visitedPaths, filePath};
	if (step.isLoopDetected()) {
		error_ = fmt::format("Include loop detected: '{}'", filePath);
		return false;
	}

	// Specified path does not have to exist. File is reported as included anyway to set file watchers.
	includedFiles_.emplace(filePath);

	std::ifstream shaderFile(u8path(filePath).internalPath());
	if (!shaderFile.is_open()) {
		error_ = fmt::format("Cannot open file: '{}'", filePath);
		return false;
	}

	std::string line;
	while (std::getline(shaderFile, line)) {
		std::string includeError;
		std::string includePath;
		switch (findInclude(line, includePath, includeError)) {
			case EIncludeSearchResult::Found:
				// Output included file contents
				if (!traverseIncludedFiles(getIncludedFilePath(filePath, includePath), os, visitedPaths)) {
					return false;
				}
				break;
			case EIncludeSearchResult::NotFound:
				// Bypass the line to output
				os << line << std::endl;
				break;
			case EIncludeSearchResult::Error:
				// Break processing and report an error
				error_ = includeError;
				return false;
			default:
				return false;
		}
	}

	return true;
}

void ShaderPreprocessor::processShaderFile(const std::string& shaderPath) {
	std::ostringstream os;

	// Recursion detection context.
	std::unordered_set<std::string> visitedPaths;

	if (traverseIncludedFiles(shaderPath, os, visitedPaths)) {
		processedShader_ = os.str();
	}

	// Shader itself is not treated as included file.
	includedFiles_.erase(shaderPath);
}

bool ShaderPreprocessor::hasError() const {
	return !error_.empty();
}

const std::string& ShaderPreprocessor::getError() const {
	return error_;
}

}  // namespace raco::utils::shader
