/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "utils/stdfilesystem.h"


#include <map>
#include <string>

namespace raco::components {
class RaCoPreferences;
}

namespace raco::core {

struct PathManager {
	static constexpr const char* DEFAULT_FILENAME = "Unnamed.rca";
	static constexpr const char* LOG_FILE_NAME = "RamsesComposer.log";
	static constexpr const char* Q_LAYOUT_FILE_NAME = "layout.ini";
	static constexpr const char* Q_PREFERENCES_FILE_NAME = "preferences.ini";
	static constexpr const char* Q_RECENT_FILES_STORE_NAME = "recent_files.ini";
	static constexpr const char* DEFAULT_CONFIG_SUB_DIRECTORY = "configfiles";
	static constexpr const char* DEFAULT_PROJECT_SUB_DIRECTORY = "projects";
	static constexpr const char* RESOURCE_SUB_DIRECTORY = "resources";
	static constexpr const char* IMAGE_SUB_DIRECTORY = "images";
	static constexpr const char* MESH_SUB_DIRECTORY = "meshes";
	static constexpr const char* SCRIPT_SUB_DIRECTORY = "scripts";
	static constexpr const char* SHADER_SUB_DIRECTORY = "shaders";

	static std::filesystem::path normal_path(const std::string& path);

	static void init(const std::string& executableDirectory);

	static std::filesystem::path defaultBaseDirectory();

	static std::string defaultConfigDirectory();

	static std::filesystem::path defaultResourceDirectory();

	static std::string defaultProjectFallbackPath();

	static std::string logFilePath();

	static std::string layoutFilePath();

	static std::string recentFilesStorePath();

	static std::string preferenceFileLocation();

	static std::string constructRelativePath(const std::string& absolutePath, const std::string& basePath);

	// Construct absolute paths from base directory and relative  or absolute file path.
	// Absolute file paths are returned as is.
	// Relative file paths are interpreted as relative to the dirPath argument.
	static std::string constructAbsolutePath(const std::string& dirPath, const std::string& filePath);

	static std::string rerootRelativePath(const std::string& relativePath, const std::string& oldPath, const std::string& newPath);

	static std::string sanitizePath(const std::string& path);

	static bool pathsShareSameRoot(const std::string& lhd, const std::string& rhd);

	static const std::string& getCachedPath(const std::string& key, const std::string &fallbackPath = "");

	static void setCachedPath(const std::string& key, const std::string& path);

	static void setAllCachedPathRoots(const std::string& folder);

private:
	friend class raco::components::RaCoPreferences;

	static std::filesystem::path basePath_;

	static inline std::map<std::string, std::string> cachedPaths_ = {
		{DEFAULT_PROJECT_SUB_DIRECTORY, ""},
		{IMAGE_SUB_DIRECTORY, ""},
		{MESH_SUB_DIRECTORY, ""},
		{SCRIPT_SUB_DIRECTORY, ""},
		{SHADER_SUB_DIRECTORY, ""}
	};
};

}  // namespace raco::core