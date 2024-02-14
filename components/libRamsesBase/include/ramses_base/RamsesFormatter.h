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

#include <ramses/framework/RamsesObjectTypes.h>
#include <ramses/framework/RamsesFrameworkTypes.h>
#include <ramses/framework/RendererSceneState.h>
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
			case ramses::ERamsesObjectType::Invalid:
				return format_to(ctx.out(), "Invalid");
			case ramses::ERamsesObjectType::ClientObject:
				return format_to(ctx.out(), "ClientObject");
			case ramses::ERamsesObjectType::RamsesObject:
				return format_to(ctx.out(), "RamsesObject");
			case ramses::ERamsesObjectType::SceneObject:
				return format_to(ctx.out(), "SceneObject");
			case ramses::ERamsesObjectType::Client:
				return format_to(ctx.out(), "Client");
			case ramses::ERamsesObjectType::Scene:
				return format_to(ctx.out(), "Scene");
			case ramses::ERamsesObjectType::LogicEngine:
				return format_to(ctx.out(), "LogicEngine");
			case ramses::ERamsesObjectType::LogicObject:
				return format_to(ctx.out(), "LogicObject");
			case ramses::ERamsesObjectType::Node:
				return format_to(ctx.out(), "Node");
			case ramses::ERamsesObjectType::MeshNode:
				return format_to(ctx.out(), "MeshNode");
			case ramses::ERamsesObjectType::Camera:
				return format_to(ctx.out(), "Camera");
			case ramses::ERamsesObjectType::PerspectiveCamera:
				return format_to(ctx.out(), "PerspectiveCamera");
			case ramses::ERamsesObjectType::OrthographicCamera:
				return format_to(ctx.out(), "OrthographicCamera");
			case ramses::ERamsesObjectType::Effect:
				return format_to(ctx.out(), "Effect");
			case ramses::ERamsesObjectType::Appearance:
				return format_to(ctx.out(), "Appearance");
			case ramses::ERamsesObjectType::Geometry:
				return format_to(ctx.out(), "Geometry");
			case ramses::ERamsesObjectType::PickableObject:
				return format_to(ctx.out(), "PickableObject");
			case ramses::ERamsesObjectType::Resource:
				return format_to(ctx.out(), "Resource");
			case ramses::ERamsesObjectType::Texture2D:
				return format_to(ctx.out(), "Texture2D");
			case ramses::ERamsesObjectType::Texture3D:
				return format_to(ctx.out(), "Texture3D");
			case ramses::ERamsesObjectType::TextureCube:
				return format_to(ctx.out(), "TextureCube");
			case ramses::ERamsesObjectType::ArrayResource:
				return format_to(ctx.out(), "ArrayResource");
			case ramses::ERamsesObjectType::RenderGroup:
				return format_to(ctx.out(), "RenderGroup");
			case ramses::ERamsesObjectType::RenderPass:
				return format_to(ctx.out(), "RenderPass");
			case ramses::ERamsesObjectType::BlitPass:
				return format_to(ctx.out(), "BlitPass");
			case ramses::ERamsesObjectType::TextureSampler:
				return format_to(ctx.out(), "TextureSampler");
			case ramses::ERamsesObjectType::TextureSamplerMS:
				return format_to(ctx.out(), "TextureSamplerMS");
			case ramses::ERamsesObjectType::RenderBuffer:
				return format_to(ctx.out(), "RenderBuffer");
			case ramses::ERamsesObjectType::RenderTarget:
				return format_to(ctx.out(), "RenderTarget");
			case ramses::ERamsesObjectType::ArrayBuffer:
				return format_to(ctx.out(), "ArrayBuffer");
			case ramses::ERamsesObjectType::Texture2DBuffer:
				return format_to(ctx.out(), "Texture2DBuffer");
			case ramses::ERamsesObjectType::DataObject:
				return format_to(ctx.out(), "DataObject");
			case ramses::ERamsesObjectType::SceneReference:
				return format_to(ctx.out(), "SceneReference");
			case ramses::ERamsesObjectType::TextureSamplerExternal:
				return format_to(ctx.out(), "TextureSamplerExternal");
			default:
				return format_to(ctx.out(), "Unknown");
		}
	}
};


