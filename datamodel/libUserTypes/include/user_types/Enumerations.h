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

#include <map>
#include <string>

namespace raco::user_types {

enum class ERenderLayerOrder {
	Manual = 0,
	SceneGraph
};

extern std::map<int, std::string> enumerationRenderLayerOrder;


enum class ERenderLayerMaterialFilterMode {
	Inclusive = 0,
	Exclusive
};

extern std::map<int, std::string> enumerationRenderLayerMaterialFilterMode;

}