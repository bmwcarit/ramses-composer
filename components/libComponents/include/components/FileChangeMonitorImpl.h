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

#include <memory>
#include <unordered_set>

namespace raco::core {
	class BaseContext;
}

namespace raco::components {
class FileChangeListenerImpl;

class FileChangeMonitorImpl :  public core::FileChangeMonitor {
public:	
	FileChangeMonitorImpl(core::BaseContext& context) : context_(context) {}

	UniqueListener registerFileChangedHandler(std::string absPath, core::FileChangeCallback callback) override;
	
private:
	void unregister(std::string absPath, core::FileChangeListener* listener);

	std::unordered_map<std::string, std::unordered_set<core::FileChangeListener*>> listeners_;
	core::BaseContext& context_;

	friend class FileChangeListenerImpl;
};

class ExternalProjectFileChangeMonitor {
public:
	using Del = std::function<void(core::FileChangeListener*)>;
	using UniqueListener = std::unique_ptr<core::FileChangeListener, Del>;
	using Callback = std::function<void(void)>;

	UniqueListener registerFileChangedHandler(std::string absPath, Callback callback);

private:
	void unregister(std::string absPath, core::FileChangeListener* listener);

	std::unordered_map<std::string, std::unordered_set<core::FileChangeListener*>> listeners_;

	friend class FileChangeListenerImpl;
};

}  // namespace raco::core