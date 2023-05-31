/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/PathManager.h"
#include "user_types/AnimationChannel.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/Skin.h"
#include "user_types/Texture.h"

#include <algorithm>
#include <cctype>
#include <QStandardPaths>


namespace raco::core {

using u8path = raco::utils::u8path;

u8path PathManager::basePath_;
u8path PathManager::appDataBasePath_;
u8path PathManager::executableDirectory_;

void PathManager::init(const u8path& executableDirectory, const u8path& appDataDirectory) {
	executableDirectory_ = executableDirectory.normalized();
	basePath_ = executableDirectory.normalized().parent_path().parent_path();
	appDataBasePath_ = appDataDirectory.normalized();
	migrateLegacyConfigDirectory();
}

u8path PathManager::executableDirectory() {
	return executableDirectory_;
}

u8path PathManager::defaultBaseDirectory() {
	return basePath_;
}

u8path PathManager::defaultConfigDirectory() {
	return appDataBasePath_;
}

u8path PathManager::legacyConfigDirectory() {
	return defaultBaseDirectory() / LEGACY_CONFIG_SUB_DIRECTORY;
}

u8path PathManager::defaultResourceDirectory() {
	return defaultBaseDirectory() / DEFAULT_PROJECT_SUB_DIRECTORY;
}

u8path PathManager::logFileDirectory() {
	return defaultConfigDirectory() / LOG_SUB_DIRECTORY;
}

u8path PathManager::layoutFilePath() {
	return defaultConfigDirectory() / Q_LAYOUT_FILE_NAME;
}

u8path PathManager::recentFilesStorePath() {
	return defaultConfigDirectory() / Q_RECENT_FILES_STORE_NAME;
}

u8path PathManager::preferenceFilePath() {
	return defaultConfigDirectory() / Q_PREFERENCES_FILE_NAME;
}

void PathManager::migrateLegacyConfigDirectory() {
	auto configDir = defaultConfigDirectory();
	auto legacyConfigDir = legacyConfigDirectory();
	auto logDir = logFileDirectory();

	// Check which config dirs exist and contain files
	auto legacyConfigExistsAndHasFiles = legacyConfigDir.existsDirectory() && std::filesystem::directory_iterator(legacyConfigDir) != std::filesystem::directory_iterator{};
	auto newConfigExistsAndHasFiles = configDir.existsDirectory() && std::filesystem::directory_iterator(configDir) != std::filesystem::directory_iterator{};

	if (legacyConfigExistsAndHasFiles && !newConfigExistsAndHasFiles) {
		if (configDir.existsDirectory()) {
			std::filesystem::remove(configDir);
		}
		std::filesystem::rename(legacyConfigDir, configDir);
		std::filesystem::create_directories(logDir);	
		for(const auto& p : std::filesystem::directory_iterator(configDir)){
			if (u8path(p).existsFile() && p.path().extension().u8string() == ".log") {
				std::filesystem::rename(p.path(), logDir / p.path().filename());
			}
		}
	} else {		
		std::filesystem::create_directories(configDir);	
		std::filesystem::create_directories(logDir);	
	}
}

u8path PathManager::defaultProjectFallbackPath() {
	return defaultBaseDirectory() / DEFAULT_PROJECT_SUB_DIRECTORY;
}

QSettings PathManager::layoutSettings() {
	return QSettings(raco::core::PathManager::layoutFilePath().string().c_str(), QSettings::IniFormat);
}

QSettings PathManager::recentFilesStoreSettings() {
	return QSettings(raco::core::PathManager::recentFilesStorePath().string().c_str(), QSettings::IniFormat);
}

QSettings PathManager::preferenceSettings() {
	return QSettings(raco::core::PathManager::preferenceFilePath().string().c_str(), QSettings::IniFormat);
}

const u8path& PathManager::getCachedPath(FolderTypeKeys key, const u8path& fallbackPath) {
	auto pathIt = cachedPaths_.find(key);
	if (pathIt != cachedPaths_.end()) {
		auto& cachedPath = pathIt->second;
		if (!cachedPath.existsDirectory() && !fallbackPath.empty()) {
			return fallbackPath;
		}
		return cachedPath;
	}

	return fallbackPath;
}

void PathManager::setCachedPath(FolderTypeKeys key, const u8path& path) {
	auto pathIt = cachedPaths_.find(key);
	if (pathIt != cachedPaths_.end()) {
		pathIt->second = path;
	}
}

}  // namespace raco::core
