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
	: scene_{ ramsesScene(sceneId, &client)}, 
	camera_{ ramsesOrthographicCamera(scene_.get()) }, 
	renderGroup_{std::shared_ptr<ramses::RenderGroup>(scene_.get()->createRenderGroup(), raco::ramses_base::createRamsesObjectDeleter<ramses::RenderGroup>(scene_.get()))},
	  renderPass_{scene_->createRenderPass(), raco::ramses_base::createRamsesObjectDeleter<ramses::RenderPass>(scene_.get())} {
	(*camera_)->setTranslation(0, 0, 10.0f);
	(*camera_)->setFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	// we need to have an inital viewport
	(*camera_)->setViewport(0, 0, 1, 1);

	renderPass_->setCamera(**camera_);
	renderPass_->addRenderGroup(*renderGroup_.get());
	// Ramses ignores the clear flags/clear color because we are rendering to the default framebuffer.
	// Still: apply some sensible default here. We expect every pixel to be covered by the blit operation
	// so disable clearing the buffers.
	renderPass_->setClearFlags(ramses::EClearFlags_None);

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
	effect_ = ramsesEffect(scene_.get(), effectDescription);
	appearance_ = ramsesAppearance(scene_.get(), effect_);

	// buffers
	static std::vector<float> vertex_data = {
		0.0f, 0.0f, 0.f,
		0.0f, 1.0f, 0.f,
		1.0f, 0.0f, 0.f,
		1.0f, 1.0f, 0.f};
	static std::vector<float> uv_data = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f};
	static std::vector<uint32_t> index_data = {
		0, 2, 1,
		1, 2, 3};

	const uint32_t indexSize = static_cast<uint32_t>(sizeof(uint32_t) * index_data.size());
	indexDataBuffer_ = ramsesArrayResource(scene_.get(), ramses::EDataType::UInt32, indexSize, index_data.data());

	const uint32_t vertexSize = static_cast<uint32_t>(sizeof(float) * vertex_data.size());
	vertexDataBuffer_ = ramsesArrayResource(scene_.get(), ramses::EDataType::Vector3F, vertexSize, vertex_data.data());

	const uint32_t uvSize = static_cast<uint32_t>(sizeof(float) * uv_data.size());
	uvDataBuffer_ = ramsesArrayResource(scene_.get(), ramses::EDataType::Vector2F, uvSize, uv_data.data());

	geometryBinding_ = ramsesGeometryBinding(scene_.get(), effect_);
	(*geometryBinding_)->setIndices(*indexDataBuffer_.get());

	ramses::AttributeInput vertexInput;
	effect_->findAttributeInput("a_Position", vertexInput);
	(*geometryBinding_)->setInputBuffer(vertexInput, *vertexDataBuffer_.get());

	ramses::AttributeInput uvInput;
	effect_->findAttributeInput("a_TextureCoordinate", uvInput);
	(*geometryBinding_)->setInputBuffer(uvInput, *uvDataBuffer_.get());

	meshNode_ =  ramsesMeshNode(scene_.get());
	meshNode_->setGeometryBinding(geometryBinding_);
	meshNode_->setAppearance(appearance_);

	renderGroup_->addMeshNode(**meshNode_);

	scene_->flush();
	scene_->publish();
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

ramses::dataConsumerId_t PreviewFramebufferScene::setupFramebufferTexture(RendererBackend& backend, const QSize& size, PreviewMultiSampleRate sampleRate) {
	if (renderbuffer_ && renderbuffer_->getWidth() == size.width() && renderbuffer_->getHeight() == size.height() && renderbuffer_->getSampleCount() == sampleRate) {
		return framebufferSampleId_;
	}
	auto& client = backend.client();
	ramses::UniformInput texUniformInput;
	(*appearance_)->getEffect().findUniformInput("uTex0", texUniformInput);

	ramses::UniformInput sampleRateUniformInput;
	(*appearance_)->getEffect().findUniformInput("sampleCount", sampleRateUniformInput);

	std::vector<uint8_t> data(4 * size.width() * size.height(), 0);

	ramses::MipLevelData mipData(static_cast<uint32_t>(data.size()), data.data());
	const ramses::TextureSwizzle textureSwizzle{};

	renderbuffer_ = ramsesRenderBuffer(scene_.get(), size.width(), size.height(), ramses::ERenderBufferType_Color, ramses::ERenderBufferFormat_RGBA8, ramses::ERenderBufferAccessMode_ReadWrite, sampleRate);

	ramses::RenderTargetDescription rtDesc;
	rtDesc.addRenderBuffer(*renderbuffer_);

	samplerMS_ = ramsesTextureSamplerMS(scene_.get(), renderbuffer_);
	(*appearance_)->setInputTexture(texUniformInput, *samplerMS_.get());
	(*appearance_)->setInputValueInt32(sampleRateUniformInput, sampleRate);
	scene_->flush();

	static ramses::dataConsumerId_t id{42u};

	framebufferSampleId_ = backend.internalDataConsumerId();
	scene_->createTextureConsumer(*samplerMS_.get(), framebufferSampleId_);

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
