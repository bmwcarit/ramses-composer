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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class QJsonArray;
class QJsonObject;
class QJsonParseError;
class QJsonValue;

namespace raco::core {
class UndoStack;
class DataChangeRecorder;
class Project;
class ValueHandle;
class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;
enum class ErrorLevel;
}  // namespace raco::core

namespace raco::components {
class TracePlayer {
public:
	using timeInMilliSeconds = int64_t;

	enum class PlayerState {
		Faulty = -1,
		Init = 0,
		Stopped = 1,
		Playing = 2,
		Paused = 3
	};

	using OnStateChangeCallback = std::function<void(PlayerState)>;
	using OnLuaUpdateCallback = std::function<void(int)>;
	using OnLogUpdateCallback = std::function<void(const std::vector<std::string>&, core::ErrorLevel)>;

	~TracePlayer();
	TracePlayer() = delete;
	TracePlayer(const TracePlayer&) = delete;
	TracePlayer& operator=(const TracePlayer&) = delete;
	TracePlayer(TracePlayer&&) = delete;
	TracePlayer& operator=(TracePlayer&&) = delete;
	TracePlayer(core::Project& project, core::DataChangeRecorder& uiChanges, core::UndoStack& undoStack);

	void setCallbacks(
		const OnStateChangeCallback& onStateChange = [](PlayerState) {},
		const OnLuaUpdateCallback& onLuaUpdate = [](int) {},
		const OnLogUpdateCallback& onLogChange = [](const std::vector<std::string>&, core::ErrorLevel) {});
	void refresh(timeInMilliSeconds elapsedTimeSinceStart);
	std::string const& getFilePath() const;
	int getTraceLen() const;
	PlayerState getState() const;
	int getIndex() const;
	int getTimestamp() const;
	bool getLoopingStatus() const;
	void clearLog();
	std::unordered_map<std::string, core::ErrorLevel> const& getLog() const;

	/// player playback controls
	QJsonArray const* const loadTrace(const std::string& fileName);
	void play();
	void pause();
	void stop();
	void step(int step);
	void jumpTo(int newIndex);
	void setSpeed(double speed);
	void toggleLooping();

	core::DataChangeRecorder& uiChanges() const;

private:
	raco::core::SEditorObject findLua(const std::string& luaObjName, bool logErrors = true);
	QJsonObject parseFrame(int frameIndex);
	QJsonObject parseSceneData(const QJsonObject& qjFrame, int frameIndex = -1);
	QJsonObject parseTracePlayerData(const QJsonObject& qjFrame, int frameIndex = -1);
	int parseTimestamp(const QJsonObject& qjTracePlayerData, int frameIndex = -1);
	bool parseFrameAndUpdateLua();
	void updateLua(const QJsonValue& jsonChild, std::vector<std::string>& keysChain, const raco::core::SEditorObject& lua);
	void updateLuaProperty(const raco::core::SEditorObject& lua, const std::vector<std::string>& keysChain, const QJsonValue& jsonChild, bool logErrors = true);
	std::string streamKeysChain(const std::vector<std::string>& keysChain) const;
	void qjParseErrMsg(const QJsonParseError& qjParseError, const std::string& fileName);
	void clearError(const std::string& msg, core::ErrorLevel level);
	void reset();
	void failSafe();
	bool isLastFrame() const;
	timeInMilliSeconds getNextTs();
	void setState(PlayerState newState);
	void addError(const std::string& msg, core::ErrorLevel level);
	void lockLua();
	void makeFramesConsistent();
	QJsonValue deepAddMissingProperties(const QJsonValue& qjPrev, const QJsonValue& qjCurr, std::vector<std::string>& propertyPath);
	QJsonValue buildFullFrameFromLua(std::unordered_set<core::SEditorObject> const& sceneLuaList);
	QJsonValue deepCopyFromLua(raco::core::ValueHandle const& luaValHandle);
	void rebuildFrameSceneData(int index, const QJsonValue& qjPrev, const QJsonValue& qjCurr);

	class CodeControlledObjectExtension;
	std::unique_ptr<CodeControlledObjectExtension> racoCoreInterface_;
	std::unique_ptr<QJsonArray> qjRoot_;
	PlayerState state_{PlayerState::Init};
	double speed_{1.0};
	int playbackIndex_{0};
	OnStateChangeCallback onStateChange_{nullptr};
	OnLuaUpdateCallback onLuaUpdate_{nullptr};
	OnLogUpdateCallback onLogChange_{nullptr};
	std::unordered_map<std::string, core::ErrorLevel> tracePlayerLog_{};
	std::vector<std::string> logReport_{};
	core::ErrorLevel highestCriticality_;
	timeInMilliSeconds refreshTs_{0};
	timeInMilliSeconds playbackTs_{0};
	timeInMilliSeconds timeOfLastRefresh_{0};
	bool looping_{false};
	std::string filePath_{};
	std::vector<int> framesTsList_{};
};

}  // namespace raco::components
