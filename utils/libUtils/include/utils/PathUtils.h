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

namespace raco::utils::path {

bool exists(const std::string& path);

bool userHasReadAccess(const std::string& path);

bool isExistingDirectory(const std::string& path);

bool isExistingFile(const std::string& path);

}  // namespace raco::utils::path
