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

#include "ramses_widgets/BuildOptions.h"
#include "ramses_base/BaseEngineBackend.h"
#include <memory>

namespace ramses {
	class RamsesRenderer;
}

namespace raco::ramses_widgets {

class SceneStateEventHandler;

class RendererBackend final : public raco::ramses_base::BaseEngineBackend {
	Q_DISABLE_COPY(RendererBackend)
public:
	typedef std::unique_ptr<ramses::RamsesRenderer, std::function<void(ramses::RamsesRenderer*)>> UniqueRenderer;
	static ramses::RamsesFrameworkConfig& ramsesFrameworkConfig(const std::string& frameworkArgs) noexcept;
	explicit RendererBackend(const std::string& frameworkArgs = {});
	~RendererBackend();

	ramses::RamsesRenderer& renderer() const;
	SceneStateEventHandler& eventHandler();

	/**
	 * Naive scene id factory for the creation of internal scenes needed by ramses composer (e.g. offscreen rendering)
	 */
	ramses::sceneId_t internalSceneId();
	ramses::dataConsumerId_t internalDataConsumerId();
	void doOneLoop() const;

private:
	UniqueRenderer renderer_;
	std::unique_ptr<SceneStateEventHandler> eventHandler_;	// we don't want to include SceneStateEventHandler (with its explicit Ramses renderer headers, hence the pointer)
	ramses::sceneId_t internalSceneId_{BuildOptions::ramsesComposerSceneIdStart};
	ramses::dataConsumerId_t dataConsumerId_{42u};
};

}  // namespace raco::ramses_widgets