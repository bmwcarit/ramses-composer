/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "OpenRecentMenu.h"

#include "core/PathManager.h"
#include "log_system/log.h"
#include "utils/u8path.h"
#include <QSettings>

OpenRecentMenu::OpenRecentMenu(QWidget* parent) : QMenu{"Open Recent", parent} {
	QObject::connect(this, &QMenu::aboutToShow, this, [this]() {
		refreshRecentFileMenu();
	});
}

void OpenRecentMenu::addRecentFile(const QString& file) {
	if (file.size()) {
		auto recentFilesStore = raco::core::PathManager::recentFilesStoreSettings();
		QStringList recentFiles{recentFilesStore.value("recent_files").toStringList()};
		auto it = std::find(recentFiles.begin(), recentFiles.end(), file);
		if (it != recentFiles.end()) {
			recentFiles.erase(it);
		}
		recentFiles.insert(0, file);
		int len = recentFiles.length();
		if (len > maxRecentEntries) {
			recentFiles.erase(recentFiles.begin() + maxRecentEntries, recentFiles.end());
		}
		recentFilesStore.setValue("recent_files", recentFiles);
		refreshRecentFileMenu();

		recentFilesStore.sync();
		if (recentFilesStore.status() != QSettings::NoError) {
			LOG_ERROR(raco::log_system::COMMON, "Saving recent files list failed: {}", raco::core::PathManager::recentFilesStorePath().string());
		}
	}
}

void OpenRecentMenu::refreshRecentFileMenu() {
	auto recentFilesStore = raco::core::PathManager::recentFilesStoreSettings();
	QStringList recentFiles{recentFilesStore.value("recent_files").toStringList()};
	setDisabled(recentFiles.size() == 0);
	while (actions().size() > 0) {
		removeAction(actions().at(0));
	}
	for (const auto& file : recentFiles) {
		auto* action = addAction(file);

		auto fileString = file.toStdString();
		if (!raco::utils::u8path(fileString).exists()) {
			action->setEnabled(false);
			action->setText(file + " (unavailable)");
		} else if (!raco::utils::u8path(fileString).userHasReadAccess()) {
			action->setEnabled(false);
			action->setText(file + " (no read access)");
		}
		QObject::connect(action, &QAction::triggered, this, [this, file]() {
			Q_EMIT openProject(file);
		});
	}
}
