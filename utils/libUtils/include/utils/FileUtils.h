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

#include <string>
#include <vector>

namespace raco::utils::file {

using Path = std::string;
std::string read(const Path& path);
std::vector<unsigned char> readBinary(const Path& path);
void write(const Path& path, const std::string& content);

}  // namespace raco::utils::file
