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

#include "utils/u8path.h"

#include "core/PathManager.h"
#include <QSettings>

namespace raco::components {

RaCoPreferences::RaCoPreferences() {
	load();
}

void RaCoPreferences::init() noexcept {
	RaCoPreferences::instance();
}

bool RaCoPreferences::save() {
	auto settings = raco::core::PathManager::preferenceSettings();
	settings.setValue("userProjectsDirectory", userProjectsDirectory);

	settings.setValue("imageSubdirectory", imageSubdirectory);
	settings.setValue("meshSubdirectory", meshSubdirectory);
	settings.setValue("scriptSubdirectory", scriptSubdirectory);
	settings.setValue("shaderSubdirectory", shaderSubdirectory);

	settings.sync();
	return settings.status() == QSettings::NoError;
}

void RaCoPreferences::load() {
	auto settings = raco::core::PathManager::preferenceSettings();
	std::string dir = settings.value("userProjectsDirectory", "").toString().toStdString();
	if (raco::utils::u8path(dir).existsDirectory()) {
		userProjectsDirectory = QString::fromStdString(dir);
	} else {
		userProjectsDirectory = QString::fromStdString(raco::core::PathManager::defaultProjectFallbackPath().string());
	}

	imageSubdirectory = settings.value("imageSubdirectory", "images").toString();
	meshSubdirectory = settings.value("meshSubdirectory", "meshes").toString();
	scriptSubdirectory = settings.value("scriptSubdirectory", "scripts").toString();
	shaderSubdirectory = settings.value("shaderSubdirectory", "shaders").toString();
}

RaCoPreferences& RaCoPreferences::instance() noexcept {
	static RaCoPreferences instance_{};
	return instance_;
}

}  // namespace raco::components
