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

#include "data_storage/ReflectionInterface.h"
#include "utils/u8path.h"

#include <map>
#include <string>
#include <QSettings>

namespace raco::components {
class RaCoPreferences;
}

namespace raco::core {
	

struct PathManager {
	using u8path = raco::utils::u8path;

	static constexpr const char* DEFAULT_FILENAME = "Unnamed.rca";
	static constexpr const char* Q_LAYOUT_FILE_NAME = "layout.ini";
	static constexpr const char* Q_PREFERENCES_FILE_NAME = "preferences.ini";
	static constexpr const char* Q_RECENT_FILES_STORE_NAME = "recent_files.ini";
	static constexpr const char* LOG_SUB_DIRECTORY = "logs";
	static constexpr const char* LEGACY_CONFIG_SUB_DIRECTORY = "configfiles";
	static constexpr const char* DEFAULT_PROJECT_SUB_DIRECTORY = "projects";
	static constexpr const char* LOG_FILE_EDITOR_BASE_NAME = "RamsesComposer";
	static constexpr const char* LOG_FILE_HEADLESS_BASE_NAME = "RaCoHeadless";
	
	enum class FolderTypeKeys {
		Invalid = 0,
		Project,
		Image,
		Mesh,
		Script,
		Interface,
		Shader
	};

	static u8path legacyConfigDirectory();	

	static void init(const u8path& executableDirectory, const u8path& appDataDirectory);

	static u8path executableDirectory();

	static u8path defaultBaseDirectory();

	static u8path logFileDirectory();

	static u8path defaultConfigDirectory();

	static u8path defaultResourceDirectory();

	static u8path defaultProjectFallbackPath();

	static void migrateLegacyConfigDirectory();
	
	static u8path layoutFilePath();

	static u8path recentFilesStorePath();

	static u8path preferenceFilePath();
		
	static QSettings layoutSettings();

	static QSettings recentFilesStoreSettings();

	static QSettings preferenceSettings();

	static const u8path& getCachedPath(FolderTypeKeys key, const u8path& fallbackPath = {});

	static void setCachedPath(FolderTypeKeys key, const u8path& path);

private:
	friend class raco::components::RaCoPreferences;

	static u8path executableDirectory_;
	static u8path basePath_;
	static u8path appDataBasePath_;

	// The default values for the subdirectories are set in RaCoPreferences::load
	static inline std::map<FolderTypeKeys, u8path> cachedPaths_ = {
		{FolderTypeKeys::Project, {}},
		{FolderTypeKeys::Image, {}},
		{FolderTypeKeys::Mesh, {}},
		{FolderTypeKeys::Script, {}},
		{FolderTypeKeys::Interface, {}},
		{FolderTypeKeys::Shader, {}}
	};
};

}  // namespace raco::core