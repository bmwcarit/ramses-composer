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

#include <map>
#include <string>

namespace raco::user_types {

constexpr int TEXTURE_ORIGIN_BOTTOM(0);
constexpr int TEXTURE_ORIGIN_TOP(1);

extern std::map<int, std::string> enumerationTextureOrigin;
}