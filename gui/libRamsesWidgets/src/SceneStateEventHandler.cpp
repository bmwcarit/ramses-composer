/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/SceneStateEventHandler.h"

#include "log_system/log.h"
#include "ramses_base/RamsesFormatter.h"

using raco::log_system::RAMSES_BACKEND;

#include <stdexcept>
#include <thread>

namespace raco::ramses_widgets {

SceneStateEventHandler::SceneStateEventHandler(ramses::RamsesRenderer& renderer)
	: renderer_(renderer) {
}

void SceneStateEventHandler::offscreenBufferLinked(ramses::displayBufferId_t offscreenBufferId, ramses::sceneId_t sceneId, ramses::dataConsumerId_t, bool success) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferLinked({}, {}, {})", offscreenBufferId, sceneId, success);
	if (success) {
		linkedOffscreenBuffers_.insert(offscreenBufferId);
	}
}

void SceneStateEventHandler::offscreenBufferCreated(ramses::displayId_t displayId, ramses::displayBufferId_t offscreenBufferId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferCreated({}, {}, {})", displayId, offscreenBufferId, result);
	if (result == ramses::ERendererEventResult_OK) {
		offscreenBuffers_.insert(offscreenBufferId);
	}
}

void SceneStateEventHandler::offscreenBufferDestroyed(ramses::displayId_t displayId, ramses::displayBufferId_t offscreenBufferId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferDestroyed({}, {}, {})", displayId, offscreenBufferId, result);
	offscreenBuffers_.erase(offscreenBufferId);
}

void SceneStateEventHandler::displayCreated(ramses::displayId_t displayId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "displayCreated({}, {})", displayId, result);
	if (result == ramses::ERendererEventResult::ERendererEventResult_OK)
		displays_.insert(displayId);
}

void SceneStateEventHandler::displayDestroyed(ramses::displayId_t displayId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "displayDestroyed({}, {})", displayId, result);
	displays_.erase(displayId);
}

void SceneStateEventHandler::sceneStateChanged(ramses::sceneId_t sceneId, ramses::RendererSceneState state) {
	LOG_TRACE(RAMSES_BACKEND, "sceneStateChanged({}, {})", sceneId, state);
	scenes_[sceneId].state = state;
}

void SceneStateEventHandler::sceneFlushed(ramses::sceneId_t sceneId, ramses::sceneVersionTag_t sceneVersion) {
	LOG_TRACE(RAMSES_BACKEND, "sceneFlushed({}, {})", sceneId, sceneVersion);
	scenes_[sceneId].version = sceneVersion;
}

void SceneStateEventHandler::waitForSceneState(ramses::sceneId_t sceneId, ramses::RendererSceneState state) {
	waitUntilOrTimeout([&] { return scenes_[sceneId].state == state; });
}

bool SceneStateEventHandler::waitForFlush(ramses::sceneId_t sceneId, ramses::sceneVersionTag_t sceneVersion) {
	return waitUntilOrTimeout([&] { return scenes_[sceneId].version == sceneVersion; });
}

bool SceneStateEventHandler::waitForDisplayCreation(ramses::displayId_t displayId) {
	return waitUntilOrTimeout([&] { return displays_.find(displayId) != displays_.end(); });
}

bool SceneStateEventHandler::waitForDisplayDestruction(ramses::displayId_t displayId) {
	return waitUntilOrTimeout([&] { return displays_.find(displayId) == displays_.end(); });
}

bool SceneStateEventHandler::waitForOffscreenBufferCreation(ramses::displayBufferId_t displayBufferId) {
	return waitUntilOrTimeout([&] { return offscreenBuffers_.find(displayBufferId) != offscreenBuffers_.end(); });
}

bool SceneStateEventHandler::waitForOffscreenBufferDestruction(ramses::displayBufferId_t displayBufferId) {
	return waitUntilOrTimeout([&] { return offscreenBuffers_.find(displayBufferId) == offscreenBuffers_.end(); });
}

bool SceneStateEventHandler::waitForOffscreenBufferLinked(ramses::displayBufferId_t displayBufferId) {
	return waitUntilOrTimeout([&] { return linkedOffscreenBuffers_.find(displayBufferId) == linkedOffscreenBuffers_.end(); });
}

bool SceneStateEventHandler::waitUntilOrTimeout(const std::function<bool()>& conditionFunction) {
	const std::chrono::steady_clock::time_point timeoutTS = std::chrono::steady_clock::now() + std::chrono::seconds{5};
	while (!conditionFunction()) {
		if (std::chrono::steady_clock::now() > timeoutTS) {
			throw std::runtime_error{"Something went wrong"};
		}
		std::this_thread::sleep_for(std::chrono::milliseconds{5});
		renderer_.doOneLoop();
		renderer_.dispatchEvents(*this);
		renderer_.getSceneControlAPI()->dispatchEvents(*this);
	}
	return conditionFunction();
}

template <typename T>
void SceneStateEventHandler::waitForElementInSet(const T element, const std::unordered_set<T>& set) {
	while (set.find(element) == set.end()) {
		renderer_.dispatchEvents(*this);
		renderer_.getSceneControlAPI()->dispatchEvents(*this);
		std::this_thread::sleep_for(std::chrono::milliseconds(10u));
	}
}

ramses::RendererSceneState SceneStateEventHandler::sceneState(ramses::sceneId_t sceneId) {
	return scenes_[sceneId].state;
}

}  // namespace raco::ramses_widgets