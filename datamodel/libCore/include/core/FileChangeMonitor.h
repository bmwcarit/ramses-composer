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

#include "FileChangeCallback.h"

#include <memory>

namespace raco::core {

template<typename TCallback>
class FileChangeMonitorInterface {
public:		
	virtual ~FileChangeMonitorInterface() = default;
	
	using Callback = TCallback;
	using Del = std::function<void(Callback*)>;
	using UniqueListener = std::unique_ptr<Callback, Del>;

	virtual UniqueListener registerFileChangedHandler(std::string absPath, Callback callback) = 0;
};

using FileChangeMonitor = FileChangeMonitorInterface<core::FileChangeCallback>;

}  // amespace raco::core