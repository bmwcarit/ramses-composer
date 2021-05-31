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

class FileChangeListener {
public:
	static constexpr int DELAYED_FILE_LOAD_TIME_MSEC = 100;
	
	virtual ~FileChangeListener() = default;

	virtual std::string getPath() const = 0;
};

class FileChangeMonitor {
public:		
	virtual ~FileChangeMonitor() = default;
	
	using Del = std::function<void(FileChangeListener*)>;
	using UniqueListener = std::unique_ptr<FileChangeListener, Del>;

	virtual UniqueListener registerFileChangedHandler(std::string absPath, FileChangeCallback callback) = 0;
};

}  // namespace raco::core