/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "u8path.h"

#include <set>
#include <string>
#include <sstream>
#include <unordered_set>

class ShaderPreprocessorTest;

namespace raco::utils::shader {

enum class EIncludeSearchResult {
	NotFound = 0,
	Found,
	Error,
	NUMBER_OF_ELEMENTS
};

class ShaderPreprocessor {
public:
	ShaderPreprocessor(const std::string& mainShaderPath);

	[[nodiscard]] const std::string& getProcessedShader() const;

	// Paths known to be included. Actual files might be missing.
	[[nodiscard]] const std::set<std::string>& getIncludedFiles() const;

	[[nodiscard]] bool hasError() const;
	[[nodiscard]] const std::string& getError() const;

private:
	// Needs access to findInclude()
	friend class ::ShaderPreprocessorTest;

	static EIncludeSearchResult findInclude(const std::string& line, std::string& foundPath, std::string& error);
	static std::string getIncludedFilePath(const std::string& mainFilePath, const std::string& includePath);
	bool traverseIncludedFiles(const std::string& filePath, std::ostringstream& os, std::unordered_set<std::string>& visitedPaths);
	void processShaderFile(const std::string& shaderPath);

	std::string processedShader_;
	std::set<std::string> includedFiles_;
	std::string error_;
};

}  // namespace raco::utils::shader
