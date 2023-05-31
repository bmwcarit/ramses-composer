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

#include "core/CoreAnnotations.h"

#include <map>
#include <string>

namespace raco::user_types {

const std::map<int, std::string>& enumerationDescription(core::EUserTypeEnumerations type);

// !!! Careful: !!!
// We can't change the numeric value of the members of any of the enumerations below since they 
// define the property values in the data model which are serialized.

// items match ramses::ECullMode
enum class ECullMode {
	Disabled = 0,
	FrontFacing,
	BackFacing,
	FrontAndBackFacing,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionCullMode;


// item match ramses::EBlendOperation
enum class EBlendOperation {
	Disabled = 0,
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionBlendOperation;


	// items match ramses::EBlendFactor
enum class EBlendFactor {
	Zero = 0,
	One,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	ConstColor,
	OneMinusConstColor,
	ConstAlpha,
	OneMinusConstAlpha,
	AlphaSaturate,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionBlendFactor;


// items match ramses::EDepthFunc
enum class EDepthFunc {
	Disabled = 0,
	Greater,
	GreaterEqual,
	Less,
	LessEqual,
	Equal,
	NotEqual,
	Always,
	Never,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionDepthFunction;


// items mach ramses::EStencilFunc
enum class EStencilFunc {
	Disabled = 0,
	Never,
	Always,
	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionStencilFunction;


// items match ramses::EStencilOperation
enum class EStencilOperation {
	Keep = 0,
	Zero,
	Replace,
	Increment,
	IncrementWrap,
	Decrement,
	DecrementWrap,
	Invert,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionStencilOperation;


// items match ETextureAddressMode
enum class ETextureAddressMode {
	Clamp = 0,
	Repeat,
	Mirror,
	NUMBER_OF_ELEMENTS
};
extern std::map<int, std::string> enumDescriptionTextureAddressMode;


// itemss match ramses::ETextureSamplingMethod
enum class ETextureSamplingMethod {
	Nearest = 0,
	Linear,
	Nearest_MipMapNearest,
	Nearest_MipMapLinear,
	Linear_MipMapNearest,
	Linear_MipMapLinear,
	NUMBER_OF_ELEMENTS
};

extern std::map<int, std::string> enumDescriptionTextureMinSamplingMethod;

extern std::map<int, std::string> enumDescriptionTextureMagSamplingMethod;


// items match ramses::ETextureFormat
enum class ETextureFormat {
	//Invalid = 0,
	R8 = 1,
	RG8 = 2,
	RGB8 = 3,
	//RGB565 = 4,
	RGBA8 = 5,
	//RGBA4 = 6,
	//RGBA5551 = 7,
	//ETC2RGB = 8,
	//ETC2RGBA = 9,
	//R16F = 10,
	//R32F = 11,
	//RG16F = 12,
	//RG32F = 13,
	RGB16F = 14,
	//RGB32F = 15,
	RGBA16F = 16,
	//RGBA32F = 17,
	SRGB8 = 18,
	SRGB8_ALPHA8 = 19,
};
extern std::map<int, std::string> enumDescriptionTextureFormat;


// items match ramses::ERenderBufferFormat
enum class ERenderBufferFormat {
	RGBA4 = 0,
	R8,
	RG8,
	RGB8,
	RGBA8,
	R16F,
	R32F,
	RG16F,
	RG32F,
	RGB16F,
	RGB32F,
	RGBA16F,
	RGBA32F,

	Depth24,
	Depth24_Stencil8
};
extern std::map<int, std::string> enumDescriptionRenderBufferFormat;


enum class ERenderLayerOrder {
	Manual = 0,
	SceneGraph
};
extern std::map<int, std::string> enumDescriptionRenderLayerOrder;

enum class ERenderLayerMaterialFilterMode {
	Inclusive = 0,
	Exclusive
};

extern std::map<int, std::string> enumDescriptionRenderLayerMaterialFilterMode;

enum class EFrustumType {
	Aspect_FieldOfView = 0,
	Planes
};

extern std::map<int, std::string> enumDescriptionFrustumType;


}  // namespace raco::user_types