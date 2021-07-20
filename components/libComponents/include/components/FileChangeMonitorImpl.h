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

#include "components/FileChangeListenerImpl.h"

#include <memory>
#include <unordered_set>

namespace raco::core {
	class BaseContext;
}

namespace raco::components {

template<typename Base>
class GenericFileChangeMonitorImpl : public Base {
public:

	typename Base::UniqueListener registerFileChangedHandler(std::string absPath, typename Base::Callback callback) override {
		if (absPath.empty()) {
			return typename Base::UniqueListener(nullptr);
		}

		if (listeners_.find(absPath) == listeners_.end()) {
			listeners_[absPath] = std::make_unique<FileChangeListenerImpl>(absPath, [this, absPath]() {
				notify(absPath);
			});
		}

		auto l = new typename Base::Callback{callback};
		callbacks_[absPath].emplace(l);

		return typename Base::UniqueListener(l, [this, absPath, l](typename Base::Callback* listener) {
			this->unregister(absPath, l);
			delete listener;
		});
	}

protected:
	virtual void unregister(std::string absPath, typename Base::Callback* listener) {
		auto it = callbacks_.find(absPath);
		if (it != callbacks_.end()) {
			it->second.erase(listener);
			if (it->second.empty()) {
				callbacks_.erase(absPath);
				listeners_.erase(absPath);
			}
		}
	}

	virtual void notify(const std::string& absPath) {
		auto it = callbacks_.find(absPath);
		if (it != callbacks_.end()) {
			for (auto callback : it->second) {
				(*callback)();
			}
		}
	}

	std::unordered_map<std::string, std::unique_ptr<components::FileChangeListenerImpl>> listeners_;
	std::unordered_map<std::string, std::unordered_set<typename Base::Callback*>> callbacks_;
};

using FileChangeMonitorImpl = GenericFileChangeMonitorImpl<raco::core::FileChangeMonitor>;

using ExternalProjectFileChangeMonitor = GenericFileChangeMonitorImpl<raco::core::FileChangeMonitorInterface<std::function<void(void)>>>;


}  // namespace raco::core