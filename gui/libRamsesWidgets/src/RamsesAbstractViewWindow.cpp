/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/RamsesAbstractViewWindow.h"

#include "ramses_widgets/SceneStateEventHandler.h"

#include <ramses/renderer/DisplayConfig.h>
#include <ramses/renderer/RamsesRenderer.h>

#include "style/Colors.h"

namespace {

using namespace raco::ramses_widgets;

void setAndWaitSceneState(
	RendererBackend& backend,
	const ramses::RendererSceneState state,
	const ramses::sceneId_t sceneId) {
	auto& sceneControlAPI = *backend.renderer().getSceneControlAPI();

	if (sceneId.isValid()) {
		auto status = sceneControlAPI.setSceneState(sceneId, state);
		if (sceneControlAPI.flush()) {
			backend.eventHandler().waitForSceneState(sceneId, state);
		}
	}
}

/**
 * Sets and await's current scene state for frambuffer and conditionally for scene. The scene state is only set if the current scene state is greater than the request scene state.
 */
void reduceAndWaitSceneState(
	RendererBackend& backend,
	const ramses::RendererSceneState state,
	const ramses::sceneId_t sceneId) {
	auto& eventHandler = backend.eventHandler();
	auto& sceneControlAPI = *backend.renderer().getSceneControlAPI();

	bool scene_success = false;

	if (sceneId.isValid() && eventHandler.sceneState(sceneId) > state) {
		auto status = sceneControlAPI.setSceneState(sceneId, state);
		if (sceneControlAPI.flush()) {
			// Wait for a state that is <= the desired state:
			// Apparently the renderer may already be in a lower state but the event handler hasn't caught up with the state yet which
			// leads to the scene state comparison above being true which triggers a setSceneState which then has no effect since the
			// real state is already lower than the set state which leads to no callback being invoked which leads to waiting forever
			// of using an exact scene state comparison.
			backend.eventHandler().waitForSceneState(sceneId, state, SceneStateEventHandler::ECompareFunc::LessEqual);
		}
	}
}

}  // namespace

namespace raco::ramses_widgets {

RamsesAbstractViewWindow::RamsesAbstractViewWindow(void* windowHandle, RendererBackend& rendererBackend)
	: windowHandle_{windowHandle}, rendererBackend_{rendererBackend}, displayId_{ramses::displayId_t::Invalid()} {
}

RamsesAbstractViewWindow::~RamsesAbstractViewWindow() {
	reset();
}

void RamsesAbstractViewWindow::reset() {
	reduceAndWaitSceneState(rendererBackend_, ramses::RendererSceneState::Available, current_.sceneId);
	if (displayId_.isValid()) {
		rendererBackend_.renderer().destroyDisplay(displayId_);
		rendererBackend_.renderer().flush();
		rendererBackend_.eventHandler().waitForDisplayDestruction(displayId_);
		displayId_ = ramses::displayId_t::Invalid();
		current_ = State{};
	}
}

const RamsesAbstractViewWindow::State& RamsesAbstractViewWindow::currentState() {
	return current_;
}

RamsesAbstractViewWindow::State& RamsesAbstractViewWindow::nextState() {
	return next_;
}

void RamsesAbstractViewWindow::commit(bool forceUpdate) {
	if (forceUpdate || !displayId_.isValid() || next_ != current_) {
		// Unload current scenes
		reset();

		auto& sceneControlAPI = *rendererBackend_.renderer().getSceneControlAPI();

		if (!displayId_.isValid()) {
			ramses::DisplayConfig displayConfig = {};
			auto backgroundColor = style::Colors::color(style::Colormap::abstractSceneViewBackground);
			displayConfig.setWindowRectangle(0, 0, next_.viewportSize.width(), next_.viewportSize.height());
			displayConfig.setClearColor({backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), backgroundColor.alphaF()});
			displayConfig.setWindowIviVisible();
			if constexpr (!BuildOptions::nativeRamsesDisplayWindow) {
#if (defined(__WIN32) || defined(_WIN32))
				displayConfig.setWindowsWindowHandle(windowHandle_);
#else
				displayConfig.setX11WindowHandle(ramses::X11WindowHandle(reinterpret_cast<unsigned long>(windowHandle_)));
#endif
			}
			displayId_ = rendererBackend_.renderer().createDisplay(displayConfig);
			rendererBackend_.renderer().flush();
			rendererBackend_.eventHandler().waitForDisplayCreation(displayId_);

			if (next_.sceneId.isValid()) {
				/// @todo maybe we need to reset old scene mapping?
				sceneControlAPI.setSceneMapping(next_.sceneId, displayId_);
				sceneControlAPI.flush();
				setAndWaitSceneState(rendererBackend_, ramses::RendererSceneState::Rendered, next_.sceneId);
			}

			current_ = next_;
		}
	}
}

}  // namespace raco::ramses_widgets
