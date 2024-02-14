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

#include "ramses_base/RamsesHandles.h"
#include "ramses_widgets/RendererBackend.h"
#include <QtGlobal>

namespace raco::ramses_widgets {

enum class PreviewFilteringMode {
	NearestNeighbor,
	Linear
};

enum PreviewMultiSampleRate {
	MSAA_0X = 0,	 // no multi-sampling
	MSAA_1X = 1,
	MSAA_2X = 2,
	MSAA_4X = 4,
	MSAA_8X = 8
};

class PreviewFramebufferScene final {
	Q_DISABLE_COPY(PreviewFramebufferScene);

public:
	explicit PreviewFramebufferScene(ramses::RamsesClient& client, ramses::sceneId_t sceneId);

	ramses::sceneId_t getSceneId() const;
	ramses::dataConsumerId_t setupFramebufferTexture(RendererBackend& backend, const QSize& size, PreviewFilteringMode filteringMode, PreviewMultiSampleRate sampleRate);
	void setViewport(const QPoint& viewportPosition, const QSize& viewportSize, const QSize& virtualSize);

private:
	// Scene
	ramses_base::RamsesScene scene_;
	ramses_base::RamsesOrthographicCamera camera_;
	std::shared_ptr<ramses::RenderGroup> renderGroup_;
	std::shared_ptr<ramses::RenderPass> renderPass_;
	ramses_base::RamsesEffect effect_;
	ramses_base::RamsesEffect effectMS_;
	ramses_base::RamsesAppearance appearance_;
	ramses_base::RamsesAppearance appearanceMS_;
	ramses_base::RamsesArrayResource indexDataBuffer_;
	ramses_base::RamsesArrayResource vertexDataBuffer_;
	ramses_base::RamsesArrayResource uvDataBuffer_;
	ramses_base::RamsesGeometry geometry_;
	ramses_base::RamsesGeometry geometryMS_;
	ramses_base::RamsesMeshNode meshNode_;

	// Offscreen texture and consumer
	ramses_base::RamsesTexture2D framebufferTexture_;
	ramses_base::RamsesTextureSampler sampler_;
	ramses_base::RamsesRenderBuffer renderbufferMS_;
	ramses_base::RamsesTextureSamplerMS samplerMS_;
	ramses::dataConsumerId_t framebufferSampleId_;

	int currentSampleCount_ = 0;
};

}  // namespace raco::ramses_widgets
