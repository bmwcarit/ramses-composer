/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/FileChangeListenerImpl.h"

#include "utils/u8path.h"

#include "core/PathManager.h"
#include "log_system/log.h"

#if (defined(OS_WINDOWS))
#include <Windows.h>
#elif (defined(OS_UNIX))
#include <unistd.h>
#include <fcntl.h>
#endif

namespace raco::components {

FileChangeListenerImpl::FileChangeListenerImpl(const Callback& callbackHandler) 
	: fileChangeCallback_(callbackHandler) {
	delayedLoadTimer_.setInterval(DELAYED_FILE_LOAD_TIME_MSEC);
	delayedLoadTimer_.setSingleShot(true);

	fileWatchConnection_ = QObject::connect(&fileWatcher_, &QFileSystemWatcher::fileChanged, [this](const auto& filePath) { onFileChanged(filePath); });
	directoryWatchConnection_ = QObject::connect(&fileWatcher_, &QFileSystemWatcher::directoryChanged, [this](const auto& dirPath) { onDirectoryChanged(dirPath); });
	delayedLoadTimerConnection_ = QObject::connect(&delayedLoadTimer_, &QTimer::timeout, [this]() { onDelayedLoad(); });
}

FileChangeListenerImpl::~FileChangeListenerImpl() {
	QObject::disconnect(fileWatchConnection_);
	QObject::disconnect(directoryWatchConnection_);
	QObject::disconnect(delayedLoadTimerConnection_);
	delayedLoadTimer_.stop();
}

void FileChangeListenerImpl::addPathToWatch(const QString& path) {
	auto pathSuccessfullyAdded = fileWatcher_.addPath(path);
	if (!pathSuccessfullyAdded) {
		LOG_DEBUG(log_system::RAMSES_BACKEND, "Could not add path {} to file change listener", path.toLatin1());
	} else {
		LOG_DEBUG(log_system::RAMSES_BACKEND, "Added path {} to file change listener", path.toLatin1());
	}
}

void FileChangeListenerImpl::installFileWatch(const raco::utils::u8path& path) {
	if (path.exists()) {
		auto pathQtString = QString::fromStdString(path.string());
		auto fileWatcherContainsFilePath = fileWatcher_.files().contains(pathQtString);

		if (!fileWatcherContainsFilePath) {
			addPathToWatch(pathQtString);
		}
	}
}

void FileChangeListenerImpl::add(const std::string& absPath) {
	const raco::utils::u8path& path(absPath);

	// create directoriy watchers up to root -> return direct parent directory
	auto directoryNode = createDirectoryWatches(path.parent_path());

	// create file watcher and insert into direct parent directory
	installFileWatch(path);
	auto [it, success] = directoryNode->children_.insert({path, std::make_unique<Node>(path, directoryNode, false, path.exists())});
	watchedFiles_[path] = it->second.get();
}

FileChangeListenerImpl::Node* FileChangeListenerImpl::createDirectoryWatches(const raco::utils::u8path& path) {
	if (path != path.root_path()) {
		auto node = createDirectoryWatches(path.parent_path());

		auto it = node->children_.find(path);
		if (it == node->children_.end()) {
			bool exists = path.existsDirectory();
			if (exists) {
				addPathToWatch(QString::fromStdString(path.string()));
			}
			auto [newIt, success] = node->children_.insert({path, std::make_unique<Node>(path, node, true, exists)});
			return newIt->second.get();
		} else {
			return it->second.get();
		}
	}
	return &rootNode_;
}

FileChangeListenerImpl::Node* FileChangeListenerImpl::getNode(const raco::utils::u8path& path) {
	if (path != path.root_path()) {
		auto node = getNode(path.parent_path());

		auto it = node->children_.find(path);
		assert(it != node->children_.end());
		return it->second.get();
	}
	return &rootNode_;
}


void FileChangeListenerImpl::updateDirectoryWatches(Node* node) {
	if (node->isDirectory_ && node->path_.existsDirectory()) {
		addPathToWatch(QString::fromStdString(node->path_.string()));
		for (const auto& [dummy, childNode] : node->children_) {
			updateDirectoryWatches(childNode.get());
		}
	}
}

void FileChangeListenerImpl::remove(const std::string& absPath) {
	const raco::utils::u8path& path(absPath);
	if (fileWatcher_.removePath(QString::fromStdString(path.string()))) {

		// remove file node and all parent directories which have no child nodes anymore
		auto fileNode = watchedFiles_[path];
		watchedFiles_.erase(path);

		auto parentNode = fileNode->parent_;
		parentNode->children_.erase(path);
		removeDirectoryWatches(parentNode);
	}
}

void FileChangeListenerImpl::removeDirectoryWatches(Node* node) {
	if (node->children_.empty() && node != &rootNode_) {
		fileWatcher_.removePath(QString::fromStdString(node->path_.string()));
	
		auto parentNode = node->parent_;
		parentNode->children_.erase(node->path_);
		removeDirectoryWatches(parentNode);
	}
}

void FileChangeListenerImpl::launchDelayedLoad(const utils::u8path& path) {
	changedFiles_.insert(path);
	delayedLoadTimer_.stop();
	delayedLoadTimer_.start();
	LOG_DEBUG(log_system::RAMSES_BACKEND, "Launched delayed file watch loading");
}

void FileChangeListenerImpl::onFileChanged(const QString& filePath) {
	launchDelayedLoad(filePath.toStdString());
}

void FileChangeListenerImpl::onDelayedLoad() {
#if (defined(OS_WINDOWS) || defined(OS_UNIX))
	for (const auto& path : changedFiles_) {

		auto it = watchedFiles_.find(path);
		if (it != watchedFiles_.end()) {
			it->second->didFileExistOnLastWatch_ = path.exists();
			if (!it->second->didFileExistOnLastWatch_ || fileCanBeAccessed(path)) {
				fileChangeCallback_(path.string());
			}
		}
	}
#else
	fileChangeCallback_();
#endif

	changedFiles_.clear();
}

void FileChangeListenerImpl::onDirectoryChanged(const QString& dirPathString) {
	utils::u8path dirPath(dirPathString.toStdString());
	if (auto node = getNode(dirPath)) {
		updateDirectoryWatches(node);
	}
	for (const auto& [path, entry] : watchedFiles_) {
		if (dirPath.contains(path)) {
			if (path.exists() != entry->didFileExistOnLastWatch_) {
				launchDelayedLoad(path);
			}
			installFileWatch(path);
		}
	}
}

bool FileChangeListenerImpl::fileCanBeAccessed(const raco::utils::u8path& path) {
#if (defined(OS_WINDOWS))
	auto fileHandle = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
		return true;
	}

	LOG_DEBUG(log_system::RAMSES_BACKEND, "Windows could not access file {} - it seems to be opened for writing by another process right now.", path.string());
#elif (defined(OS_UNIX))
	auto fileDescriptor = open(path.string().c_str(), O_RDONLY);
	if (fileDescriptor > 0) {
		close(fileDescriptor);
		return true;
	}

	LOG_DEBUG(log_system::RAMSES_BACKEND, "Linux could not access file {} - {}", path.string(), strerror(errno));
#endif
	return false;
}

}  // namespace raco::components