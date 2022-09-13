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

#include <array>
#include <vector>
#include <string>

namespace raco::utils::math {

std::array<double, 3> eulerAngle(double lastX, double lastY, double lastZ, double curX, double curY, double curZ);

std::array<double, 3> quaternionToXYZDegrees(double qX, double qY, double qZ, double qW);

uint16_t twoBytesToHalfFloat(unsigned char firstByte, unsigned char secondByte);

}  // namespace raco::utils::math
