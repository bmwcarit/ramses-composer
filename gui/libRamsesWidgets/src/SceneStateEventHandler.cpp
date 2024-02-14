/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_widgets/SceneStateEventHandler.h"

#include "log_system/log.h"
#include "ramses_base/RamsesFormatter.h"
#include "ramses_widgets/RamsesPreviewWindow.h"

using raco::log_system::RAMSES_BACKEND;

#include<stdexcept>
#include <thread>

namespace raco::ramses_widgets {

SceneStateEventHandler::SceneStateEventHandler(ramses::RamsesRenderer& renderer)
	: renderer_(renderer) {
}

void SceneStateEventHandler::framebufferPixelsRead(const uint8_t* pixelData, const uint32_t pixelDataSize, ramses::displayId_t displayId, ramses::displayBufferId_t displayBuffer, ramses::ERendererEventResult result) {
	if (!screenshot_.empty()) {
		screenshotSaved_ = false;
		if (result == ramses::ERendererEventResult::Ok) {
			std::vector<uint8_t> buffer;
			buffer.insert(buffer.end(), &pixelData[0], &pixelData[pixelDataSize]);
			screenshotSaved_ = ramses::RamsesUtils::SaveImageBufferToPng(screenshot_, buffer, screenshotWidth_, screenshotHeight_, true);
		}
		screenshot_.clear();
	}
}

void SceneStateEventHandler::offscreenBufferLinked(ramses::displayBufferId_t offscreenBufferId, ramses::sceneId_t sceneId, ramses::dataConsumerId_t, bool success) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferLinked({}, {}, {})", offscreenBufferId, sceneId, success);
	if (success) {
		linkedOffscreenBuffers_.insert(offscreenBufferId);
	}
}

void SceneStateEventHandler::offscreenBufferCreated(ramses::displayId_t displayId, ramses::displayBufferId_t offscreenBufferId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferCreated({}, {}, {})", displayId, offscreenBufferId, result);
	if (result == ramses::ERendererEventResult::Ok) {
		offscreenBuffers_.insert(offscreenBufferId);
	}
}

void SceneStateEventHandler::offscreenBufferDestroyed(ramses::displayId_t displayId, ramses::displayBufferId_t offscreenBufferId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "offscreenBufferDestroyed({}, {}, {})", displayId, offscreenBufferId, result);
	offscreenBuffers_.erase(offscreenBufferId);
}

void SceneStateEventHandler::displayCreated(ramses::displayId_t displayId, ramses::ERendererEventResult result) {
	LOG_TRACE(RAMSES_BACKEND, "displayCreated({}, {})", displayId, result);
	if (result == ramses::ERendererEventResult::Ok)
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

void SceneStateEventHandler::waitForSceneState(ramses::sceneId_t sceneId, ramses::RendererSceneState state, ECompareFunc compFunc) {
	waitUntilOrTimeout([this, sceneId, state, compFunc] {
		switch (compFunc) {
			case ECompareFunc::LessEqual:
				return scenes_[sceneId].state <= state;
			case ECompareFunc::Equal:
				return scenes_[sceneId].state == state;
			case ECompareFunc::GreaterEqual:
				return scenes_[sceneId].state >= state;
			default:
				return false;
		}
	});
}

void SceneStateEventHandler::objectsPicked(ramses::sceneId_t sceneId, const ramses::pickableObjectId_t* pickedObjects, size_t pickedObjectsCount) {
	std::string ids;
	std::vector<ramses::pickableObjectId_t> pickIds;
	for (int index = 0; index < pickedObjectsCount; index++) {
		ids.append(std::to_string(pickedObjects[index].getValue()));
		ids.append(", ");
		pickIds.emplace_back(pickedObjects[index]);
	}
	LOG_TRACE(RAMSES_BACKEND, "objectsPicked({})", ids);
	Q_EMIT pickRequest(pickIds);
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

bool SceneStateEventHandler::waitForScreenshot() {
	waitUntilOrTimeout([&] { return screenshot_.empty(); });
	return screenshotSaved_;
}

ramses::RendererSceneState SceneStateEventHandler::sceneState(ramses::sceneId_t sceneId) {
	return scenes_[sceneId].state;
}

bool SceneStateEventHandler::saveScreenshot(const std::string& filename, ramses::displayId_t displayId, ramses::displayBufferId_t screenshotBuf, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	if (screenshot_.empty() && !filename.empty()) {
		screenshotSaved_ = false;
		screenshot_ = filename;
		screenshotWidth_ = width - x;
		screenshotHeight_ = height - x;
		renderer_.readPixels(displayId, screenshotBuf, x, y, width, height);
		renderer_.flush();
		return true;
	}
	return false;
}

}  // namespace raco::ramses_widgets