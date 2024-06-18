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

#include "user_types/Enumerations.h"

#include "core/MeshCacheInterface.h"

#include <ramses/framework/AppearanceEnums.h>
#include <ramses/framework/TextureEnums.h>
#include <ramses/client/logic/AnimationTypes.h>

#include <map>
#include <string>

namespace raco::ramses_base {
extern std::map<user_types::ECullMode, ramses::ECullMode> enumerationTranslationsCullMode;

extern std::map<user_types::EDepthFunc, ramses::EDepthFunc> enumerationTranslationsDepthFunc;

extern std::map<user_types::EBlendOperation, ramses::EBlendOperation> enumerationTranslationsBlendOperation;

extern std::map<user_types::EBlendFactor, ramses::EBlendFactor> enumerationTranslationsBlendFactor;

extern std::map<user_types::ERenderBufferFormat, ramses::ERenderBufferFormat> enumerationTranslationRenderBufferFormat;

extern std::map<user_types::ETextureFormat, ramses::ETextureFormat> enumerationTranslationTextureFormat;

extern std::map<user_types::ETextureAddressMode, ramses::ETextureAddressMode> enumerationTranslationTextureAddressMode;
extern std::map<user_types::ETextureSamplingMethod, ramses::ETextureSamplingMethod> enumerationTranslationTextureSamplingMethod;

extern std::map<core::MeshAnimationInterpolation, ramses::EInterpolationType> enumerationTranslationAnimationInterpolationType;


}  // namespace raco::ramses_base
