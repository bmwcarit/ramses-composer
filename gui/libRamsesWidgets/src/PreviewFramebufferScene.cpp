/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_widgets/PreviewFramebufferScene.h"

#include "ramses_widgets/SceneStateEventHandler.h"

namespace raco::ramses_widgets {

using namespace raco::ramses_base;

PreviewFramebufferScene::PreviewFramebufferScene(
	ramses::RamsesClient& client,
	ramses::sceneId_t sceneId)
	: scene_{ramsesScene(sceneId, &client)},
	  camera_{ramsesOrthographicCamera(scene_.get(), {0, 0})},
	  renderGroup_{std::shared_ptr<ramses::RenderGroup>(scene_.get()->createRenderGroup(), ramses_base::createRamsesObjectDeleter<ramses::RenderGroup>(scene_.get()))},
	  renderPass_{scene_->createRenderPass(), ramses_base::createRamsesObjectDeleter<ramses::RenderPass>(scene_.get())} {
	(*camera_)->setTranslation({0, 0, 10.0f});
	(*camera_)->setFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	// we need to have an inital viewport
	(*camera_)->setViewport(0, 0, 1, 1);

	renderPass_->setCamera(**camera_);
	renderPass_->addRenderGroup(*renderGroup_.get());
	// Ramses ignores the clear flags/clear color because we are rendering to the default framebuffer.
	// Still: apply some sensible default here. We expect every pixel to be covered by the blit operation
	// so disable clearing the buffers.
	renderPass_->setClearFlags(ramses::EClearFlag::None);

	static const std::string vertexShader =
		"#version 310 es\n\
		precision mediump float;\n\
		uniform mat4 mvpMatrix;\n\
		\n\
		in vec3 a_Position;\n\
		in vec2 a_TextureCoordinate;\n\
		out vec2 vTC0;\n\
\n\
		void main() {\n\
			gl_Position = mvpMatrix * vec4(a_Position, 1.0);\n\
			vTC0 = a_TextureCoordinate;\n\
		}";

	static const std::string fragmentShader =
		"#version 310 es\n\
		precision mediump float;\n\
		\n\
		in vec2 vTC0;\n\
		uniform mediump sampler2D uTex0;\n\
		\n\
		out vec4 FragColor;\n\
		\n\
		void main() {\n\
			vec3 clr0 = texture(uTex0, vTC0).rgb;\n\
			FragColor = vec4(clr0, 1.0); \n\
		}";

	static const std::string fragmentShaderMS =
		"#version 310 es\n\
		precision mediump float;\n\
		\n\
		in vec2 vTC0;\n\
		uniform mediump sampler2DMS uTex0;\n\
		uniform int sampleCount;\n\
		\n\
		out vec4 FragColor;\n\
		\n\
		void main() {\n\
			vec4 color = vec4(0.0);\n\
\n\
			for (int i = 0; i < sampleCount; i++)\n\
				color += texelFetch(uTex0, ivec2(vTC0 * vec2(textureSize(uTex0))), i);\n\
\n\
			color /= float(sampleCount);\n\
			FragColor = color;\n\
		}";

	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(vertexShader.c_str());
	effectDescription.setFragmentShader(fragmentShader.c_str());
	effectDescription.setUniformSemantic("mvpMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);
	effect_ = ramsesEffect(scene_.get(), effectDescription, {}, {0, 0});
	appearance_ = ramsesAppearance(scene_.get(), effect_, {0, 0});

	ramses::EffectDescription effectDescriptionMS{};
	effectDescriptionMS.setVertexShader(vertexShader.c_str());
	effectDescriptionMS.setFragmentShader(fragmentShaderMS.c_str());
	effectDescriptionMS.setUniformSemantic("mvpMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);
	effectMS_ = ramsesEffect(scene_.get(), effectDescriptionMS, {}, {0, 0});
	appearanceMS_ = ramsesAppearance(scene_.get(), effectMS_, {0, 0});

	// buffers
	static std::vector<glm::vec3> vertex_data = {
		{0.0f, 0.0f, 0.f},
		{0.0f, 1.0f, 0.f},
		{1.0f, 0.0f, 0.f},
		{1.0f, 1.0f, 0.f}};
	static std::vector<glm::vec2> uv_data = {
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f}};
	static std::vector<uint32_t> index_data = {
		0, 2, 1,
		1, 2, 3};

	indexDataBuffer_ = ramsesArrayResource(scene_.get(), index_data);
	vertexDataBuffer_ = ramsesArrayResource(scene_.get(), vertex_data);
	uvDataBuffer_ = ramsesArrayResource(scene_.get(), uv_data);

	{
		geometry_ = ramsesGeometry(scene_.get(), effect_, {0, 0});
		(*geometry_)->setIndices(*indexDataBuffer_.get());

		ramses::AttributeInput vertexInput = effect_->findAttributeInput("a_Position").value();
		(*geometry_)->setInputBuffer(vertexInput, *vertexDataBuffer_.get());

		ramses::AttributeInput uvInput = effect_->findAttributeInput("a_TextureCoordinate").value();
		(*geometry_)->setInputBuffer(uvInput, *uvDataBuffer_.get());
	}

	{
		geometryMS_ = ramsesGeometry(scene_.get(), effectMS_, {0, 0});
		(*geometryMS_)->setIndices(*indexDataBuffer_.get());

		ramses::AttributeInput vertexInput = effectMS_->findAttributeInput("a_Position").value();
		(*geometryMS_)->setInputBuffer(vertexInput, *vertexDataBuffer_.get());

		ramses::AttributeInput uvInput = effectMS_->findAttributeInput("a_TextureCoordinate").value();
		(*geometryMS_)->setInputBuffer(uvInput, *uvDataBuffer_.get());
	}

	meshNode_ = ramsesMeshNode(scene_.get(), {0, 0});
	meshNode_->setGeometry(geometry_);
	meshNode_->setAppearance(appearance_);

	renderGroup_->addMeshNode(**meshNode_);

	scene_->flush();
	scene_->publish(ramses::EScenePublicationMode::LocalAndRemote);
}

void PreviewFramebufferScene::setViewport(const QPoint& viewportPosition, const QSize& viewportSize, const QSize& virtualSize) {
	// viewport
	const float left = static_cast<float>(viewportPosition.x()) / virtualSize.width();
	const float right = static_cast<float>(viewportPosition.x() + viewportSize.width()) / virtualSize.width();
	const float bottom = 1.0 - static_cast<float>(viewportPosition.y() + viewportSize.height()) / virtualSize.height();
	const float top = 1.0 - static_cast<float>(viewportPosition.y()) / virtualSize.height();
	if (
		left != (*camera_)->getLeftPlane() || right != (*camera_)->getRightPlane() || bottom != (*camera_)->getBottomPlane() || top != (*camera_)->getTopPlane() || (*camera_)->getViewportWidth() != viewportSize.width() || (*camera_)->getViewportHeight() != viewportSize.height()) {
		(*camera_)->setFrustum(left, right, bottom, top, 0.1f, 100.0f);
		(*camera_)->setViewport(0, 0, viewportSize.width(), viewportSize.height());
		scene_->flush();
	}
}

ramses::sceneId_t PreviewFramebufferScene::getSceneId() const {
	return scene_->getSceneId();
}

ramses::dataConsumerId_t PreviewFramebufferScene::setupFramebufferTexture(RendererBackend& backend, const QSize& size, PreviewFilteringMode filteringMode, PreviewMultiSampleRate sampleRate) {
	ramses::ETextureSamplingMethod samplingMethod = ramses::ETextureSamplingMethod::Nearest;
	if (filteringMode == PreviewFilteringMode::Linear) {
		samplingMethod = ramses::ETextureSamplingMethod::Linear;
	}

	if (currentSampleCount_ == sampleRate) {
		if (currentSampleCount_ > 0) {
			if (renderbufferMS_ && renderbufferMS_->getWidth() == size.width() && renderbufferMS_->getHeight() == size.height() && currentSampleCount_ == sampleRate) {
				return framebufferSampleId_;
			}
		} else {
			if (framebufferTexture_ && framebufferTexture_->getWidth() == size.width() && framebufferTexture_->getHeight() == size.height() && sampler_ && sampler_->getMagSamplingMethod() == samplingMethod && sampler_->getMinSamplingMethod() == samplingMethod) {
				return framebufferSampleId_;
			}
		}
	}
	auto& client = backend.client();

	if (sampleRate > 0) {
		framebufferTexture_.reset();
		sampler_.reset();

		renderbufferMS_ = ramsesRenderBuffer(scene_.get(), size.width(), size.height(), ramses::ERenderBufferFormat::RGBA8, ramses::ERenderBufferAccessMode::ReadWrite, sampleRate, {}, {0, 0});

		samplerMS_ = ramsesTextureSamplerMS(scene_.get(), renderbufferMS_, {}, {0, 0});

		ramses::UniformInput texUniformInput = (*appearanceMS_)->getEffect().findUniformInput("uTex0").value();
		(*appearanceMS_)->setInputTexture(texUniformInput, *samplerMS_.get());

		ramses::UniformInput sampleRateUniformInput = (*appearanceMS_)->getEffect().findUniformInput("sampleCount").value();
		(*appearanceMS_)->setInputValue<int32_t>(sampleRateUniformInput, static_cast<int32_t>(sampleRate));

		meshNode_->removeAppearanceAndGeometry();
		meshNode_->setGeometry(geometryMS_);
		meshNode_->setAppearance(appearanceMS_);
	} else {
		renderbufferMS_.reset();
		samplerMS_.reset();

		std::vector<uint8_t> data(4 * size.width() * size.height(), 0);

		std::vector<ramses::MipLevelData> mipDatas;
		mipDatas.emplace_back(reinterpret_cast<std::byte*>(data.data()), reinterpret_cast<std::byte*>(data.data()) + data.size());
		const ramses::TextureSwizzle textureSwizzle{};

		framebufferTexture_ = ramsesTexture2D(scene_.get(), ramses::ETextureFormat::RGBA8, size.width(), size.height(), mipDatas, false, textureSwizzle, "framebuffer texture", {0, 0});

		sampler_ = ramsesTextureSampler(scene_.get(), ramses::ETextureAddressMode::Clamp, ramses::ETextureAddressMode::Clamp, samplingMethod, samplingMethod, framebufferTexture_.get(), 1, "framebuffer sampler", {0, 0});

		ramses::UniformInput texUniformInput = (*appearance_)->getEffect().findUniformInput("uTex0").value();
		(*appearance_)->setInputTexture(texUniformInput, *sampler_.get());

		meshNode_->removeAppearanceAndGeometry();
		meshNode_->setGeometry(geometry_);
		meshNode_->setAppearance(appearance_);
	}
	currentSampleCount_ = sampleRate;
	scene_->flush();

	framebufferSampleId_ = backend.internalDataConsumerId();
	if (sampleRate > 0) {
		scene_->createTextureConsumer(*samplerMS_.get(), framebufferSampleId_);
	} else {
		scene_->createTextureConsumer(*sampler_.get(), framebufferSampleId_);
	}

	static const ramses::sceneVersionTag_t SCENE_VERSION_TAG_DATA_CONSUMER_CREATED{42};
	static const ramses::sceneVersionTag_t SCENE_VERSION_TAG_RESET{41};

	auto& eventHandler = backend.eventHandler();
	// Toggle version tag for scene so that we are sure that data consumer is create
	scene_->flush(SCENE_VERSION_TAG_DATA_CONSUMER_CREATED);
	eventHandler.waitForFlush(scene_->getSceneId(), SCENE_VERSION_TAG_DATA_CONSUMER_CREATED);
	scene_->flush(SCENE_VERSION_TAG_RESET);
	eventHandler.waitForFlush(scene_->getSceneId(), SCENE_VERSION_TAG_RESET);

	return framebufferSampleId_;
}

}  // namespace raco::ramses_widgets
