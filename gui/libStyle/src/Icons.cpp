/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "style/Icons.h"

namespace raco::style {
	
const Icons& Icons::instance() {
	static Icons icons;
	return icons;
}

Icons::Icons() {
}

Icons::~Icons() {
}

}  // namespace raco::style