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

#include <ramses-client-api/RamsesObjectTypes.h>
#include <ramses-framework-api/RamsesFrameworkTypes.h>
#include <ramses-framework-api/RendererSceneState.h>
#include <spdlog/fmt/fmt.h>

template <>
struct fmt::formatter<ramses::sceneId_t> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::sceneId_t id, FormatContext& ctx) {
		return formatter<string_view>::format(std::to_string(id.getValue()), ctx);
	}
};

template <>
struct fmt::formatter<ramses::displayId_t> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::displayId_t id, FormatContext& ctx) {
		return formatter<string_view>::format(std::to_string(id.getValue()), ctx);
	}
};

template <>
struct fmt::formatter<ramses::displayBufferId_t> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::displayBufferId_t id, FormatContext& ctx) {
		return formatter<string_view>::format(std::to_string(id.getValue()), ctx);
	}
};

template <>
struct fmt::formatter<ramses::dataConsumerId_t> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::dataConsumerId_t id, FormatContext& ctx) {
		return formatter<string_view>::format(std::to_string(id.getValue()), ctx);
	}
};

/*
template <>
struct fmt::formatter<ramses::ERendererEventResult> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::ERendererEventResult result, FormatContext& ctx) {
		switch (result) {
			case ramses::ERendererEventResult::ERendererEventResult_OK:
				return formatter<string_view>::format("OK", ctx);
			case ramses::ERendererEventResult::ERendererEventResult_INDIRECT:
				return formatter<string_view>::format("INDIRECT", ctx);
			case ramses::ERendererEventResult::ERendererEventResult_FAIL:
				return formatter<string_view>::format("FAIL", ctx);
			default:
				return formatter<string_view>::format(std::to_string(static_cast<int>(result)), ctx);
		}
	}
};
*/

template <>
struct fmt::formatter<ramses::resourceId_t> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::resourceId_t id, FormatContext& ctx) {
		return format_to(ctx.out(), "resourceId {{ low: {}, high: {} }}", id.lowPart, id.highPart);
	}
};

template <>
struct fmt::formatter<ramses::RendererSceneState> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::RendererSceneState state, FormatContext& ctx) {
		switch (state) {
			case ramses::RendererSceneState::Unavailable:
				return formatter<string_view>::format("Unavailable", ctx);
			case ramses::RendererSceneState::Available:
				return formatter<string_view>::format("Available", ctx);
			case ramses::RendererSceneState::Ready:
				return formatter<string_view>::format("Ready", ctx);
			case ramses::RendererSceneState::Rendered:
				return formatter<string_view>::format("Rendered", ctx);
			default:
				return formatter<string_view>::format(std::to_string(static_cast<int>(state)), ctx);
		}
	}
};

template <>
struct fmt::formatter<ramses::ERamsesObjectType> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const ramses::ERamsesObjectType type, FormatContext& ctx) {
		switch (type) {
			case ramses::ERamsesObjectType::ERamsesObjectType_Invalid:
				return format_to(ctx.out(), "Invalid");
			case ramses::ERamsesObjectType::ERamsesObjectType_ClientObject:
				return format_to(ctx.out(), "ClientObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_RamsesObject:
				return format_to(ctx.out(), "RamsesObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_SceneObject:
				return format_to(ctx.out(), "SceneObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_AnimationObject:
				return format_to(ctx.out(), "AnimationObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_Client:
				return format_to(ctx.out(), "Client");
			case ramses::ERamsesObjectType::ERamsesObjectType_Scene:
				return format_to(ctx.out(), "Scene");
			case ramses::ERamsesObjectType::ERamsesObjectType_AnimationSystem:
				return format_to(ctx.out(), "AnimationSystem");
			case ramses::ERamsesObjectType::ERamsesObjectType_AnimationSystemRealTime:
				return format_to(ctx.out(), "AnimationSystemRealTime");
			case ramses::ERamsesObjectType::ERamsesObjectType_Node:
				return format_to(ctx.out(), "Node");
			case ramses::ERamsesObjectType::ERamsesObjectType_MeshNode:
				return format_to(ctx.out(), "MeshNode");
			case ramses::ERamsesObjectType::ERamsesObjectType_Camera:
				return format_to(ctx.out(), "Camera");
			case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
				return format_to(ctx.out(), "PerspectiveCamera");
			case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
				return format_to(ctx.out(), "OrthographicCamera");
			case ramses::ERamsesObjectType::ERamsesObjectType_Effect:
				return format_to(ctx.out(), "Effect");
			case ramses::ERamsesObjectType::ERamsesObjectType_AnimatedProperty:
				return format_to(ctx.out(), "AnimatedProperty");
			case ramses::ERamsesObjectType::ERamsesObjectType_Animation:
				return format_to(ctx.out(), "Animation");
			case ramses::ERamsesObjectType::ERamsesObjectType_AnimationSequence:
				return format_to(ctx.out(), "AnimationSequence");
			case ramses::ERamsesObjectType::ERamsesObjectType_Appearance:
				return format_to(ctx.out(), "Appearance");
			case ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding:
				return format_to(ctx.out(), "GeometryBinding");
			case ramses::ERamsesObjectType::ERamsesObjectType_PickableObject:
				return format_to(ctx.out(), "PickableObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_Spline:
				return format_to(ctx.out(), "Spline");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepBool:
				return format_to(ctx.out(), "SplineStepBool");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepFloat:
				return format_to(ctx.out(), "SplineStepFloat");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepInt32:
				return format_to(ctx.out(), "SplineStepInt32");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector2f:
				return format_to(ctx.out(), "SplinteStepVector2f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector3f:
				return format_to(ctx.out(), "SplineStepVector3f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector4f:
				return format_to(ctx.out(), "SplineStepVector4f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector2i:
				return format_to(ctx.out(), "SplineStepVector2i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector3i:
				return format_to(ctx.out(), "SplineStepVector3i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineStepVector4i:
				return format_to(ctx.out(), "SplineStepVector4i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearFloat:
				return format_to(ctx.out(), "SplineLinearFloat");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearInt32:
				return format_to(ctx.out(), "SplineLinearInt32");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector2f:
				return format_to(ctx.out(), "SplineLinearVector2f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector3f:
				return format_to(ctx.out(), "SplineLinearVector3f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector4f:
				return format_to(ctx.out(), "SplineLinearVector4f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector2i:
				return format_to(ctx.out(), "SplineLinearVector2i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector3i:
				return format_to(ctx.out(), "SplineLinearVector3i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineLinearVector4i:
				return format_to(ctx.out(), "SplineLinearVector4i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierFloat:
				return format_to(ctx.out(), "SplineBezierFloat");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierInt32:
				return format_to(ctx.out(), "SplineBezierInt32");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector2f:
				return format_to(ctx.out(), "SplineBezierVector2f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector3f:
				return format_to(ctx.out(), "SplineBezierVector3f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector4f:
				return format_to(ctx.out(), "SplineBezierVector4f");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector2i:
				return format_to(ctx.out(), "SplineBezierVector2i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector3i:
				return format_to(ctx.out(), "SplineBezierVector3i");
			case ramses::ERamsesObjectType::ERamsesObjectType_SplineBezierVector4i:
				return format_to(ctx.out(), "SplineBezierVector4i");
			case ramses::ERamsesObjectType::ERamsesObjectType_Resource:
				return format_to(ctx.out(), "Resource");
			case ramses::ERamsesObjectType::ERamsesObjectType_Texture2D:
				return format_to(ctx.out(), "Texture2D");
			case ramses::ERamsesObjectType::ERamsesObjectType_Texture3D:
				return format_to(ctx.out(), "Texture3D");
			case ramses::ERamsesObjectType::ERamsesObjectType_TextureCube:
				return format_to(ctx.out(), "TextureCube");
			case ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource:
				return format_to(ctx.out(), "ArrayResource");
			case ramses::ERamsesObjectType::ERamsesObjectType_RenderGroup:
				return format_to(ctx.out(), "RenderGroup");
			case ramses::ERamsesObjectType::ERamsesObjectType_RenderPass:
				return format_to(ctx.out(), "RenderPass");
			case ramses::ERamsesObjectType::ERamsesObjectType_BlitPass:
				return format_to(ctx.out(), "BlitPass");
			case ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler:
				return format_to(ctx.out(), "TextureSampler");
			case ramses::ERamsesObjectType::ERamsesObjectType_TextureSamplerMS:
				return format_to(ctx.out(), "TextureSamplerMS");
			case ramses::ERamsesObjectType::ERamsesObjectType_RenderBuffer:
				return format_to(ctx.out(), "RenderBuffer");
			case ramses::ERamsesObjectType::ERamsesObjectType_RenderTarget:
				return format_to(ctx.out(), "RenderTarget");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataBufferObject:
				return format_to(ctx.out(), "DataBufferObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_Texture2DBuffer:
				return format_to(ctx.out(), "Texture2DBuffer");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataObject:
				return format_to(ctx.out(), "DataObject");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataFloat:
				return format_to(ctx.out(), "DataFloat");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector2f:
				return format_to(ctx.out(), "DataVector2f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector3f:
				return format_to(ctx.out(), "DataVector3f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector4f:
				return format_to(ctx.out(), "DataVector4f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataMatrix22f:
				return format_to(ctx.out(), "DataMatrix22f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataMatrix33f:
				return format_to(ctx.out(), "DataMatrix33f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataMatrix44f:
				return format_to(ctx.out(), "DataMatrix44f");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataInt32:
				return format_to(ctx.out(), "DataInt32");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector2i:
				return format_to(ctx.out(), "DataVector2i");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector3i:
				return format_to(ctx.out(), "DataVector3i");
			case ramses::ERamsesObjectType::ERamsesObjectType_DataVector4i:
				return format_to(ctx.out(), "DataVector4i");
			case ramses::ERamsesObjectType::ERamsesObjectType_StreamTexture:
				return format_to(ctx.out(), "StreamTexture");
			case ramses::ERamsesObjectType::ERamsesObjectType_SceneReference:
				return format_to(ctx.out(), "SceneReference");
			case ramses::ERamsesObjectType::ERamsesObjectType_TextureSamplerExternal:
				return format_to(ctx.out(), "TextureSamplerExternal");
			default:
				return format_to(ctx.out(), "Unknown");
		}
	}
};


