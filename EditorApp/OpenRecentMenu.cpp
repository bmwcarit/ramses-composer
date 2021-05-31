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
#include <QSettings>

OpenRecentMenu::OpenRecentMenu(QWidget* parent) : QMenu{"Open Recent", parent} {
	refreshRecentFileMenu();
}

void OpenRecentMenu::addRecentFile(const QString& file) {
	if (file.size()) {
		QSettings recentFilesStore(raco::core::PathManager::recentFilesStorePath().c_str(), QSettings::IniFormat);
		QStringList recentFiles{recentFilesStore.value("recent_files").toStringList()};
		auto it = std::find(recentFiles.begin(), recentFiles.end(), file);
		if (it != recentFiles.end()) {
			recentFiles.erase(it);
		}
		recentFiles.insert(0, file);
		recentFilesStore.setValue("recent_files", recentFiles);
		refreshRecentFileMenu();
	}
}

void OpenRecentMenu::refreshRecentFileMenu() {
	QSettings recentFilesStore(raco::core::PathManager::recentFilesStorePath().c_str(), QSettings::IniFormat);
	QStringList recentFiles{recentFilesStore.value("recent_files").toStringList()};
	setDisabled(recentFiles.size() == 0);
	while (actions().size() > 0) {
		removeAction(actions().at(0));
	}
	for (const auto& file : recentFiles) {
		auto* action = addAction(file);
		QObject::connect(action, &QAction::triggered, this, [this, file]() {
			Q_EMIT openProject(file);
		});
	}
}
