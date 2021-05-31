/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "FileChangeListenerImpl.h"

#include "utils/PathUtils.h"

#include "core/PathManager.h"
#include "log_system/log.h"

#if (defined(OS_WINDOWS))
#include <Windows.h>
#elif (defined(OS_UNIX))
#include <unistd.h>
#include <fcntl.h>
#endif

namespace raco::components {

FileChangeListenerImpl::FileChangeListenerImpl(std::string& absPath, Callback& callbackHandler)
	: path_(absPath), fileChangeCallback_(callbackHandler) {
	delayedLoadTimer_.setInterval(DELAYED_FILE_LOAD_TIME_MSEC);
	delayedLoadTimer_.setSingleShot(true);

	fileWatchConnection_ = QObject::connect(&fileWatcher_, &QFileSystemWatcher::fileChanged, [this](const auto& filePath) { onFileChanged(filePath); });
	directoryWatchConnection_ = QObject::connect(&fileWatcher_, &QFileSystemWatcher::directoryChanged, [this](const auto& dirPath) { onDirectoryChanged(dirPath); });
	delayedLoadTimerConnection_ = QObject::connect(&delayedLoadTimer_, &QTimer::timeout, [this]() { onDelayedLoad(); });

	didFileExistOnLastWatch_ = raco::utils::path::exists(absPath);

	installWatchers();
}

FileChangeListenerImpl::~FileChangeListenerImpl() {
	QObject::disconnect(fileWatchConnection_);
	QObject::disconnect(directoryWatchConnection_);
	QObject::disconnect(delayedLoadTimerConnection_);
	delayedLoadTimer_.stop();
}

std::string FileChangeListenerImpl::getPath() const {
	return path_.generic_string();
}

void FileChangeListenerImpl::addPathToWatch(const QString& path) {
	auto pathSuccessfullyAdded = fileWatcher_.addPath(path);
	if (!pathSuccessfullyAdded) {
		LOG_DEBUG(log_system::RAMSES_BACKEND, "Could not add path {} to file change listener", path.toLatin1());
	} else {
		LOG_DEBUG(log_system::RAMSES_BACKEND, "Added path {} to file change listener", path.toLatin1());
	}
}

void FileChangeListenerImpl::installWatchers() {
	installFileWatch();
	installDirectoryWatch();
}

void FileChangeListenerImpl::installFileWatch() {
	if (raco::utils::path::exists(path_.generic_string())) {
		auto pathQtString = QString::fromStdString(path_.generic_string());
		auto fileWatcherContainsFilePath = fileWatcher_.files().contains(pathQtString);

		if (!fileWatcherContainsFilePath) {
			addPathToWatch(pathQtString);
		}
	}
}

void FileChangeListenerImpl::installDirectoryWatch() {
	auto previouslyWatchedDirs = fileWatcher_.directories();
	for (auto parentPath = path_.parent_path(); parentPath != path_.root_path() && raco::utils::path::isExistingDirectory(parentPath.generic_string()); parentPath = parentPath.parent_path()) {
		auto parentPathQtString = QString::fromStdString(parentPath.generic_string());
		auto fileWatcherContainsDirectory = fileWatcher_.files().contains(parentPathQtString);

		if (!fileWatcherContainsDirectory) {
			addPathToWatch(parentPathQtString);
		} else {
			previouslyWatchedDirs.removeAll(parentPathQtString);
		}
	}

	if (!previouslyWatchedDirs.empty()) {
		fileWatcher_.removePaths(previouslyWatchedDirs);
	}
}

void FileChangeListenerImpl::launchDelayedLoad() {
	delayedLoadTimer_.stop();
	delayedLoadTimer_.start();
	LOG_DEBUG(log_system::RAMSES_BACKEND, "Launched delayed file watch loading");
}

void FileChangeListenerImpl::onFileChanged(const QString& filePath) {
	launchDelayedLoad();
}

void FileChangeListenerImpl::onDelayedLoad() {
	didFileExistOnLastWatch_ = raco::utils::path::exists(path_.generic_string());

#if (defined(OS_WINDOWS) || defined(OS_UNIX))
	if (didFileExistOnLastWatch_) {
		if (fileCanBeAccessed()) {
			fileChangeCallback_();
		}
	} else {
		fileChangeCallback_();
	}
#else
	fileChangeCallback_();
#endif
}

void FileChangeListenerImpl::onDirectoryChanged(const QString& dirPath) {
	auto fileExists = raco::utils::path::exists(path_.generic_string());
	auto directoryOrFileWasRenamed = didFileExistOnLastWatch_ && !fileExists;
	auto fileWasCreatedOrMovedInPlace = !didFileExistOnLastWatch_ && fileExists;

	if (directoryOrFileWasRenamed || fileWasCreatedOrMovedInPlace) {
		launchDelayedLoad();
	}

	installFileWatch();
}

bool FileChangeListenerImpl::fileCanBeAccessed() {
#if (defined(OS_WINDOWS))
	return fileCanBeAccessedOnWindows();
#elif (defined(OS_UNIX))
	return fileCanBeAccessedOnUnix();
#endif
}

#if (defined(OS_WINDOWS))
bool FileChangeListenerImpl::fileCanBeAccessedOnWindows() {
	auto fileHandle = CreateFileW(path_.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
		return true;
	}

	LOG_DEBUG(log_system::RAMSES_BACKEND, "Windows could not access file {} - it seems to be opened for writing by another process right now.", path_.generic_string());

	return false;
}
#endif

#if (defined(OS_UNIX))
bool FileChangeListenerImpl::fileCanBeAccessedOnUnix() {
	auto fileDescriptor = open(path_.generic_string().c_str(), O_RDONLY);

	if (fileDescriptor > 0 && EACCES != errno && EAGAIN != errno) {
		close(fileDescriptor);
		return true;
	}

	LOG_DEBUG(log_system::RAMSES_BACKEND, "Linux could not access file {} - {}", path_.generic_string(), strerror(errno));

	return false;
}
#endif

}  // namespace raco::components