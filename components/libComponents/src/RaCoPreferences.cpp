/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/RaCoPreferences.h"

#include "utils/PathUtils.h"

#include "core/PathManager.h"
#include "log_system/log.h"
#include <QSettings>

namespace raco::components {

RaCoPreferences::RaCoPreferences() {
	load();
}

bool RaCoPreferences::init() noexcept {
	RaCoPreferences::instance();
	return true;
}

bool RaCoPreferences::save() {
	QSettings settings(raco::core::PathManager::preferenceFileLocation().c_str(), QSettings::IniFormat);
	settings.setValue("userProjectsDirectory", userProjectsDirectory);

	settings.setValue("imageSubdirectory", imageSubdirectory);
	settings.setValue("meshSubdirectory", meshSubdirectory);
	settings.setValue("scriptSubdirectory", scriptSubdirectory);
	settings.setValue("shaderSubdirectory", shaderSubdirectory);

	return true;
}

bool RaCoPreferences::load() {
	LOG_INFO(log_system::COMMON, "{}", raco::core::PathManager::preferenceFileLocation());
	QSettings settings(raco::core::PathManager::preferenceFileLocation().c_str(), QSettings::IniFormat);
	std::string dir = settings.value("userProjectsDirectory", "").toString().toStdString();
	if (raco::utils::path::isExistingDirectory(dir)) {
		userProjectsDirectory = QString::fromStdString(dir);
	} else {
		userProjectsDirectory = QString::fromStdString(raco::core::PathManager::defaultProjectFallbackPath());
	}

	imageSubdirectory = settings.value("imageSubdirectory", "images").toString();
	meshSubdirectory = settings.value("meshSubdirectory", "meshes").toString();
	scriptSubdirectory = settings.value("scriptSubdirectory", "scripts").toString();
	shaderSubdirectory = settings.value("shaderSubdirectory", "shaders").toString();

	return true;
}

RaCoPreferences& RaCoPreferences::instance() noexcept {
	static RaCoPreferences instance_{};
	return instance_;
}

}  // namespace raco::components
