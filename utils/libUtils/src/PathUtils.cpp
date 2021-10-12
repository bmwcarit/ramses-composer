/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "utils/PathUtils.h"

#include "utils/stdfilesystem.h"
#include <fstream>

namespace raco::utils::path {

bool exists(const std::string& path) {
	std::error_code ec;
	auto status = std::filesystem::status(path, ec);
	if (!ec) {
		return std::filesystem::exists(status);
	}

	return false;
}

bool userHasReadAccess(const std::string& path) {
	std::ifstream stream(path);
	return stream.good();
}

bool isExistingDirectory(const std::string& path) {
	return exists(path) && std::filesystem::is_directory(path);
}

bool isExistingFile(const std::string& path) {
	return exists(path) && !std::filesystem::is_directory(path);
}

}  // namespace raco::utils::path
