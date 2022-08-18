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

#include <string>

namespace raco::utils::zip {

constexpr auto ZIP_COMPRESSION_LEVEL = 10;

struct UnZipStatus {
	bool success;
	std::string payload;
};

std::string projectToZip(const char* fileContents, const char* projectFileName);
UnZipStatus zipToProject(const char* fileContents, int fileContentSize);
bool isZipFile(const std::string& fileContents);

}  // namespace raco::utils::zip
