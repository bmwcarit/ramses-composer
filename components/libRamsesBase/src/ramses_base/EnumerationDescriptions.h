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
#include <ramses-client-api/AppearanceEnums.h>
#include <ramses-client-api/TextureEnums.h>

#include <string>

namespace raco::ramses_base {
std::map<int, std::string> enumerationEmpty;
std::map<int, std::string> enumerationCullMode{
	{ramses::ECullMode_Disabled, "Disabled"},
	{ramses::ECullMode_FrontFacing, "Front"},
	{ramses::ECullMode_BackFacing, "Back"},
	{ramses::ECullMode_FrontAndBackFacing, "Front and Back"}};
std::map<int, std::string> enumerationBlendOperation{
	{ramses::EBlendOperation_Disabled, "Disabled"},
	{ramses::EBlendOperation_Add, "Add"},
	{ramses::EBlendOperation_Subtract, "Substract"},
	{ramses::EBlendOperation_ReverseSubtract, "Reverse Substract"},
	{ramses::EBlendOperation_Min, "Min"},
	{ramses::EBlendOperation_Max, "Max"}};
std::map<int, std::string> enumerationBlendFactor{
	{ramses::EBlendFactor_Zero, "Zero"},
	{ramses::EBlendFactor_One, "One"},
	{ramses::EBlendFactor_SrcAlpha, "Src Alpha"},
	{ramses::EBlendFactor_OneMinusSrcAlpha, "One minus Src Alpha"},
	{ramses::EBlendFactor_DstAlpha, "Dst Alpha"},
	{ramses::EBlendFactor_OneMinusDstAlpha, "One minus Dst Alpha"},
	{ramses::EBlendFactor_SrcColor, "Src Color"},
	{ramses::EBlendFactor_OneMinusSrcColor, "One minus Src Color"},
	{ramses::EBlendFactor_DstColor, "Dst Color"},
	{ramses::EBlendFactor_OneMinusDstColor, "One minus Dst Color"},
	{ramses::EBlendFactor_ConstColor, "Const Color"},
	{ramses::EBlendFactor_OneMinusConstColor, "One minus Const Color"},
	{ramses::EBlendFactor_ConstAlpha, "Const Alpha"},
	{ramses::EBlendFactor_OneMinusConstAlpha, "One minus Const Alpha"},
	{ramses::EBlendFactor_AlphaSaturate, "Alpha Saturated"}};
std::map<int, std::string> enumerationDepthFunction{
	{ramses::EDepthFunc_Disabled, "Disabled"},
	{ramses::EDepthFunc_Greater, ">"},
	{ramses::EDepthFunc_GreaterEqual, ">="},
	{ramses::EDepthFunc_Less, "<"},
	{ramses::EDepthFunc_LessEqual, "<="},
	{ramses::EDepthFunc_Equal, "="},
	{ramses::EDepthFunc_NotEqual, "!="},
	{ramses::EDepthFunc_Always, "true"},
	{ramses::EDepthFunc_Never, "false"}};
std::map<int, std::string> enumerationTextureAddressMode{
	{ramses::ETextureAddressMode_Clamp, "Clamp"},
	{ramses::ETextureAddressMode_Repeat, "Repeat"},
	{ramses::ETextureAddressMode_Mirror, "Mirror"}};
std::map<int, std::string> enumerationTextureMinSamplingMethod{
	{ramses::ETextureSamplingMethod_Nearest, "Nearest"},
	{ramses::ETextureSamplingMethod_Linear, "Linear"},
	{ramses::ETextureSamplingMethod_Nearest_MipMapNearest, "Nearest MipMapNearest"},
	{ramses::ETextureSamplingMethod_Nearest_MipMapLinear, "Nearest MipMapLinear"},
	{ramses::ETextureSamplingMethod_Linear_MipMapNearest, "Linear MipMapNearest"},
	{ramses::ETextureSamplingMethod_Linear_MipMapLinear, "Linear MipMapLinear"}};
std::map<int, std::string> enumerationTextureMagSamplingMethod{
	{ramses::ETextureSamplingMethod_Nearest, "Nearest"},
	{ramses::ETextureSamplingMethod_Linear, "Linear"}};
std::map<int, std::string> enumerationTextureFormat{
	{static_cast<int>(ramses::ETextureFormat::Invalid), "Invalid"},
	{static_cast<int>(ramses::ETextureFormat::R8), "R8"},
	{static_cast<int>(ramses::ETextureFormat::RG8), "RG8"},
	{static_cast<int>(ramses::ETextureFormat::RGB8), "RGB8"},
	{static_cast<int>(ramses::ETextureFormat::RGB565), "RGB565"},
	{static_cast<int>(ramses::ETextureFormat::RGBA8), "RGBA8"},
	{static_cast<int>(ramses::ETextureFormat::RGBA4), "RGBA4"},
	{static_cast<int>(ramses::ETextureFormat::RGBA5551), "RGBA5551"},
	{static_cast<int>(ramses::ETextureFormat::ETC2RGB), "ETC2RGB"},
	{static_cast<int>(ramses::ETextureFormat::ETC2RGBA), "ETC2RGBA"},
	{static_cast<int>(ramses::ETextureFormat::R16F), "R16F"},
	{static_cast<int>(ramses::ETextureFormat::R32F), "R32F"},
	{static_cast<int>(ramses::ETextureFormat::RG16F), "RG16F"},
	{static_cast<int>(ramses::ETextureFormat::RG32F), "RG32F"},
	{static_cast<int>(ramses::ETextureFormat::RGB16F), "RGB16F"},
	{static_cast<int>(ramses::ETextureFormat::RGB32F), "RGB32F"},
	{static_cast<int>(ramses::ETextureFormat::RGBA16F), "RGBA16F"},
	{static_cast<int>(ramses::ETextureFormat::RGBA32F), "RGBA32F"},
	{static_cast<int>(ramses::ETextureFormat::SRGB8), "SRGB8"},
	{static_cast<int>(ramses::ETextureFormat::SRGB8_ALPHA8), "SRGB8_ALPHA8"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_4x4), "ASTC_RGBA_4x4"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_5x4), "ASTC_RGBA_5x4"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_5x5), "ASTC_RGBA_5x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_6x5), "ASTC_RGBA_6x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_6x6), "ASTC_RGBA_6x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_8x5), "ASTC_RGBA_8x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_8x6), "ASTC_RGBA_8x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_8x8), "ASTC_RGBA_8x8"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_10x5), "ASTC_RGBA_10x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_10x6), "ASTC_RGBA_10x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_10x8), "ASTC_RGBA_10x8"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_10x10), "ASTC_RGBA_10x10"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_12x10), "ASTC_RGBA_12x10"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_RGBA_12x12), "ASTC_RGBA_12x12"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_4x4), "ASTC_SRGBA_4x4"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_5x4), "ASTC_SRGBA_5x4"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_5x5), "ASTC_SRGBA_5x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_6x5), "ASTC_SRGBA_6x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_6x6), "ASTC_SRGBA_6x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_8x5), "ASTC_SRGBA_8x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_8x6), "ASTC_SRGBA_8x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_8x8), "ASTC_SRGBA_8x8"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_10x5), "ASTC_SRGBA_10x5"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_10x6), "ASTC_SRGBA_10x6"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_10x8), "ASTC_SRGBA_10x8"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_10x10), "ASTC_SRGBA_10x10"},
	{static_cast<int>(ramses::ETextureFormat::ASTC_SRGBA_12x10), "ASTC_SRGBA_12x12"},
};
}  // namespace raco::ramses_base