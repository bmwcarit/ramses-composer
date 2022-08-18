/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/RenderPass.h"

namespace raco::user_types {

bool RenderPass::isClearTargetProperty(ValueHandle const& handle) const {
	if (handle.depth() == 0) {
		return false;
	}
	std::string const& pn = handle.getPropName();
	return pn == "clearColor" || pn == "enableClearColor" || pn == "enableClearDepth" || pn == "enableClearStencil";
}

}  // namespace raco::user_types