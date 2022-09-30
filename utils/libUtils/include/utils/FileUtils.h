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
#include <vector>
#include "u8path.h"

namespace raco::utils::file {

std::string read(const u8path& path);
std::vector<unsigned char> readBinary(const u8path& path);
void write(const u8path& path, const std::string& content);
bool isGitLfsPlaceholderFile(const u8path& path);

}  // namespace raco::utils::file
