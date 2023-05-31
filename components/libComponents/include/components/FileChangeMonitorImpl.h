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
	GenericFileChangeMonitorImpl() {
		listener_ = std::make_unique<FileChangeListenerImpl>([this](auto absPath) {
			notify(absPath);
		});
	}

	typename Base::UniqueListener registerFileChangedHandler(std::string absPath, typename Base::Callback callback) override {
		if (absPath.empty()) {
			return typename Base::UniqueListener(nullptr);
		}

		listener_->add(absPath);

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
				listener_->remove(absPath);
			}
		}
	}

	virtual void notify(const std::string& absPath) = 0;

	std::unique_ptr<components::FileChangeListenerImpl> listener_;
	std::unordered_map<std::string, std::unordered_set<typename Base::Callback*>> callbacks_;
};

class ProjectFileChangeMonitor : public GenericFileChangeMonitorImpl<raco::core::FileChangeMonitorInterface<std::function<void(void)>>> {
public:
	ProjectFileChangeMonitor() : GenericFileChangeMonitorImpl() {}

protected:
	void notify(const std::string& absPath) override {
		auto it = callbacks_.find(absPath);
		if (it != callbacks_.end()) {
			for (auto callback : it->second) {
				(*callback)();
			}
		}
	}
};

}  // namespace raco::core