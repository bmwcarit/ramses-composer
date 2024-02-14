/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "OpenRecentMenu.h"

#include "core/PathManager.h"
#include "log_system/log.h"
#include "utils/u8path.h"
#include <fstream>
#include <QSettings>

using namespace raco;

OpenRecentMenu::OpenRecentMenu(QWidget* parent) : QMenu{"Open &Recent", parent} {
	QObject::connect(this, &QMenu::aboutToShow, this, [this]() {
		refreshRecentFileMenu();
	});
}

void OpenRecentMenu::addRecentFile(const QString& file) {
	if (file.size()) {
		auto recentFilesStore = core::PathManager::recentFilesStoreSettings();
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
			LOG_ERROR(log_system::COMMON, "Saving recent files list failed: {}", core::PathManager::recentFilesStorePath().string());
		}
	}
}

bool isOneDriveIniFile(std::filesystem::path path) {
	auto magicString = "\\OneDrive.exe";
	std::ifstream in{path, std::ifstream::in | std::ifstream::binary};
	std::string line;

	if (in.is_open()) {
		while (std::getline(in, line)) {
			if (line.find(magicString) != std::string::npos) {
				return true;
			}
		}
	}

	return false;
}

// Check whether specified path is on OneDrive.
bool isOneDrivePath(utils::u8path path) {
	auto onedriveIniFilename = "desktop.ini";

	auto fsPath = std::filesystem::path(path.internalPath());

	if (fsPath.is_relative()) {
		// Can not analyze relative paths.
		return false;
	}

	if (fsPath.has_filename()) {
		fsPath.remove_filename();
	}

	// Search the directory tree up from current folder until system root folder.
	while (fsPath != fsPath.parent_path()) {
		auto directory = std::filesystem::directory_entry(fsPath);
		if (directory.is_directory()) {
			auto itBegin = std::filesystem::directory_iterator{fsPath};
			auto itEnd{std::filesystem::directory_iterator{}};

			// Look for OneDrive's desktop.ini file.
			auto itFound = std::find_if(itBegin, itEnd, [&](const auto& directoryEntry) {
				return directoryEntry.path().filename() == onedriveIniFilename;
				});
			if (itFound != itEnd) {
				std::filesystem::path foundPath(*itFound);

				if (isOneDriveIniFile(foundPath)) {
					LOG_INFO(log_system::COMMON, "OneDrive root found at: {}", foundPath.string());
					return true;
				}
			}
		}

		fsPath = fsPath.parent_path();
	}

	return false;
}

void OpenRecentMenu::refreshRecentFileMenu() {
	auto recentFilesStore = core::PathManager::recentFilesStoreSettings();
	QStringList recentFiles{recentFilesStore.value("recent_files").toStringList()};
	setDisabled(recentFiles.size() == 0);
	while (actions().size() > 0) {
		removeAction(actions().at(0));
	}
	int index = 0;
	for (const auto& file : recentFiles) {
		index = (index + 1) % 10;
		auto actionText = QString::fromStdString("&" + std::to_string(index) + ". ") + file;
		auto* action = addAction(actionText);

		auto fileString = file.toStdString();
		if (!utils::u8path(fileString).exists()) {
			action->setEnabled(false);
			action->setText(actionText + " (unavailable)");
		} else if (isOneDrivePath(utils::u8path(fileString))) {
			// Not touching cloud file to prevent its download.
			action->setText(actionText + " (OneDrive)");
		} else if (!utils::u8path(fileString).userHasReadAccess()) {
			action->setEnabled(false);
			action->setText(actionText + " (no read access)");
		}
		QObject::connect(action, &QAction::triggered, this, [this, file]() {
			Q_EMIT openProject(file);
		});
	}
}
