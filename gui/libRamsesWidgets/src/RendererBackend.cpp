/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/RendererBackend.h"

#include "ramses_widgets/SceneStateEventHandler.h"

#include <sstream>

namespace raco::ramses_widgets {

RendererBackend::RendererBackend(const ramses::RamsesFrameworkConfig& frameworkConfig)
	: BaseEngineBackend{frameworkConfig} {
}

RendererBackend::~RendererBackend() {
}

void RendererBackend::reset() {
	renderer_.reset();
	eventHandler_.reset();
	BaseEngineBackend::reset();
}

void RendererBackend::setup(ramses::EFeatureLevel featureLevel) {
	BaseEngineBackend::setup(featureLevel);
	renderer_ = UniqueRenderer(framework().createRenderer(ramses::RendererConfig{}), [=](ramses::RamsesRenderer* c) { framework().destroyRenderer(*c); });
	eventHandler_ = std::make_unique<SceneStateEventHandler>(*renderer_.get());

	renderer_->setSkippingOfUnmodifiedBuffers(false);
}

ramses::RamsesRenderer& RendererBackend::renderer() const {
	return *renderer_.get();
}

SceneStateEventHandler& RendererBackend::eventHandler() {
	return *eventHandler_;
}

ramses::sceneId_t RendererBackend::internalSceneId() {
	internalSceneId_ = ramses::sceneId_t{internalSceneId_.getValue() + 1};
	if (internalSceneId_.getValue() > BuildOptions::ramsesComposerSceneIdEnd) {
		internalSceneId_ = static_cast<ramses::sceneId_t>(BuildOptions::ramsesComposerSceneIdStart);
	}
	return internalSceneId_;
}

ramses::dataConsumerId_t RendererBackend::internalDataConsumerId() {
	dataConsumerId_ = ramses::dataConsumerId_t{dataConsumerId_.getValue() + 1};
	return dataConsumerId_;
}

void RendererBackend::doOneLoop() const {
	renderer().doOneLoop();
	renderer().dispatchEvents(*eventHandler_);
	renderer().getSceneControlAPI()->dispatchEvents(*eventHandler_);
}

}  // namespace raco::ramses_widgets
