/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_base/EnumerationTranslations.h"

namespace raco::ramses_base {
std::map<user_types::ECullMode, ramses::ECullMode> enumerationTranslationsCullMode{
	{user_types::ECullMode::Disabled, ramses::ECullMode_Disabled},
	{user_types::ECullMode::FrontFacing, ramses::ECullMode_FrontFacing},
	{user_types::ECullMode::BackFacing, ramses::ECullMode_BackFacing},
	{user_types::ECullMode::FrontAndBackFacing, ramses::ECullMode_FrontAndBackFacing}};

std::map<user_types::EBlendOperation, ramses::EBlendOperation> enumerationTranslationsBlendOperation{
	{user_types::EBlendOperation::Disabled, ramses::EBlendOperation_Disabled},
	{user_types::EBlendOperation::Add, ramses::EBlendOperation_Add},
	{user_types::EBlendOperation::Subtract, ramses::EBlendOperation_Subtract},
	{user_types::EBlendOperation::ReverseSubtract, ramses::EBlendOperation_ReverseSubtract},
	{user_types::EBlendOperation::Min, ramses::EBlendOperation_Min},
	{user_types::EBlendOperation::Max, ramses::EBlendOperation_Max}};

std::map<user_types::EBlendFactor, ramses::EBlendFactor> enumerationTranslationsBlendFactor{
	{user_types::EBlendFactor::Zero, ramses::EBlendFactor_Zero},
	{user_types::EBlendFactor::One, ramses::EBlendFactor_One},
	{user_types::EBlendFactor::SrcAlpha, ramses::EBlendFactor_SrcAlpha},
	{user_types::EBlendFactor::OneMinusSrcAlpha, ramses::EBlendFactor_OneMinusSrcAlpha},
	{user_types::EBlendFactor::DstAlpha, ramses::EBlendFactor_DstAlpha},
	{user_types::EBlendFactor::OneMinusDstAlpha, ramses::EBlendFactor_OneMinusDstAlpha},
	{user_types::EBlendFactor::SrcColor, ramses::EBlendFactor_SrcColor},
	{user_types::EBlendFactor::OneMinusSrcColor, ramses::EBlendFactor_OneMinusSrcColor},
	{user_types::EBlendFactor::DstColor, ramses::EBlendFactor_DstColor},
	{user_types::EBlendFactor::OneMinusDstColor, ramses::EBlendFactor_OneMinusDstColor},
	{user_types::EBlendFactor::ConstColor, ramses::EBlendFactor_ConstColor},
	{user_types::EBlendFactor::OneMinusConstColor, ramses::EBlendFactor_OneMinusConstColor},
	{user_types::EBlendFactor::ConstAlpha, ramses::EBlendFactor_ConstAlpha},
	{user_types::EBlendFactor::OneMinusConstAlpha, ramses::EBlendFactor_OneMinusConstAlpha},
	{user_types::EBlendFactor::AlphaSaturate, ramses::EBlendFactor_AlphaSaturate}};

std::map<user_types::EDepthFunc, ramses::EDepthFunc> enumerationTranslationsDepthFunc{
	{user_types::EDepthFunc::Disabled, ramses::EDepthFunc_Disabled},
	{user_types::EDepthFunc::Greater, ramses::EDepthFunc_Greater},
	{user_types::EDepthFunc::GreaterEqual, ramses::EDepthFunc_GreaterEqual},
	{user_types::EDepthFunc::Less, ramses::EDepthFunc_Less},
	{user_types::EDepthFunc::LessEqual, ramses::EDepthFunc_LessEqual},
	{user_types::EDepthFunc::Equal, ramses::EDepthFunc_Equal},
	{user_types::EDepthFunc::NotEqual, ramses::EDepthFunc_NotEqual},
	{user_types::EDepthFunc::Always, ramses::EDepthFunc_Always},
	{user_types::EDepthFunc::Never, ramses::EDepthFunc_Never}};

std::map<user_types::ETextureAddressMode, ramses::ETextureAddressMode> enumerationTranslationTextureAddressMode{
	{user_types::ETextureAddressMode::Clamp, ramses::ETextureAddressMode_Clamp},
	{user_types::ETextureAddressMode::Repeat, ramses::ETextureAddressMode_Repeat},
	{user_types::ETextureAddressMode::Mirror, ramses::ETextureAddressMode_Mirror}};

std::map<user_types::ETextureSamplingMethod, ramses::ETextureSamplingMethod> enumerationTranslationTextureSamplingMethod{
	{user_types::ETextureSamplingMethod::Nearest, ramses::ETextureSamplingMethod_Nearest},
	{user_types::ETextureSamplingMethod::Linear, ramses::ETextureSamplingMethod_Linear},
	{user_types::ETextureSamplingMethod::Nearest_MipMapNearest, ramses::ETextureSamplingMethod_Nearest_MipMapNearest},
	{user_types::ETextureSamplingMethod::Nearest_MipMapLinear, ramses::ETextureSamplingMethod_Nearest_MipMapLinear},
	{user_types::ETextureSamplingMethod::Linear_MipMapNearest, ramses::ETextureSamplingMethod_Linear_MipMapNearest},
	{user_types::ETextureSamplingMethod::Linear_MipMapLinear, ramses::ETextureSamplingMethod_Linear_MipMapLinear}};

std::map<user_types::ETextureFormat, ramses::ETextureFormat> enumerationTranslationTextureFormat{
	{user_types::ETextureFormat::R8, ramses::ETextureFormat::R8},
	{user_types::ETextureFormat::RG8, ramses::ETextureFormat::RG8},
	{user_types::ETextureFormat::RGB8, ramses::ETextureFormat::RGB8},
	{user_types::ETextureFormat::RGBA8, ramses::ETextureFormat::RGBA8},
	{user_types::ETextureFormat::RGB16F, ramses::ETextureFormat::RGB16F},
	{user_types::ETextureFormat::RGBA16F, ramses::ETextureFormat::RGBA16F},
	{user_types::ETextureFormat::SRGB8, ramses::ETextureFormat::SRGB8},
	{user_types::ETextureFormat::SRGB8_ALPHA8, ramses::ETextureFormat::SRGB8_ALPHA8}};

std::map<user_types::ERenderBufferFormat, ramses::ERenderBufferFormat> enumerationTranslationRenderBufferFormat{
	{user_types::ERenderBufferFormat::RGBA4, ramses::ERenderBufferFormat_RGBA4},
	{user_types::ERenderBufferFormat::R8, ramses::ERenderBufferFormat_R8},
	{user_types::ERenderBufferFormat::RG8, ramses::ERenderBufferFormat_RG8},
	{user_types::ERenderBufferFormat::RGB8, ramses::ERenderBufferFormat_RGB8},
	{user_types::ERenderBufferFormat::RGBA8, ramses::ERenderBufferFormat_RGBA8},
	{user_types::ERenderBufferFormat::R16F, ramses::ERenderBufferFormat_R16F},
	{user_types::ERenderBufferFormat::R32F, ramses::ERenderBufferFormat_R32F},
	{user_types::ERenderBufferFormat::RG16F, ramses::ERenderBufferFormat_RG16F},
	{user_types::ERenderBufferFormat::RG32F, ramses::ERenderBufferFormat_RG32F},
	{user_types::ERenderBufferFormat::RGB16F, ramses::ERenderBufferFormat_RGB16F},
	{user_types::ERenderBufferFormat::RGB32F, ramses::ERenderBufferFormat_RGB32F},
	{user_types::ERenderBufferFormat::RGBA16F, ramses::ERenderBufferFormat_RGBA16F},
	{user_types::ERenderBufferFormat::RGBA32F, ramses::ERenderBufferFormat_RGBA32F},

	{user_types::ERenderBufferFormat::Depth24, ramses::ERenderBufferFormat_Depth24},
	{user_types::ERenderBufferFormat::Depth24_Stencil8, ramses::ERenderBufferFormat_Depth24_Stencil8}};

}  // namespace raco::ramses_base
