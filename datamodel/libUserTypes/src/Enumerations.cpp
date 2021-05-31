/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <map>
#include <string>
#include "user_types/Enumerations.h"

namespace raco::user_types {

std::map<int, std::string> enumerationTextureOrigin{
	{TEXTURE_ORIGIN_BOTTOM, "Bottom left (OpenGL)"},
	{TEXTURE_ORIGIN_TOP, "Top left (Direct 3D)"}};
}