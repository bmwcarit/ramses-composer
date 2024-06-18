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
#include "gtest/gtest.h"

#include "application/RaCoApplication.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"

#include "user_types/BlitPass.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Material.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"

#pragma once

using namespace raco::user_types;

class RaCoApplicationTest : public RacoBaseTest<> {
public:
	ramses_base::HeadlessEngineBackend backend{};
	application::RaCoApplication application{backend, {{}, false, false, -1, -1, false}};

	core::ExternalProjectsStoreInterface* externalProjectStore() {
		return application.externalProjects();
	};

	core::CommandInterface& commandInterface() {
		return *application.activeRaCoProject().commandInterface();
	};

	core::DataChangeRecorder& recorder() {
		return *application.activeRaCoProject().recorder();
	};

	core::Project& project() {
		return *application.activeRaCoProject().project();
	};

	template <class C>
	std::shared_ptr<C> create(std::string name, core::SEditorObject parent = nullptr, const std::vector<std::string>& tags = {}) {
		auto obj = std::dynamic_pointer_cast<C>(commandInterface().createObject(C::typeDescription.typeName, name));
		if (parent) {
			commandInterface().moveScenegraphChildren({obj}, parent);
		}
		if (!tags.empty()) {
			commandInterface().setTags({obj, {"tags"}}, tags);
		}
		return obj;
	}

	user_types::SMesh create_mesh(const std::string& name, const std::string& relpath) {
		auto mesh = create<user_types::Mesh>(name);
		commandInterface().set({mesh, {"uri"}}, (RacoBaseTest<>::test_path() / relpath).string());
		return mesh;
	}

	user_types::SMaterial create_material(const std::string& name, const std::string& relpathVertex, const std::string& relpathFragment, const std::string& relpathGeometry = std::string(), const std::vector<std::string>& tags = {}) {
		auto material = create<user_types::Material>(name, {}, tags);
		if (relpathGeometry.length()) {
			commandInterface().set({material, {"uriGeometry"}}, (RacoBaseTest<>::test_path() / relpathGeometry).string());
		}
		commandInterface().set({material, {"uriVertex"}}, (RacoBaseTest<>::test_path() / relpathVertex).string());
		commandInterface().set({material, {"uriFragment"}}, (RacoBaseTest<>::test_path() / relpathFragment).string());
		return material;
	}

	user_types::SMeshNode create_meshnode(const std::string& name, user_types::SMesh mesh, user_types::SMaterial material, user_types::SEditorObject parent = nullptr, const std::vector<std::string>& tags = {}) {
		auto meshnode = create<user_types::MeshNode>(name, parent, tags);
		commandInterface().set({meshnode, {"mesh"}}, mesh);
		commandInterface().set({meshnode, {"materials", "material", "material"}}, material);
		return meshnode;
	}

	user_types::SRenderLayer create_layer(const std::string& name,
		const std::vector<std::string>& tags = {},
		const std::vector<std::pair<std::string, int>>& renderables = {},
		const std::vector<std::string>& matFilterTags = {},
		bool filterExclusive = true) {
		auto layer = create<user_types::RenderLayer>(name, nullptr, tags);
		commandInterface().setRenderableTags({layer, &RenderLayer::renderableTags_}, renderables);
		if (!matFilterTags.empty()) {
			commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, matFilterTags);
		}
		commandInterface().set({layer, &user_types::RenderLayer::materialFilterMode_},
			static_cast<int>(filterExclusive ? user_types::ERenderLayerMaterialFilterMode::Exclusive : user_types::ERenderLayerMaterialFilterMode::Inclusive));

		return layer;
	}

	user_types::SRenderPass create_renderpass(const std::string& name,
		SRenderTarget target,
		const std::vector<SRenderLayer>& layers,
		int renderOrder = 0) {
		auto renderpass = create<RenderPass>(name);
		commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
		commandInterface().resizeArray({renderpass, &RenderPass::layers_}, layers.size());
		for (int index = 0; index < layers.size(); index++) {
			commandInterface().set(ValueHandle(renderpass, &RenderPass::layers_)[index], layers[index]);
		}
		commandInterface().set({renderpass, &RenderPass::renderOrder_}, renderOrder);
		return renderpass;
	}

	user_types::SRenderTarget create_rendertarget(const std::string& name, const std::vector<SRenderBuffer> & buffers) {
		auto rendertarget = create<RenderTarget>(name);
		commandInterface().resizeArray({rendertarget, &RenderTarget::buffers_}, buffers.size());
		for (int index = 0; index < buffers.size(); index++) {
			commandInterface().set(ValueHandle(rendertarget, &RenderTarget::buffers_)[index], buffers[index]);
		}
		return rendertarget;
	}


	user_types::SBlitPass create_blitpass(const std::string& name, int renderOrder, SRenderBuffer srcBuffer, SRenderBuffer targetBuffer) {
		auto renderpass = create<BlitPass>(name);
		commandInterface().set({renderpass, &BlitPass::renderOrder_}, renderOrder);
		commandInterface().set({renderpass, &BlitPass::sourceRenderBuffer_}, srcBuffer);
		commandInterface().set({renderpass, &BlitPass::targetRenderBuffer_}, targetBuffer);
		return renderpass;
	}
};
