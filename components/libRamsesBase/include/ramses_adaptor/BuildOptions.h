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

#include <ramses-client-api/ERotationConvention.h>

namespace raco::ramses_adaptor {
    // In order to adapt to the Euler angle rotation order of CID, modify it as ZYX.
    constexpr auto RAMSES_ROTATION_CONVENTION { ramses::ERotationConvention::ZYX }; // XYZ
}
