/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * For more details about TracePlayer Finite State Machine (FSM), please refer to this diagram: https://azure.paradoxcat.com/confluence/x/NY_QNQ
 */

#include <random>
#include <string>

#include "gtest/gtest.h"
#include "testing/RaCoApplicationTest.h"

#include "application/RaCoApplication.h"
#include "components/TracePlayer.h"
#include "core/CodeControlledPropertyModifier.h"
#include "core/EditorObject.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

using namespace raco::components;
constexpr int RENDER_TIME_TOLERENCE_MS = 50;
enum class LuaType {
	LuaScript,
	LuaInterface
};

class TracePlayerTest : public RacoBaseTest<> {
protected:
	void SetUp() override {
		RacoBaseTest::SetUp();
		spdlog::drop_all();
		raco::log_system::init();
		raco::log_system::setConsoleLogLevel(spdlog::level::level_enum::info);
		application.overrideTime([this] {
			return tracePlayerTestTime_;
		});
		cmd_ = application.activeRaCoProject().commandInterface();
		player_ = &application.activeRaCoProject().tracePlayer();
		tracePlayerDataLua_ = createLua("TracePlayerData", LuaType::LuaScript, false)->as<raco::user_types::LuaScript>();
		createLua("saInfo", LuaType::LuaScript);
	}

	raco::core::CommandInterface* cmd_{nullptr};
	TracePlayer* player_{nullptr};
	std::vector<raco::core::SEditorObject> luaObjs_;
	raco::user_types::SLuaScript tracePlayerDataLua_{nullptr};

	/// states transition validation
	void isInit() const;
	void isFaulty() const;
	void isStopped() const;
	void isPlaying() const;
	void isPaused() const;

	/// utilities
	QJsonArray const* const loadTrace(std::string const& fileName);
	raco::core::SEditorObject createLua(std::string const& luaName, const LuaType type, bool sceneScript = true, raco::core::SEditorObject parent = nullptr);
	std::unordered_map<std::string, raco::data_storage::Table> const* const backupSceneLuaObjs(QJsonArray const* const);
	void deleteLuaObj(raco::core::SEditorObject lua);
	void playOneFrame();
	void increaseTimeAndDoOneLoop();
	void setMinMaxFrameTime(long minFrameTime, long maxFrameTime);

	/// validations
	bool isValidRange() const;
	bool areLuaObjsRestored(std::unordered_map<std::string, raco::data_storage::Table> const* const luaObjsBackup) const;
	bool areAllLuaObjsLocked() const;
	bool areAllLuaObjsUnlocked() const;
	raco::core::SEditorObject findLua(std::string const& luaName) const;

	long long tracePlayerTestTime_{0};	// we need a reproducable time line to avoid flaky tests.
	long long lastFrameTime_{-1};
	long minFrameTime_{5};
	long maxFrameTime_{300};
	std::minstd_rand frameTimeGenerator_{42};

	int argc{0};
	QCoreApplication eventLoop_{argc, nullptr};
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication application{backend, {{}, false, false, -1, -1, false}};
};

/* ************************************** helper functions ************************************** */
raco::core::SEditorObject TracePlayerTest::createLua(std::string const& luaName, const LuaType type, bool sceneScript, raco::core::SEditorObject parent) {
	const auto lua_filename{luaName + ".lua"};
	raco::core::SEditorObject luaObj;
	if (type == LuaType::LuaScript) {
		luaObj = cmd_->createObject(raco::user_types::LuaScript::typeDescription.typeName, luaName, parent);
	} else if (type == LuaType::LuaInterface) {
		luaObj = cmd_->createObject(raco::user_types::LuaInterface::typeDescription.typeName, luaName, parent);
	}

	cmd_->set(raco::core::ValueHandle{luaObj, {"uri"}}, (test_path() / "lua_scripts" / lua_filename).string());

	if (sceneScript) {
		luaObjs_.push_back(luaObj);
	}

	return luaObj;
}

raco::core::SEditorObject TracePlayerTest::findLua(std::string const& luaObjName) const {
	const auto isControllableLua{[&luaObjName](const raco::core::SEditorObject& o) {
		return (
			((o->objectName() == luaObjName) && (!raco::core::PrefabOperations::findContainingPrefab(o))) &&
			((o->isType<raco::user_types::LuaInterface>()) ||
				(o->isType<raco::user_types::LuaScript>() && (!raco::core::PrefabOperations::findContainingPrefabInstance(o)))));
	}};

	if (const auto itrLuaObj{std::find_if(cmd_->project()->instances().cbegin(), cmd_->project()->instances().cend(), isControllableLua)};
		itrLuaObj != cmd_->project()->instances().cend()) {
		if (const auto itrDuplicateLua{std::find_if(itrLuaObj + 1, cmd_->project()->instances().cend(), isControllableLua)};
			itrDuplicateLua != cmd_->project()->instances().cend()) {
			return nullptr;
		}

		const auto lua{*itrLuaObj};
		if (raco::user_types::Queries::isReadOnly(lua)) {
			return nullptr;
		}

		return lua;
	}

	return nullptr;
}

void TracePlayerTest::deleteLuaObj(raco::core::SEditorObject lua) {
	EXPECT_EQ(cmd_->deleteObjects(std::vector<raco::core::SEditorObject>{lua}), 1);
	const auto lastRemoved{luaObjs_.erase(std::remove(luaObjs_.begin(), luaObjs_.end(), lua), luaObjs_.end())};
	EXPECT_EQ(lastRemoved, luaObjs_.end());
}

QJsonArray const* const TracePlayerTest::loadTrace(std::string const& fileName) {
	return player_->loadTrace((test_path() / fileName).string());
}

/// backup features Lua nodes from Scene (e.g. saInfo Lua node) to validate backup/restore mechanism
std::unordered_map<std::string, raco::data_storage::Table> const* const TracePlayerTest::backupSceneLuaObjs(QJsonArray const* const qjTrace) {
	std::unordered_map<std::string, raco::data_storage::Table>* const luaObjsBackup{new std::unordered_map<std::string, raco::data_storage::Table>()};

	for (int frameIndex{0}; frameIndex < qjTrace->size(); ++frameIndex) {
		const auto qjSceneData{qjTrace->at(frameIndex).toObject().value("SceneData").toObject()};
		for (auto itr{qjSceneData.constBegin()}; itr != qjSceneData.constEnd(); ++itr) {
			if (const auto lua{findLua(itr.key().toStdString())}) {
				luaObjsBackup->emplace(lua->objectName(), lua->get("inputs")->asTable());
			} else {
				continue;
			}
		}
	}

	return luaObjsBackup;
}

bool TracePlayerTest::areAllLuaObjsLocked() const {
	bool ret{true};

	for (auto const& lua : luaObjs_) {
		ret = ret && cmd_->project()->isCodeCtrldObj(lua);
	}

	return ret;
}

bool TracePlayerTest::areAllLuaObjsUnlocked() const {
	bool ret{true};

	for (auto const& lua : luaObjs_) {
		ret = ret && !cmd_->project()->isCodeCtrldObj(lua);
	}

	return ret;
}

bool TracePlayerTest::isValidRange() const {
	return ((player_->getIndex() >= 0) && (player_->getIndex() < player_->getTraceLen()));
}

bool TracePlayerTest::areLuaObjsRestored(std::unordered_map<std::string, raco::data_storage::Table> const* const luaObjsBackup) const {
	bool restored{true};
	bool atLeastOneWasFound{false};

	if (!luaObjsBackup->empty()) {
		for (auto const& luaBackupEntry : *luaObjsBackup) {
			if (auto lua{findLua(luaBackupEntry.first)}) {
				atLeastOneWasFound = true;
				restored = restored && (lua->get("inputs")->asTable() == luaBackupEntry.second);
			} else {
				continue;
			}
		}
	}

	return (restored && atLeastOneWasFound);
}

void TracePlayerTest::increaseTimeAndDoOneLoop() {
	lastFrameTime_ = minFrameTime_;
	if (maxFrameTime_ > minFrameTime_) {
		lastFrameTime_ += frameTimeGenerator_() % (maxFrameTime_ - minFrameTime_);
	}
	tracePlayerTestTime_ += lastFrameTime_;
	application.doOneLoop();
}

void TracePlayerTest::setMinMaxFrameTime(long minFrameTime, long maxFrameTime) {
	minFrameTime_ = std::max(minFrameTime, 0l);
	maxFrameTime_ = std::max(maxFrameTime, minFrameTime_);
}

void TracePlayerTest::playOneFrame() {
	const auto lastIndex{player_->getIndex()};
	while ((player_->getIndex() <= lastIndex) && (player_->getIndex() < player_->getTraceLen() - 1)) {
		increaseTimeAndDoOneLoop();
		if (player_->getState() != TracePlayer::PlayerState::Playing) {
			return;
		}
	}
}

/* ****************************************** States ****************************************** */
void TracePlayerTest::isInit() const {
	ASSERT_EQ(TracePlayer::PlayerState::Init, player_->getState());

	/// entry: is there any loaded trace?
	EXPECT_EQ(-1, player_->getTraceLen());
}

void TracePlayerTest::isFaulty() const {
	ASSERT_EQ(TracePlayer::PlayerState::Faulty, player_->getState());
}

void TracePlayerTest::isStopped() const {
	ASSERT_EQ(TracePlayer::PlayerState::Stopped, player_->getState());

	/// entry: are Lua nodes unlocked?
	EXPECT_EQ(true, areAllLuaObjsUnlocked());
}

void TracePlayerTest::isPlaying() const {
	ASSERT_EQ(TracePlayer::PlayerState::Playing, player_->getState());

	/// entry: are Lua nodes locked?
	EXPECT_EQ(true, areAllLuaObjsLocked());
}

void TracePlayerTest::isPaused() const {
	ASSERT_EQ(TracePlayer::PlayerState::Paused, player_->getState());

	/// entry: are Lua nodes locked?
	EXPECT_EQ(true, areAllLuaObjsLocked());

	// entry: is frame index at valid range?
	EXPECT_EQ(true, isValidRange());
}

/* ####################################### Test Fixtures ####################################### */

TEST_F(TracePlayerTest, TF01_ValidTraceLoad) {
	isInit();

	EXPECT_NE(nullptr, loadTrace("raco_traces/valid_20211123.rctrace"));
	isStopped();
}

TEST_F(TracePlayerTest, TF02_InvalidTrace_Empty) {
	isInit();

	EXPECT_EQ(nullptr, loadTrace("raco_traces/invalid_empty.rctrace"));
	isFaulty();
}

TEST_F(TracePlayerTest, TF03_InvalidTrace_WrongRootType) {
	isInit();

	EXPECT_EQ(nullptr, loadTrace("raco_traces/invalid_wrongRootType.rctrace"));
	isFaulty();
}

TEST_F(TracePlayerTest, TF04_InvalidTrace_WrongFormat) {
	isInit();

	EXPECT_EQ(nullptr, loadTrace("raco_traces/invalid_wrongFormat.rctrace"));
	isFaulty();
}

TEST_F(TracePlayerTest, TF05_InvalidTrace_WrongExtension) {
	isInit();

	EXPECT_EQ(nullptr, loadTrace("raco_traces/invalid_wrongExtension.so"));
	isFaulty();
}

TEST_F(TracePlayerTest, TF11_Playing_Paused_playToEnd) {
	constexpr int FRAME_UPDATE_TIME_MS = 500;

	auto qjTrace{loadTrace("raco_traces/valid_10frames_500ms.rctrace")};
	isStopped();

	/// run playback to end
	/// dummy doOneLoop to update totalElapsedMsec_ in racoApp
	increaseTimeAndDoOneLoop();
	player_->play();
	const auto traceStartTs{player_->getTimestamp()};
	const auto startTime{tracePlayerTestTime_};
	while (player_->getIndex() < player_->getTraceLen() - 1) {
		auto lastDoOneLoopTime{tracePlayerTestTime_};
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		lastDoOneLoopTime = tracePlayerTestTime_ - lastDoOneLoopTime;
		if (prevFrameIndex >= 0 && currentFrameIndex > prevFrameIndex) {
			const auto appTimeline{tracePlayerTestTime_ - startTime};
			const auto traceTimeline{player_->getTimestamp() - traceStartTs};
			EXPECT_GE(appTimeline, traceTimeline);
			EXPECT_LE(appTimeline, traceTimeline + lastDoOneLoopTime);
		}
	}

	/// is playback ended (paused at last frame)?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 1);
	isPaused();
}

TEST_F(TracePlayerTest, TF12_Playing_Paused_pause) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	player_->pause();
	/// is playback paused?
	isPaused();
}

TEST_F(TracePlayerTest, TF13_Playing_Playing_restart) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and restart it midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	player_->play();
	/// is playback restarted?
	ASSERT_EQ(player_->getIndex(), -1);

	playOneFrame();
	isPlaying();
}

TEST_F(TracePlayerTest, TF14_Playing_Faulty_noLua) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and delete all Lua nodes midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	for (auto& lua : luaObjs_) {
		deleteLuaObj(lua);
		EXPECT_EQ(nullptr, findLua(lua->objectName()));
	}

	playOneFrame();
	isFaulty();
}

TEST_F(TracePlayerTest, TF15_Playing_Faulty_invalidTrace) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and load invalid trace midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	loadTrace("raco_traces/invalid_empty.rctrace");
	isFaulty();
}

TEST_F(TracePlayerTest, TF16_Playing_Jump_Paused) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and jump midway near end
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	/// is playback jumped and paused near end?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 3);
	isPaused();
}

TEST_F(TracePlayerTest, TF17_Playing_Jump_Faulty_noLua) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and jump midway near end
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	for (auto& lua : luaObjs_) {
		deleteLuaObj(lua);
		EXPECT_EQ(nullptr, findLua(lua->objectName()));
	}

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF18_Playing_Jump_Faulty_outOfRange) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and jump out-of-range midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->jumpTo(player_->getTraceLen() * 2);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF19_Playing_Stopped) {
	auto qjTrace{loadTrace("raco_traces/valid_20211123.rctrace")};
	isStopped();

	auto luaObjsBackup{backupSceneLuaObjs(qjTrace)};

	player_->play();
	/// run playback and stop it midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	player_->stop();
	EXPECT_EQ(true, areLuaObjsRestored(luaObjsBackup));
	isStopped();
}

TEST_F(TracePlayerTest, TF21_Stopped_Jump_Paused) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	/// is playback jumped and paused near end?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 3);
	isPaused();
}

TEST_F(TracePlayerTest, TF22_Stopped_Jump_Faulty_noLua) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	for (auto& lua : luaObjs_) {
		deleteLuaObj(lua);
		EXPECT_EQ(nullptr, findLua(lua->objectName()));
	}

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF23_Stopped_Jump_Faulty_outOfRange) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->jumpTo(player_->getTraceLen() * 2);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF24_Stopped_Faulty_invalidTrace) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	loadTrace("raco_traces/invalid_empty.rctrace");
	isFaulty();
}

TEST_F(TracePlayerTest, TF25_Stopped_Jump_End_Paused) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->jumpTo(player_->getTraceLen() - 1);
	increaseTimeAndDoOneLoop();
	/// is playback jumped and paused at end?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 1);
	isPaused();
}

TEST_F(TracePlayerTest, TF31_Paused_Stopped) {
	const auto qjTrace{loadTrace("raco_traces/valid_20211123.rctrace")};
	isStopped();

	auto luaObjsBackup{backupSceneLuaObjs(qjTrace)};

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	player_->pause();
	isPaused();

	player_->stop();
	EXPECT_EQ(true, areLuaObjsRestored(luaObjsBackup));
	isStopped();
}

TEST_F(TracePlayerTest, TF32_Paused_Jump_Paused) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->pause();
	isPaused();

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	/// is playback jumped and paused near end?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 3);
	isPaused();
}

TEST_F(TracePlayerTest, TF33_Paused_Jump_Faulty_noLua) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->pause();
	isPaused();

	for (auto& lua : luaObjs_) {
		deleteLuaObj(lua);
		EXPECT_EQ(nullptr, findLua(lua->objectName()));
	}

	player_->jumpTo(player_->getTraceLen() - 3);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF34_Paused_Jump_Faulty_outOfRange) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->pause();
	isPaused();

	player_->jumpTo(player_->getTraceLen() * 2);
	increaseTimeAndDoOneLoop();
	isFaulty();
}

TEST_F(TracePlayerTest, TF35_Paused_Playing_resume) {
	constexpr int FRAME_UPDATE_TIME_MS = 500;
	constexpr int ELAPSED_TILL_PAUSED_MS = 170;
	setMinMaxFrameTime(10, RENDER_TIME_TOLERENCE_MS);

	loadTrace("raco_traces/valid_10frames_500ms.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	// Move the timeline in the current frame of 500ms to at least 1/3 of the frame time, but not outside the frame.
	while (player_->getIndex() < player_->getTraceLen()) {
		increaseTimeAndDoOneLoop();
		if (player_->getIndex() > player_->getTraceLen() / 2) {
			const auto startTimeInFrame = tracePlayerTestTime_;
			while (tracePlayerTestTime_ - startTimeInFrame < ELAPSED_TILL_PAUSED_MS) {
				increaseTimeAndDoOneLoop();
			}
			ASSERT_LT(tracePlayerTestTime_ - startTimeInFrame, FRAME_UPDATE_TIME_MS);
			break;
		}
	}

	// Pause and do a few loop
	const auto wallTimeBeforePause = tracePlayerTestTime_;
	const auto tracePlayerTimeBeforePause = player_->getTimestamp();
	player_->pause();
	isPaused();
	increaseTimeAndDoOneLoop();
	increaseTimeAndDoOneLoop();
	increaseTimeAndDoOneLoop();
	ASSERT_GT(tracePlayerTestTime_, wallTimeBeforePause);
	ASSERT_EQ(player_->getTimestamp(), tracePlayerTimeBeforePause);

	// Advance into the next frame and make sure the trace player and our wall clock agree on how much time needs to be elapsed for the next frame
	player_->play();
	const auto wallTimeBeforePlay = tracePlayerTestTime_;
	const long timeToBePlayedInThisFrame = FRAME_UPDATE_TIME_MS - (player_->getTimestamp() % FRAME_UPDATE_TIME_MS), playOneFrame();
	const long timePlayedInNextFrame = player_->getTimestamp() % FRAME_UPDATE_TIME_MS;
	EXPECT_EQ(FRAME_UPDATE_TIME_MS - timeToBePlayedInThisFrame + timePlayedInNextFrame, tracePlayerTestTime_ - wallTimeBeforePlay);

	isPlaying();
}

TEST_F(TracePlayerTest, TF36_Paused_Playing_restart) {
	auto qjTrace{loadTrace("raco_traces/valid_20211123.rctrace")};
	isStopped();

	player_->play();
	/// run playback to end
	while (player_->getIndex() < player_->getTraceLen() - 1) {
		isPlaying();
		const auto prevFrameIndex{player_->getIndex()};
		increaseTimeAndDoOneLoop();
		const auto currentFrameIndex{player_->getIndex()};
		ASSERT_GE(currentFrameIndex, prevFrameIndex);
	}

	/// is playback ended (paused at last frame)?
	ASSERT_EQ(player_->getIndex(), player_->getTraceLen() - 1);
	isPaused();

	player_->play();
	playOneFrame();
	/// has playback restarted?
	isPlaying();
}

TEST_F(TracePlayerTest, TF37_Paused_Faulty_invalidTrace) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->pause();
	isPaused();

	loadTrace("raco_traces/invalid_empty.rctrace");
	isFaulty();
}

TEST_F(TracePlayerTest, TF41_Faulty_Stopped) {
	isInit();

	EXPECT_EQ(nullptr, loadTrace("raco_traces/invalid_wrongFormat.rctrace"));
	isFaulty();

	EXPECT_NE(nullptr, loadTrace("raco_traces/valid_20211123.rctrace"));
	isStopped();
}

TEST_F(TracePlayerTest, TF71_Play_DeleteLua_Play_Failed_Recreate_Play) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	deleteLuaObj(findLua("saInfo"));

	playOneFrame();

	isFaulty();

	createLua("saInfo", LuaType::LuaScript);

	player_->stop();
	player_->play();

	playOneFrame();

	isPlaying();
}

TEST_F(TracePlayerTest, TF81_LinkedProperty) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	const auto saInfoLua{findLua("saInfo")};
	const auto srcLua{createLua("dummyAllTypes", LuaType::LuaScript)};

	const auto srcPropIn = raco::core::ValueHandle{srcLua, {"inputs", "propIntegerIn"}};
	const auto srcPropOut = raco::core::ValueHandle{srcLua, {"outputs", "propIntegerOut"}};
	const auto destProp = raco::core::ValueHandle{saInfoLua, {"inputs", "ACC", "state"}};
	cmd_->set(srcPropIn, 123000321);

	/*** Test setting linked property ***/
	{
		EXPECT_NE(123000321, destProp.asInt());
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::NOT_LINKED, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		EXPECT_NE(nullptr, cmd_->addLink(srcPropOut, destProp));
		increaseTimeAndDoOneLoop();
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::LINKED, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		EXPECT_EQ(123000321, destProp.asInt());

		player_->play();
		playOneFrame();
		EXPECT_EQ(123000321, destProp.asInt());
		const auto& log{player_->getLog()};
		ASSERT_EQ(log.size(), 1);
		ASSERT_NE(log.find("Can not set linked property! ( propPath: saInfo->ACC->state )"), log.cend());

		// EXPECT_ANY_THROW(cmd_->removeLink(destProp.getDescriptor())); currently this would fail, see RAOS-905.
		player_->stop();
		isStopped();
		cmd_->removeLink(destProp.getDescriptor());

		player_->play();
		increaseTimeAndDoOneLoop();
		playOneFrame();
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::NOT_LINKED, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		playOneFrame();
		EXPECT_NE(123000321, destProp.asInt());
		ASSERT_EQ(log.size(), 0);
		ASSERT_EQ(log.find("Can not set linked property! ( propPath: saInfo->ACC->state )"), log.cend());
	}

	/*** Test setting property with broken link ***/
	{
		player_->stop();
		cmd_->set(srcPropIn, 123000322);
		increaseTimeAndDoOneLoop();
		EXPECT_EQ(123000322, srcPropOut.asInt());

		EXPECT_NE(nullptr, cmd_->addLink(srcPropOut, destProp));
		increaseTimeAndDoOneLoop();
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::LINKED, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		player_->play();
		playOneFrame();
		EXPECT_EQ(123000322, destProp.asInt());

		cmd_->set(raco::core::ValueHandle{srcLua, {"uri"}}, (test_path() / "lua_scripts" / "dummyAllTypes_NoInteger.lua").string());
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::BROKEN, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		playOneFrame();
		EXPECT_EQ(123000322, destProp.asInt());
		const auto& log{player_->getLog()};
		ASSERT_EQ(log.size(), 1);
		ASSERT_NE(log.find("Can not set linked property! ( propPath: saInfo->ACC->state )"), log.cend());

		cmd_->removeLink(destProp.getDescriptor());
		increaseTimeAndDoOneLoop();
		EXPECT_EQ(raco::core::Queries::CurrentLinkState::NOT_LINKED, raco::core::Queries::linkState(*cmd_->project(), destProp).current);
		playOneFrame();
		EXPECT_NE(123000322, destProp.asInt());
		ASSERT_EQ(log.size(), 0);
		ASSERT_EQ(log.find("Can not set linked property! ( propPath: saInfo->ACC->state )"), log.cend());
	}
}

TEST_F(TracePlayerTest, TF91_Duplicate_Lua) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	createLua("saInfo", LuaType::LuaScript);

	player_->play();
	playOneFrame();

	isFaulty();
}

TEST_F(TracePlayerTest, TF92_TracePlayerData_Lua_Deletion) {
	loadTrace("raco_traces/valid_20211123.rctrace");
	isStopped();

	deleteLuaObj(findLua("TracePlayerData"));

	player_->play();
	playOneFrame();

	ASSERT_EQ(TracePlayer::PlayerState::Playing, player_->getState());
	EXPECT_EQ(true, areAllLuaObjsLocked());
}

TEST_F(TracePlayerTest, TF93_Move_Lua_Under_Prefab) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	cmd_->moveScenegraphChildren({findLua("saInfo")}, cmd_->createObject(raco::user_types::Prefab::typeDescription.typeName, "TestPrefab"));

	playOneFrame();

	isFaulty();
}

TEST_F(TracePlayerTest, TF94_Play_DeleteLua_Recreate_Resume) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	deleteLuaObj(findLua("saInfo"));
	createLua("saInfo", LuaType::LuaScript);

	playOneFrame();

	isFaulty();
}

TEST_F(TracePlayerTest, TF95_Play_DeleteLua_Undo_Play_Redo__FAILED) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	deleteLuaObj(findLua("saInfo"));

	cmd_->undoStack().undo();

	player_->play();
	playOneFrame();

	isFaulty();

	cmd_->undoStack().redo();

	playOneFrame();

	isFaulty();
}

TEST_F(TracePlayerTest, TF96_Lock_ReadOnly_NestedPrefabInstanceLua) {
	loadTrace("raco_traces/valid_20211123.rctrace");

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	deleteLuaObj(findLua("saInfo"));

	const auto parentPrefab{cmd_->createObject(raco::user_types::Prefab::typeDescription.typeName)};
	const auto childPrefab{cmd_->createObject(raco::user_types::Prefab::typeDescription.typeName)};
	createLua("saInfo", LuaType::LuaScript, true, childPrefab);

	const auto prefabInstance_parentPrefab{cmd_->createObject(raco::user_types::PrefabInstance::typeDescription.typeName, std::string(), parentPrefab)};
	cmd_->set(raco::core::ValueHandle{prefabInstance_parentPrefab, &raco::user_types::PrefabInstance::template_}, childPrefab);
	const auto prefabInstance_sceneGraph{cmd_->createObject(raco::user_types::PrefabInstance::typeDescription.typeName)};
	cmd_->set(raco::core::ValueHandle{prefabInstance_sceneGraph, &raco::user_types::PrefabInstance::template_}, parentPrefab);

	playOneFrame();

	isFaulty();
}

TEST_F(TracePlayerTest, TF101_Undo_NoEffect_WhilePaused) {
	const auto qjTrace{loadTrace("raco_traces/valid_20211123.rctrace")};

	player_->play();
	/// run playback and pause midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		increaseTimeAndDoOneLoop();
	}
	player_->pause();

	cmd_->createObject(raco::user_types::LuaScript::typeDescription.typeName, "dummyLua");
	cmd_->undoStack().undo();

	increaseTimeAndDoOneLoop();

	isPaused();
}

TEST_F(TracePlayerTest, TF102_Undo_NoEffect_WhilePlaying) {
	const auto qjTrace{loadTrace("raco_traces/valid_20211123.rctrace")};

	player_->play();
	/// run playback till midway
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		increaseTimeAndDoOneLoop();
	}

	cmd_->createObject(raco::user_types::LuaScript::typeDescription.typeName, "dummyLua");
	cmd_->undoStack().undo();

	playOneFrame();

	isPlaying();
}

TEST_F(TracePlayerTest, TF103_Check_Properties_Consistency) {
	deleteLuaObj(findLua("saInfo"));
	const auto luaSceneControls{createLua("SceneControls", LuaType::LuaScript)};
	const auto luaModule{cmd_->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m")};
	cmd_->set({luaModule, &raco::user_types::LuaScriptModule::uri_}, test_path().append("lua_scripts/modules/anim_utils.lua").string());
	cmd_->set({luaSceneControls, {"luaModules", "anim_utils"}}, luaModule);

	loadTrace("raco_traces/g05_demo.rctrace");
	isStopped();

	player_->play();
	/// run playback till midway
	while (player_->getIndex() < 11) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	player_->jumpTo(8);
	increaseTimeAndDoOneLoop();

	EXPECT_EQ(true, luaSceneControls->get("inputs")->asTable().get("Door_F_L_isOpen")->asBool());
	EXPECT_EQ(true, luaSceneControls->get("inputs")->asTable().get("Door_F_R_isOpen")->asBool());
	EXPECT_EQ(true, luaSceneControls->get("inputs")->asTable().get("Door_B_L_isOpen")->asBool());
	EXPECT_EQ(false, luaSceneControls->get("inputs")->asTable().get("Door_B_R_isOpen")->asBool());

	/// resume playback till Tailgate_isOpen is true, which occurs at frame index 15
	player_->play();
	while (!luaSceneControls->get("inputs")->asTable().get("Tailgate_isOpen")->asBool()) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}
	EXPECT_EQ(15, player_->getIndex());
}

TEST_F(TracePlayerTest, TF104_Disregarded_Properties) {
	deleteLuaObj(findLua("saInfo"));
	const auto luaSceneControls{createLua("SceneControls", LuaType::LuaScript)};
	const auto luaModule{cmd_->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m")};
	cmd_->set({luaModule, &raco::user_types::LuaScriptModule::uri_}, test_path().append("lua_scripts/modules/anim_utils.lua").string());
	cmd_->set({luaSceneControls, {"luaModules", "anim_utils"}}, luaModule);

	{
		loadTrace("raco_traces/g05_demo.rctrace");
		for (const auto& msg : player_->getLog()) {
			ASSERT_EQ(msg.first.find("disregarded from trace"), std::string::npos);
		}
		isStopped();
	}

	{
		loadTrace("raco_traces/g05_demo_withExtras.rctrace");
		const auto& log{player_->getLog()};
		ASSERT_EQ(log.size(), 4);
		ASSERT_NE(log.find("Step 0, Timestamp 1000, Trace line 12: Lua property was not found in the scene! Property is disregarded from trace ( propName: SceneControls->extra_property )"), log.cend());
		ASSERT_NE(log.find("Step 4, Timestamp 1400, Trace line 55: Lua is not available in the scene! Lua and its properties are disregarded from trace ( luaObjName: ExtraLuaScript )"), log.cend());
		ASSERT_NE(log.find("Step 21, Timestamp 3100, Trace line 240: Lua property was not found in the scene! Property is disregarded from trace ( propName: SceneControls->extra_property )"), log.cend());
		ASSERT_NE(log.find("Step 22, Timestamp 3200, Trace line 251: Lua property was not found in the scene! Property is disregarded from trace ( propName: SceneControls->Extra_Struct )"), log.cend());
		isStopped();
	}

	/***  Check if disregarded properties are ignored ***/

	/// run playback till midway
	player_->play();
	while (player_->getIndex() < player_->getTraceLen() / 2) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	/// switch Lua to anther one that contains extra properties. We need to cheat here and briefly unlock the editor object.
	///	the same change could happen by changing the underlying file, but that is harder to accomplish right here.
	const auto luaText{raco::utils::file::read((test_path() / "lua_scripts" / "SceneControls_withExtras.lua").string())};
	const auto fileUsedByScript{luaSceneControls->as<raco::user_types::LuaScript>()->uri_.asString()};
	raco::utils::file::write(fileUsedByScript, luaText);

	// Make sure the file system watcher picks up the change
	int maxNumLoops{50};
	while (!raco::core::ValueHandle{luaSceneControls, {"inputs", "extra_property"}} && --maxNumLoops >= 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));  // give the file monitor thread some time.
		QCoreApplication::processEvents();
		increaseTimeAndDoOneLoop();
	}

	ASSERT_GE(maxNumLoops, 0);
	ASSERT_EQ(static_cast<bool>(raco::core::ValueHandle{luaSceneControls, {"inputs", "extra_property"}}), true);

	/// handle to extra_property
	const auto extraPropValHandle{raco::core::ValueHandle{luaSceneControls, {"inputs", "extra_property"}}};
	raco::core::CodeControlledPropertyModifier::setPrimitive(extraPropValHandle, 12345, player_->uiChanges());

	/// handle to Extra_Struct properties
	const auto extraChild1PropValHandle{raco::core::ValueHandle{luaSceneControls, {"inputs", "Extra_Struct", "extra_child1"}}};
	const auto extraChild2PropValHandle{raco::core::ValueHandle{luaSceneControls, {"inputs", "Extra_Struct", "extra_child2"}}};
	raco::core::CodeControlledPropertyModifier::setPrimitive(extraChild1PropValHandle, true, player_->uiChanges());
	raco::core::CodeControlledPropertyModifier::setPrimitive(extraChild2PropValHandle, 54321, player_->uiChanges());

	/// continue playback till end of trace
	while (player_->getIndex() < player_->getTraceLen() - 1) {
		isPlaying();
		increaseTimeAndDoOneLoop();
	}

	/**
	 * Now, validate that "extra_property" isn't equal to the last value in the trace, which is 57468
	 * It should instead be equal to 12345, which was manually set in this test-case
	 * Then same thing to Extra_Struct properties
	 */
	EXPECT_EQ(12345, extraPropValHandle.asInt());
	EXPECT_EQ(true, extraChild1PropValHandle.asBool());
	EXPECT_EQ(54321, extraChild2PropValHandle.asInt());
}

TEST_F(TracePlayerTest, TF105_ArrayOfDifferentTypes) {
	deleteLuaObj(findLua("saInfo"));

	const auto luaSceneControls{createLua("SceneControls_withArrays", LuaType::LuaScript)};
	const auto luaModule{cmd_->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m")};
	cmd_->set({luaModule, &raco::user_types::LuaScriptModule::uri_}, test_path().append("lua_scripts/modules/anim_utils.lua").string());
	cmd_->set({luaSceneControls, {"luaModules", "anim_utils"}}, luaModule);

	loadTrace("raco_traces/g05_demo_Arrays.rctrace");
	player_->jumpTo(3);
	increaseTimeAndDoOneLoop();

	/// validate ArrayOfFloats array
	{
		EXPECT_EQ(7.7, luaSceneControls->get("inputs")->asTable().get("ArrayOfFloats")->asTable()[0]->asDouble());
		EXPECT_EQ(5.5, luaSceneControls->get("inputs")->asTable().get("ArrayOfFloats")->asTable()[1]->asDouble());
	}

	/// vaildate ComplexStruct array of complex struct
	{
		EXPECT_EQ(1, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[0]->asTable().get("subArr")->asTable()[0]->asTable().get("prop1")->asInt());
		EXPECT_EQ(false, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[0]->asTable().get("subArr")->asTable()[0]->asTable().get("prop2")->asBool());
		EXPECT_EQ(2, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[0]->asTable().get("subArr")->asTable()[1]->asTable().get("prop1")->asInt());
		EXPECT_EQ(true, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[0]->asTable().get("subArr")->asTable()[1]->asTable().get("prop2")->asBool());
		EXPECT_EQ(123.456, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[0]->asTable().get("prop3")->asDouble());

		EXPECT_EQ(11, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[1]->asTable().get("subArr")->asTable()[0]->asTable().get("prop1")->asInt());
		EXPECT_EQ(false, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[1]->asTable().get("subArr")->asTable()[0]->asTable().get("prop2")->asBool());
		EXPECT_EQ(22, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[1]->asTable().get("subArr")->asTable()[1]->asTable().get("prop1")->asInt());
		EXPECT_EQ(true, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[1]->asTable().get("subArr")->asTable()[1]->asTable().get("prop2")->asBool());
		EXPECT_EQ(789.101, luaSceneControls->get("inputs")->asTable().get("ComplexStruct")->asTable()[1]->asTable().get("prop3")->asDouble());
	}
}

TEST_F(TracePlayerTest, TF106_ConsistentWithUndo) {
	const auto luaSceneControls{createLua("SceneControls", LuaType::LuaScript)};
	const auto luaModule{cmd_->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m")};
	cmd_->set({luaModule, &raco::user_types::LuaScriptModule::uri_}, test_path().append("lua_scripts/modules/anim_utils.lua").string());
	cmd_->set({luaSceneControls, {"luaModules", "anim_utils"}}, luaModule);

	const auto luaObjsBackup{backupSceneLuaObjs(loadTrace("raco_traces/g05_demo_withDummy.rctrace"))};

	deleteLuaObj(findLua("SceneControls"));

	player_->play();
	/// run playback till midway
	while (player_->getIndex() < player_->getIndex() / 2) {
		increaseTimeAndDoOneLoop();
	}
	player_->pause();

	/// this should undo the deletion of SceneControls
	cmd_->undoStack().undo();

	player_->play();
	playOneFrame();
	player_->stop();

	EXPECT_EQ(true, areLuaObjsRestored(luaObjsBackup));
}

TEST_F(TracePlayerTest, TF107_ReloadResets) {
	deleteLuaObj(findLua("saInfo"));

	const auto lua{createLua("Interface_ChargingSlider", LuaType::LuaScript)};

	loadTrace("raco_traces/ChargingSlider_Colors_Test.rctrace");
	isStopped();

	const auto C1 = raco::core::ValueHandle{lua, {"inputs", "SystemColors", "C1"}};
	const auto C2 = raco::core::ValueHandle{lua, {"inputs", "SystemColors", "C2"}};
	const auto C3 = raco::core::ValueHandle{lua, {"inputs", "SystemColors", "C3"}};

	EXPECT_EQ(0.0, C1.asVec3f().x.asDouble());
	EXPECT_EQ(0.0, C1.asVec3f().y.asDouble());
	EXPECT_EQ(0.0, C1.asVec3f().z.asDouble());

	EXPECT_EQ(0.0, C2.asVec3f().x.asDouble());
	EXPECT_EQ(0.0, C2.asVec3f().y.asDouble());
	EXPECT_EQ(0.0, C2.asVec3f().z.asDouble());

	EXPECT_EQ(0.0, C3.asVec3f().x.asDouble());
	EXPECT_EQ(0.0, C3.asVec3f().y.asDouble());
	EXPECT_EQ(0.0, C3.asVec3f().z.asDouble());

	/// play first 4 frames
	for (size_t i = 0; i < 4; i++) {
		player_->step(1);
		increaseTimeAndDoOneLoop();
	}

	EXPECT_EQ(0.71373, C1.asVec3f().x.asDouble());
	EXPECT_EQ(0.96863, C1.asVec3f().y.asDouble());
	EXPECT_EQ(1.0, C1.asVec3f().z.asDouble());

	EXPECT_EQ(0.27843, C2.asVec3f().x.asDouble());
	EXPECT_EQ(0.92549, C2.asVec3f().y.asDouble());
	EXPECT_EQ(1.0, C2.asVec3f().z.asDouble());

	EXPECT_EQ(1.0, C3.asVec3f().x.asDouble());
	EXPECT_EQ(0.0, C3.asVec3f().y.asDouble());
	EXPECT_EQ(0.0, C3.asVec3f().z.asDouble());

	/// load some different trace but with same Lua interface
	loadTrace("raco_traces/ChargingSlider_Functions_Test.rctrace");
	isStopped();

	// validate reset of C1, C2, and C3 vectors, as a new trace is loaded, which doesn't modify them
	player_->play();
	while (player_->getIndex() < player_->getTraceLen() - 1) {
		increaseTimeAndDoOneLoop();
		EXPECT_EQ(0.0, C1.asVec3f().x.asDouble());
		EXPECT_EQ(0.0, C1.asVec3f().y.asDouble());
		EXPECT_EQ(0.0, C1.asVec3f().z.asDouble());

		EXPECT_EQ(0.0, C2.asVec3f().x.asDouble());
		EXPECT_EQ(0.0, C2.asVec3f().y.asDouble());
		EXPECT_EQ(0.0, C2.asVec3f().z.asDouble());

		EXPECT_EQ(0.0, C3.asVec3f().x.asDouble());
		EXPECT_EQ(0.0, C3.asVec3f().y.asDouble());
		EXPECT_EQ(0.0, C3.asVec3f().z.asDouble());
	}
}