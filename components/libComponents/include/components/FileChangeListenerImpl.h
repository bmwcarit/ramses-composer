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

#include "core/FileChangeCallback.h"
#include "core/FileChangeMonitor.h"

#include "utils/u8path.h"
#include <filesystem>
#include <set>
#include <string>
#include <map>

#include <QFileSystemWatcher>
#include <QTimer>

#if (defined(Q_OS_WIN))
#define OS_WINDOWS
#elif (defined(Q_OS_UNIX) | defined(Q_OS_LINUX))
#define OS_UNIX
#endif

namespace raco::components {

class FileChangeListenerImpl {
public:
	using Callback = std::function<void(const std::string& absPath)>;

	FileChangeListenerImpl(const Callback &callbackHandler);
	virtual ~FileChangeListenerImpl();

	void add(const std::string &absPath);
	void remove(const std::string &absPath);

	static constexpr int DELAYED_FILE_LOAD_TIME_MSEC = 100;

private:
	struct Node {
		Node(
			utils::u8path path = {}, Node *parent = nullptr, bool isDirectory = true, bool didFileExistOnLastWatch = false)
			: path_(path), parent_(parent), isDirectory_(isDirectory), didFileExistOnLastWatch_(didFileExistOnLastWatch) {
		}

		utils::u8path path_;
		std::map<utils::u8path, std::unique_ptr<Node>> children_;
		Node *parent_;
		bool isDirectory_;
		bool didFileExistOnLastWatch_;
	};

	Node rootNode_;

	std::map<raco::utils::u8path, Node*> watchedFiles_;

	Callback fileChangeCallback_;

	QFileSystemWatcher fileWatcher_;

	QTimer delayedLoadTimer_;
	std::set<utils::u8path> changedFiles_;

	QMetaObject::Connection fileWatchConnection_;
	QMetaObject::Connection directoryWatchConnection_;
	QMetaObject::Connection delayedLoadTimerConnection_;

	void addPathToWatch(const QString &path);
	void installFileWatch(const raco::utils::u8path &path);
	void launchDelayedLoad(const utils::u8path &path);
	void onFileChanged(const QString &filePath);
	void onDelayedLoad();
	void onDirectoryChanged(const QString &dirPath);
	Node *createDirectoryWatches(const raco::utils::u8path &path);
	void removeDirectoryWatches(Node* node);
	Node *getNode(const raco::utils::u8path &path);
	void updateDirectoryWatches(Node *node);


	static bool fileCanBeAccessed(const raco::utils::u8path &path);
};

}  // namespace raco::components
