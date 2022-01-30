/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

class PreviewFramebufferScene final {
	Q_DISABLE_COPY(PreviewFramebufferScene);

public:
	explicit PreviewFramebufferScene(ramses::RamsesClient& client, ramses::sceneId_t sceneId);

	ramses::sceneId_t getSceneId() const;
	ramses::dataConsumerId_t setupFramebufferTexture(RendererBackend& backend, const QSize& size, PreviewFilteringMode filteringMode);
	void setViewport(const QPoint& viewportPosition, const QSize& viewportSize, const QSize& virtualSize);

private:
	// Scene
	raco::ramses_base::RamsesScene scene_;
	raco::ramses_base::RamsesOrthographicCamera camera_;
	std::shared_ptr<ramses::RenderGroup> renderGroup_;
	std::shared_ptr<ramses::RenderPass> renderPass_;
	raco::ramses_base::RamsesEffect effect_;
	raco::ramses_base::RamsesAppearance appearance_;
	raco::ramses_base::RamsesArrayResource indexDataBuffer_;
	raco::ramses_base::RamsesArrayResource vertexDataBuffer_;
	raco::ramses_base::RamsesArrayResource uvDataBuffer_;
	raco::ramses_base::RamsesGeometryBinding geometryBinding_;
	raco::ramses_base::RamsesMeshNode meshNode_;

	// Offscreen texture and consumer
	raco::ramses_base::RamsesTexture2D framebufferTexture_;
	raco::ramses_base::RamsesTextureSampler sampler_;
	ramses::dataConsumerId_t framebufferSampleId_;
};

}  // namespace raco::ramses_widgets
