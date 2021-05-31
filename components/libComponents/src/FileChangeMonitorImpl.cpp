/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/FileChangeMonitorImpl.h"

#include "FileChangeListenerImpl.h"

namespace raco::components {

FileChangeMonitorImpl::UniqueListener FileChangeMonitorImpl::registerFileChangedHandler(std::string absPath, raco::core::FileChangeCallback callback) {
	if (absPath.empty()) {
		return UniqueListener(nullptr);
	}

	FileChangeListenerImpl::Callback lambda([this, callback]() { callback(context_); });
	auto l = new FileChangeListenerImpl{absPath, lambda};

	listeners_[l->getPath()].emplace(l);

	return UniqueListener(l, [this](core::FileChangeListener* listener) {
		this->unregister(listener->getPath(), listener);
		delete listener;
	});
}

void FileChangeMonitorImpl::unregister(std::string absPath, raco::core::FileChangeListener* listener) {
	if (listeners_.count(absPath) > 0) {
		listeners_[absPath].erase(listener);
		if (listeners_[absPath].empty()) {
			listeners_.erase(absPath);
		}
	}
}

FileChangeMonitorImpl::UniqueListener ExternalProjectFileChangeMonitor::registerFileChangedHandler(std::string absPath, Callback callback) {
	if (absPath.empty()) {
		return UniqueListener(nullptr);
	}

	auto l = new FileChangeListenerImpl{absPath, callback};

	listeners_[l->getPath()].emplace(l);

	return UniqueListener(l, [this](core::FileChangeListener* listener) {
		this->unregister(listener->getPath(), listener);
		delete listener;
	});
}

void ExternalProjectFileChangeMonitor::unregister(std::string absPath, raco::core::FileChangeListener* listener) {
	if (listeners_.count(absPath) > 0) {
		listeners_[absPath].erase(listener);
		if (listeners_[absPath].empty()) {
			listeners_.erase(absPath);
		}
	}
}

}  // namespace raco::components