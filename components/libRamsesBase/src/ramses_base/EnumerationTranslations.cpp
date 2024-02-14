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
	{user_types::ECullMode::Disabled, ramses::ECullMode::Disabled},
	{user_types::ECullMode::FrontFacing, ramses::ECullMode::FrontFacing},
	{user_types::ECullMode::BackFacing, ramses::ECullMode::BackFacing},
	{user_types::ECullMode::FrontAndBackFacing, ramses::ECullMode::FrontAndBackFacing}};

std::map<user_types::EBlendOperation, ramses::EBlendOperation> enumerationTranslationsBlendOperation{
	{user_types::EBlendOperation::Disabled, ramses::EBlendOperation::Disabled},
	{user_types::EBlendOperation::Add, ramses::EBlendOperation::Add},
	{user_types::EBlendOperation::Subtract, ramses::EBlendOperation::Subtract},
	{user_types::EBlendOperation::ReverseSubtract, ramses::EBlendOperation::ReverseSubtract},
	{user_types::EBlendOperation::Min, ramses::EBlendOperation::Min},
	{user_types::EBlendOperation::Max, ramses::EBlendOperation::Max}};

std::map<user_types::EBlendFactor, ramses::EBlendFactor> enumerationTranslationsBlendFactor{
	{user_types::EBlendFactor::Zero, ramses::EBlendFactor::Zero},
	{user_types::EBlendFactor::One, ramses::EBlendFactor::One},
	{user_types::EBlendFactor::SrcAlpha, ramses::EBlendFactor::SrcAlpha},
	{user_types::EBlendFactor::OneMinusSrcAlpha, ramses::EBlendFactor::OneMinusSrcAlpha},
	{user_types::EBlendFactor::DstAlpha, ramses::EBlendFactor::DstAlpha},
	{user_types::EBlendFactor::OneMinusDstAlpha, ramses::EBlendFactor::OneMinusDstAlpha},
	{user_types::EBlendFactor::SrcColor, ramses::EBlendFactor::SrcColor},
	{user_types::EBlendFactor::OneMinusSrcColor, ramses::EBlendFactor::OneMinusSrcColor},
	{user_types::EBlendFactor::DstColor, ramses::EBlendFactor::DstColor},
	{user_types::EBlendFactor::OneMinusDstColor, ramses::EBlendFactor::OneMinusDstColor},
	{user_types::EBlendFactor::ConstColor, ramses::EBlendFactor::ConstColor},
	{user_types::EBlendFactor::OneMinusConstColor, ramses::EBlendFactor::OneMinusConstColor},
	{user_types::EBlendFactor::ConstAlpha, ramses::EBlendFactor::ConstAlpha},
	{user_types::EBlendFactor::OneMinusConstAlpha, ramses::EBlendFactor::OneMinusConstAlpha},
	{user_types::EBlendFactor::AlphaSaturate, ramses::EBlendFactor::AlphaSaturate}};

std::map<user_types::EDepthFunc, ramses::EDepthFunc> enumerationTranslationsDepthFunc{
	{user_types::EDepthFunc::Disabled, ramses::EDepthFunc::Disabled},
	{user_types::EDepthFunc::Greater, ramses::EDepthFunc::Greater},
	{user_types::EDepthFunc::GreaterEqual, ramses::EDepthFunc::GreaterEqual},
	{user_types::EDepthFunc::Less, ramses::EDepthFunc::Less},
	{user_types::EDepthFunc::LessEqual, ramses::EDepthFunc::LessEqual},
	{user_types::EDepthFunc::Equal, ramses::EDepthFunc::Equal},
	{user_types::EDepthFunc::NotEqual, ramses::EDepthFunc::NotEqual},
	{user_types::EDepthFunc::Always, ramses::EDepthFunc::Always},
	{user_types::EDepthFunc::Never, ramses::EDepthFunc::Never}};

std::map<user_types::ETextureAddressMode, ramses::ETextureAddressMode> enumerationTranslationTextureAddressMode{
	{user_types::ETextureAddressMode::Clamp, ramses::ETextureAddressMode::Clamp},
	{user_types::ETextureAddressMode::Repeat, ramses::ETextureAddressMode::Repeat},
	{user_types::ETextureAddressMode::Mirror, ramses::ETextureAddressMode::Mirror}};

std::map<user_types::ETextureSamplingMethod, ramses::ETextureSamplingMethod> enumerationTranslationTextureSamplingMethod{
	{user_types::ETextureSamplingMethod::Nearest, ramses::ETextureSamplingMethod::Nearest},
	{user_types::ETextureSamplingMethod::Linear, ramses::ETextureSamplingMethod::Linear},
	{user_types::ETextureSamplingMethod::Nearest_MipMapNearest, ramses::ETextureSamplingMethod::Nearest_MipMapNearest},
	{user_types::ETextureSamplingMethod::Nearest_MipMapLinear, ramses::ETextureSamplingMethod::Nearest_MipMapLinear},
	{user_types::ETextureSamplingMethod::Linear_MipMapNearest, ramses::ETextureSamplingMethod::Linear_MipMapNearest},
	{user_types::ETextureSamplingMethod::Linear_MipMapLinear, ramses::ETextureSamplingMethod::Linear_MipMapLinear}};

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
	{user_types::ERenderBufferFormat::RGBA4, ramses::ERenderBufferFormat::RGBA4},
	{user_types::ERenderBufferFormat::R8, ramses::ERenderBufferFormat::R8},
	{user_types::ERenderBufferFormat::RG8, ramses::ERenderBufferFormat::RG8},
	{user_types::ERenderBufferFormat::RGB8, ramses::ERenderBufferFormat::RGB8},
	{user_types::ERenderBufferFormat::RGBA8, ramses::ERenderBufferFormat::RGBA8},
	{user_types::ERenderBufferFormat::R16F, ramses::ERenderBufferFormat::R16F},
	{user_types::ERenderBufferFormat::R32F, ramses::ERenderBufferFormat::R32F},
	{user_types::ERenderBufferFormat::RG16F, ramses::ERenderBufferFormat::RG16F},
	{user_types::ERenderBufferFormat::RG32F, ramses::ERenderBufferFormat::RG32F},
	{user_types::ERenderBufferFormat::RGB16F, ramses::ERenderBufferFormat::RGB16F},
	{user_types::ERenderBufferFormat::RGB32F, ramses::ERenderBufferFormat::RGB32F},
	{user_types::ERenderBufferFormat::RGBA16F, ramses::ERenderBufferFormat::RGBA16F},
	{user_types::ERenderBufferFormat::RGBA32F, ramses::ERenderBufferFormat::RGBA32F},

	{user_types::ERenderBufferFormat::Depth24, ramses::ERenderBufferFormat::Depth24},
	{user_types::ERenderBufferFormat::Depth24_Stencil8, ramses::ERenderBufferFormat::Depth24_Stencil8},
	{user_types::ERenderBufferFormat::Depth16, ramses::ERenderBufferFormat::Depth16},
	{user_types::ERenderBufferFormat::Depth32, ramses::ERenderBufferFormat::Depth32}};

}  // namespace raco::ramses_base
