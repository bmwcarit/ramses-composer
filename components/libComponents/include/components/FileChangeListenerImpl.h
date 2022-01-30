/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/FileChangeCallback.h"
#include "core/FileChangeMonitor.h"

#include "utils/stdfilesystem.h"
#include "utils/u8path.h"
#include <string>

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
	using Callback = std::function<void(void)>;

	FileChangeListenerImpl(std::string &absPath, const Callback& callbackHandler);
	virtual ~FileChangeListenerImpl();
	
	std::string getPath() const;

	static constexpr int DELAYED_FILE_LOAD_TIME_MSEC = 100;
private:
	
	raco::utils::u8path path_;
	Callback fileChangeCallback_;
	QFileSystemWatcher fileWatcher_;
	QTimer delayedLoadTimer_;
	bool didFileExistOnLastWatch_;

	QMetaObject::Connection fileWatchConnection_;
	QMetaObject::Connection directoryWatchConnection_;
	QMetaObject::Connection delayedLoadTimerConnection_;
	
	void addPathToWatch(const QString &path);
	void installWatchers();
	void installFileWatch();
	void installDirectoryWatch();
	void launchDelayedLoad();
	void onFileChanged(const QString &filePath);
	void onDelayedLoad();
	void onDirectoryChanged(const QString &dirPath);

	bool fileCanBeAccessed();
	#if (defined(OS_WINDOWS))
	bool fileCanBeAccessedOnWindows();
	#elif (defined(OS_UNIX))
	bool fileCanBeAccessedOnUnix();
	#endif

};

}  // namespace raco::components

