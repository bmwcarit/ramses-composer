/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/Enumerations.h"
#include <map>
#include <string>

namespace raco::user_types {

std::map<int, std::string> enumerationTextureOrigin{
	{static_cast<int>(user_types::ETextureOrigin::Bottom), "Bottom left (OpenGL)"},
	{static_cast<int>(user_types::ETextureOrigin::Top), "Top left (Direct 3D)"}};

std::map<int, std::string> enumerationRenderLayerOrder{
	{static_cast<int>(ERenderLayerOrder::Optimized), "Optimized"},
	{static_cast<int>(ERenderLayerOrder::Manual), "By Priority"},
	{static_cast<int>(ERenderLayerOrder::SceneGraph), "Scene Graph"}};

std::map<int, std::string> enumerationRenderLayerMaterialFilterFlag{
	{static_cast<int>(false), "Include materials with any of the listed tags"},
	{static_cast<int>(true), "Exclude material with any of the listed tags"}};

}  // namespace raco::user_types