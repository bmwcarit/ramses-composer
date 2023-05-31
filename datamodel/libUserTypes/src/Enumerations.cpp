/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/Enumerations.h"

#include <map>
#include <string>
#include <cassert>

namespace raco::user_types {

const std::map<int, std::string>& enumerationDescription(core::EUserTypeEnumerations type) {
	static const std::map<int, std::string> enumerationEmpty;

	switch (type) {
		case core::EUserTypeEnumerations::CullMode:
			return raco::user_types::enumDescriptionCullMode;
		case core::EUserTypeEnumerations::BlendOperation:
			return raco::user_types::enumDescriptionBlendOperation;
		case core::EUserTypeEnumerations::BlendFactor:
			return raco::user_types::enumDescriptionBlendFactor;
		case core::EUserTypeEnumerations::DepthFunction:
			return raco::user_types::enumDescriptionDepthFunction;
		case core::EUserTypeEnumerations::TextureAddressMode:
			return raco::user_types::enumDescriptionTextureAddressMode;
		case core::EUserTypeEnumerations::TextureMinSamplingMethod:
			return raco::user_types::enumDescriptionTextureMinSamplingMethod;
		case core::EUserTypeEnumerations::TextureMagSamplingMethod:
			return raco::user_types::enumDescriptionTextureMagSamplingMethod;
		case core::EUserTypeEnumerations::TextureFormat:
			return raco::user_types::enumDescriptionTextureFormat;

		case core::EUserTypeEnumerations::RenderBufferFormat:
			return raco::user_types::enumDescriptionRenderBufferFormat;

		case core::EUserTypeEnumerations::RenderLayerOrder:
			return raco::user_types::enumDescriptionRenderLayerOrder;

		case core::EUserTypeEnumerations::RenderLayerMaterialFilterMode:
			return raco::user_types::enumDescriptionRenderLayerMaterialFilterMode;

		case core::EUserTypeEnumerations::FrustumType:
			return raco::user_types::enumDescriptionFrustumType;


		case core::EUserTypeEnumerations::StencilFunction:
			return raco::user_types::enumDescriptionStencilFunction;
		case core::EUserTypeEnumerations::StencilOperation:
			return raco::user_types::enumDescriptionStencilOperation;

		default:
			assert(false);
			return enumerationEmpty;
	}
}

std::map<int, std::string> enumDescriptionCullMode{
	{static_cast<int>(ECullMode::Disabled), "Disabled"},
	{static_cast<int>(ECullMode::FrontFacing), "Front"},
	{static_cast<int>(ECullMode::BackFacing), "Back"},
	{static_cast<int>(ECullMode::FrontAndBackFacing), "Front and Back"}};

std::map<int, std::string> enumDescriptionBlendOperation{
	{static_cast<int>(EBlendOperation::Disabled), "Disabled"},
	{static_cast<int>(EBlendOperation::Add), "Add"},
	{static_cast<int>(EBlendOperation::Subtract), "Subtract"},
	{static_cast<int>(EBlendOperation::ReverseSubtract), "Reverse Subtract"},
	{static_cast<int>(EBlendOperation::Min), "Min"},
	{static_cast<int>(EBlendOperation::Max), "Max"}};

std::map<int, std::string> enumDescriptionBlendFactor{
	{static_cast<int>(EBlendFactor::Zero), "Zero"},
	{static_cast<int>(EBlendFactor::One), "One"},
	{static_cast<int>(EBlendFactor::SrcAlpha), "Src Alpha"},
	{static_cast<int>(EBlendFactor::OneMinusSrcAlpha), "One minus Src Alpha"},
	{static_cast<int>(EBlendFactor::DstAlpha), "Dst Alpha"},
	{static_cast<int>(EBlendFactor::OneMinusDstAlpha), "One minus Dst Alpha"},
	{static_cast<int>(EBlendFactor::SrcColor), "Src Color"},
	{static_cast<int>(EBlendFactor::OneMinusSrcColor), "One minus Src Color"},
	{static_cast<int>(EBlendFactor::DstColor), "Dst Color"},
	{static_cast<int>(EBlendFactor::OneMinusDstColor), "One minus Dst Color"},
	{static_cast<int>(EBlendFactor::ConstColor), "Const Color"},
	{static_cast<int>(EBlendFactor::OneMinusConstColor), "One minus Const Color"},
	{static_cast<int>(EBlendFactor::ConstAlpha), "Const Alpha"},
	{static_cast<int>(EBlendFactor::OneMinusConstAlpha), "One minus Const Alpha"},
	{static_cast<int>(EBlendFactor::AlphaSaturate), "Alpha Saturated"}};

std::map<int, std::string> enumDescriptionDepthFunction{
	{static_cast<int>(EDepthFunc::Disabled), "Disabled"},
	{static_cast<int>(EDepthFunc::Greater), ">"},
	{static_cast<int>(EDepthFunc::GreaterEqual), ">="},
	{static_cast<int>(EDepthFunc::Less), "<"},
	{static_cast<int>(EDepthFunc::LessEqual), "<="},
	{static_cast<int>(EDepthFunc::Equal), "="},
	{static_cast<int>(EDepthFunc::NotEqual), "!="},
	{static_cast<int>(EDepthFunc::Always), "true"},
	{static_cast<int>(EDepthFunc::Never), "false"}};

std::map<int, std::string> enumDescriptionStencilFunction{
	{static_cast<int>(EStencilFunc::Disabled), "Disabled"},
	{static_cast<int>(EStencilFunc::Never), "Never"},
	{static_cast<int>(EStencilFunc::Always), "Always"},
	{static_cast<int>(EStencilFunc::Equal), "="},
	{static_cast<int>(EStencilFunc::NotEqual), "!="},
	{static_cast<int>(EStencilFunc::Less), "<"},
	{static_cast<int>(EStencilFunc::LessEqual), "<="},
	{static_cast<int>(EStencilFunc::Greater), ">"},
	{static_cast<int>(EStencilFunc::GreaterEqual), ">="}
};

std::map<int, std::string> enumDescriptionStencilOperation{
	{static_cast<int>(EStencilOperation::Keep), "Keep"},
	{static_cast<int>(EStencilOperation::Zero), "Zero"},
	{static_cast<int>(EStencilOperation::Replace), "Replace"},
	{static_cast<int>(EStencilOperation::Increment), "Increment"},
	{static_cast<int>(EStencilOperation::IncrementWrap), "IncrementWrap"},
	{static_cast<int>(EStencilOperation::Decrement), "Decrement"},
	{static_cast<int>(EStencilOperation::DecrementWrap), "DecrementWrap"},
	{static_cast<int>(EStencilOperation::Invert), "Invert"}
};

std::map<int, std::string> enumDescriptionTextureAddressMode{
	{static_cast<int>(ETextureAddressMode::Clamp), "Clamp"},
	{static_cast<int>(ETextureAddressMode::Repeat), "Repeat"},
	{static_cast<int>(ETextureAddressMode::Mirror), "Mirror"}};

std::map<int, std::string> enumDescriptionTextureMinSamplingMethod{
	{static_cast<int>(ETextureSamplingMethod::Nearest), "Nearest"},
	{static_cast<int>(ETextureSamplingMethod::Linear), "Linear"},
	{static_cast<int>(ETextureSamplingMethod::Nearest_MipMapNearest), "Nearest MipMapNearest"},
	{static_cast<int>(ETextureSamplingMethod::Nearest_MipMapLinear), "Nearest MipMapLinear"},
	{static_cast<int>(ETextureSamplingMethod::Linear_MipMapNearest), "Linear MipMapNearest"},
	{static_cast<int>(ETextureSamplingMethod::Linear_MipMapLinear), "Linear MipMapLinear"}};

std::map<int, std::string> enumDescriptionTextureMagSamplingMethod{
	{static_cast<int>(ETextureSamplingMethod::Nearest), "Nearest"},
	{static_cast<int>(ETextureSamplingMethod::Linear), "Linear"}};

std::map<int, std::string> enumDescriptionTextureFormat{
	{static_cast<int>(ETextureFormat::R8), "R8"},
	{static_cast<int>(ETextureFormat::RG8), "RG8"},
	{static_cast<int>(ETextureFormat::RGB8), "RGB8"},
	{static_cast<int>(ETextureFormat::RGBA8), "RGBA8"},
	{static_cast<int>(ETextureFormat::RGB16F), "RGB16F"},
	{static_cast<int>(ETextureFormat::RGBA16F), "RGBA16F"},
	{static_cast<int>(ETextureFormat::SRGB8), "SRGB8"},
	{static_cast<int>(ETextureFormat::SRGB8_ALPHA8), "SRGB8_ALPHA8"},
};

std::map<int, std::string> enumDescriptionRenderBufferFormat{
	{static_cast<int>(ERenderBufferFormat::RGBA4), "RGBA4"},
	{static_cast<int>(ERenderBufferFormat::R8), "R8"},
	{static_cast<int>(ERenderBufferFormat::RG8), "RG8"},
	{static_cast<int>(ERenderBufferFormat::RGB8), "RGB8"},
	{static_cast<int>(ERenderBufferFormat::RGBA8), "RGBA8"},
	{static_cast<int>(ERenderBufferFormat::R16F), "R16F"},
	{static_cast<int>(ERenderBufferFormat::R32F), "R32F"},
	{static_cast<int>(ERenderBufferFormat::RG16F), "RG16F"},
	{static_cast<int>(ERenderBufferFormat::RG32F), "RG32F"},
	{static_cast<int>(ERenderBufferFormat::RGB16F), "RGB16F"},
	{static_cast<int>(ERenderBufferFormat::RGB32F), "RGB32F"},
	{static_cast<int>(ERenderBufferFormat::RGBA16F), "RGBA16F"},
	{static_cast<int>(ERenderBufferFormat::RGBA32F), "RGBA32F"},

	{static_cast<int>(ERenderBufferFormat::Depth24), "Depth24"},
	{static_cast<int>(ERenderBufferFormat::Depth24_Stencil8), "Depth24_Stencil8"}};

std::map<int, std::string> enumDescriptionRenderLayerOrder{
	{static_cast<int>(ERenderLayerOrder::Manual), "Render order value in 'Renderable Tags'"},
	{static_cast<int>(ERenderLayerOrder::SceneGraph), "Scene graph order"}};

std::map<int, std::string> enumDescriptionRenderLayerMaterialFilterMode{
	{static_cast<int>(ERenderLayerMaterialFilterMode::Inclusive), "Include materials with any of the listed tags"},
	{static_cast<int>(ERenderLayerMaterialFilterMode::Exclusive), "Exclude materials with any of the listed tags"}};

std::map<int, std::string> enumDescriptionFrustumType{
	{static_cast<int>(EFrustumType::Aspect_FieldOfView), "Aspect & Field Of View"},
	{static_cast<int>(EFrustumType::Planes), "Planes"}};

}  // namespace raco::user_types