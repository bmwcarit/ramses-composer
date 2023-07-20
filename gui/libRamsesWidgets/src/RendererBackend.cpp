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

ramses::RamsesFrameworkConfig& RendererBackend::ramsesFrameworkConfig(const std::string& frameworkArgs) noexcept {
	if (frameworkArgs.empty()) {
		char const* argv[] = {"RamsesComposer.exe",
			"--log-level-contexts-filter", "trace:RAPI,off:RPER,debug:RRND,off:RFRA,off:RDSM,info:RCOM",
			"--log-level-console", "warn",
			"--log-level-dlt", "warn",
			"--disablePeriodicLogs"};
		static ramses::RamsesFrameworkConfig config(sizeof(argv) / sizeof(argv[0]), argv);
		return config;
	} else {
		std::istringstream is(frameworkArgs);
		// Vector to store tokens
		std::vector<const char*> args{"RamsesComposer.exe"};
		const std::vector<std::string> tokens = std::vector<std::string>(std::istream_iterator<std::string>(is), std::istream_iterator<std::string>());
		for (const auto& token : tokens) args.push_back(token.c_str());
		static ramses::RamsesFrameworkConfig config(static_cast<int32_t>(args.size()), args.data());
		return config;
	}
}

RendererBackend::RendererBackend(rlogic::EFeatureLevel featureLevel, const std::string& frameworkArgs)
	: BaseEngineBackend{featureLevel, ramsesFrameworkConfig(frameworkArgs)},
	  renderer_{framework().createRenderer(ramses::RendererConfig{}), [=](ramses::RamsesRenderer* c) { framework().destroyRenderer(*c); }},
	  eventHandler_{std::make_unique<SceneStateEventHandler>(*renderer_.get())} {
	// Connect needs to be called after the renderer is created
	// Additonally there can only be one renderer per framework
	BaseEngineBackend::connect();
	renderer_->setSkippingOfUnmodifiedBuffers(false);
}

RendererBackend::~RendererBackend() {
	framework().disconnect();
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
}

}  // namespace raco::ramses_widgets
