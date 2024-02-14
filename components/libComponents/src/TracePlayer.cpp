/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/TracePlayer.h"

#include "core/CodeControlledPropertyModifier.h"
#include "core/CoreAnnotations.h"
#include "core/EditorObject.h"
#include "core/ErrorItem.h"
#include "core/Handles.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "log_system/log.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

#include <algorithm>
#include <utility>

namespace raco::components {
class TracePlayer::CodeControlledObjectExtension {
public:
	CodeControlledObjectExtension(core::Project& project, core::DataChangeRecorder& uiChanges, core::UndoStack& undoStack) : project_(project), uiChanges_(uiChanges), undoStack_(undoStack) {}
	/**
	 * @note Marking an editor object as "code controlled" makes it read-only for the user and it becomes possible to call
	 * the setCodeControlledValueHandle functions which allow changing the objects without changing the undo stack.
	 * Once a set of objects is code controlled, it is an error to call lockCodeControlledObjects until the set of objects
	 * has been released with unlockCodeControlledObjects().
	 * When the objects are released, their values are restored to the current state on the undo stack -
	 * that is any changes done via setCodeControlledValueHandle are reverted.
	 * If a code controlled object is deleted, it is automatically released.
	 */
	void lockCodeControlledObjects(core::SEditorObjectSet const& editorObjs) {
		project_.lockCodeCtrldObjs(editorObjs);
		for (auto const& object : editorObjs) {
			for (size_t index = 0; index < object->size(); index++) {
				uiChanges_.recordValueChanged(core::ValueHandle(object, {object->name(index)}));
			}
		}
		codeCtrldObjs_ = editorObjs;
	}

	void unlockCodeControlledObjects() {
		const auto unlockedObjs{project_.unlockCodeCtrldObjs(codeCtrldObjs_)};
		codeCtrldObjs_.clear();

		for (auto const& editorObj : unlockedObjs) {
			if (editorObj) {
				for (size_t index = 0; index < editorObj->size(); index++) {
					uiChanges_.recordValueChanged(core::ValueHandle(editorObj, {editorObj->name(index)}));
				}
			}
		}
		// Restore the previously locked objects to the state they had before they were locked.
		undoStack_.setIndex(undoStack_.getIndex(), true);
	}

	/**
	 * @note setCodeControlledValueHandle() can only be called on locked objects and
	 * needs to be cyclically called for locked objects to keep all controlled
	 * properties persistent; a user Undo will overwrite properties values.
	 */
	template <typename T>
	void setCodeControlledValueHandle(core::ValueHandle const& handle, T const& value);

	core::Project const& project() const {
		return project_;
	}

	core::DataChangeRecorder& uiChanges() const {
		return uiChanges_;
	}

private:
	core::Project& project_;
	core::DataChangeRecorder& uiChanges_;
	core::UndoStack& undoStack_;
	core::SEditorObjectSet codeCtrldObjs_;
};

template <typename T>
void TracePlayer::CodeControlledObjectExtension::setCodeControlledValueHandle(core::ValueHandle const& handle, T const& value) {
	if (!handle) {
		return;
	}
	if (!project_.isCodeCtrldObj(handle.rootObject())) {
		assert(false && "Trying to update a Lua which is no more uncontrolled by TracePlayer!");
		return;
	}
	core::CodeControlledPropertyModifier::setPrimitive(handle, value, uiChanges_);
}

core::DataChangeRecorder& TracePlayer::uiChanges() const {
	return racoCoreInterface_->uiChanges();
}

constexpr auto CriticalToString(core::ErrorLevel e) {
	switch (e) {
		case core::ErrorLevel::INFORMATION:
			return "INFO";
		case core::ErrorLevel::WARNING:
			return "WARNING";
		case core::ErrorLevel::ERROR:
			return "ERROR";
		default:
			assert(false && "Undefined Criticality Level!");
			return "Undefined Criticality Level!";
	}
}

TracePlayer::TracePlayer(core::Project& project, core::DataChangeRecorder& uiChanges, core::UndoStack& undoStack) : racoCoreInterface_{std::make_unique<CodeControlledObjectExtension>(project, uiChanges, undoStack)}, highestCriticality_{core::ErrorLevel::NONE} {
}

TracePlayer::~TracePlayer() = default;

void TracePlayer::setCallbacks(
	const OnStateChangeCallback& onStateChange,
	const OnLuaUpdateCallback& onLuaUpdate,
	const OnLogUpdateCallback& onLogChange) {
	onStateChange_ = onStateChange;
	onLuaUpdate_ = onLuaUpdate;
	onLogChange_ = onLogChange;
}

/// @todo when it goes into Faulty after at one successful load, then user Clear errors, make sure you update the path with the previous trace again.
/// @todo pass faulty error messages to failSafe
/// @todo switch to fmt::format using #include <spdlog/fmt/fmt.h>
QJsonArray const* const TracePlayer::loadTrace(const std::string& fileName) {
	/// validate not empty file path
	if (fileName.empty()) {
		addError("Could not open file! File path is empty.", core::ErrorLevel::ERROR);
		failSafe();
		return nullptr;
	}

	/// validate extension
	const auto FILE_EXTENSION{".rctrace"};
	if (fileName.compare(fileName.length() - std::string(FILE_EXTENSION).length(), std::string(FILE_EXTENSION).length(), FILE_EXTENSION)) {
		addError("Could not open file! Invalid file extension, expected .rctrace ( filePath: " + fileName + " )", core::ErrorLevel::ERROR);
		failSafe();
		return nullptr;
	}

	/// open trace file and validate its format
	QFile qTraceFile(QString::fromStdString(fileName));
	if (!qTraceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		addError("Could not open file! Invalid file format ( filePath: " + fileName + " )", core::ErrorLevel::ERROR);
		failSafe();
		return nullptr;
	}

	/// prepare text stream to extract line numbers
	const QByteArray jsonByteArray = qTraceFile.readAll();
	QString jsonString(jsonByteArray);
	textStream_.setString(&jsonString);
	keyLineNumber_ = 0;

	/// parse trace QFile
	QJsonParseError qjParseError{};
	const auto qjDocument{QJsonDocument::fromJson(jsonByteArray, &qjParseError)};
	qTraceFile.close();

	/// validate trace QJsonDocument format
	if (qjParseError.error) {
		qjParseErrMsg(qjParseError, fileName);
		failSafe();
		return nullptr;
	}

	/// validate root type
	if (!qjDocument.isArray()) {
		addError("Invalid trace file! Root member must be an JSON array representing the list of frames. For more details, refer to Ramses Composer documentation. ( filePath: " + fileName + " )", core::ErrorLevel::ERROR);
		if (qjParseError.error) {
			qjParseErrMsg(qjParseError, fileName);
		}
		failSafe();
		return nullptr;
	}

	/// parse root JSON array
	qjRoot_ = std::make_unique<QJsonArray>(qjDocument.array());

	/// initialize traceplayer
	setState(PlayerState::Init);
	filePath_ = fileName;
	speed_ = 1.0;
	framesTsList_.clear();
	clearLog();
	stop();

	/// match scene Lua objects and latch properties if they are missing in consecutive frames
	makeFramesConsistent();

	return qjRoot_.get();
}

void TracePlayer::qjParseErrMsg(const QJsonParseError& qjParseError, const std::string& fileName) {
	std::string errorMsg{"Invalid trace file >> Parsing error! "};
	switch (qjParseError.error) {
		case QJsonParseError::UnterminatedObject:
			errorMsg += "An object is not correctly terminated with a closing curly bracket";
			break;
		case QJsonParseError::MissingNameSeparator:
			errorMsg += "A comma separating different items is missing";
			break;
		case QJsonParseError::UnterminatedArray:
			errorMsg += "The array is not correctly terminated with a closing square bracket";
			break;
		case QJsonParseError::MissingValueSeparator:
			errorMsg += "A colon separating keys from values inside objects is missing";
			break;
		case QJsonParseError::IllegalValue:
			errorMsg += "The value is illegal";
			break;
		case QJsonParseError::TerminationByNumber:
			errorMsg += "The input stream ended while parsing a number";
			break;
		case QJsonParseError::IllegalNumber:
			errorMsg += "The number is not well formed";
			break;
		case QJsonParseError::IllegalEscapeSequence:
			errorMsg += "An illegal escape sequence occurred in the input";
			break;
		case QJsonParseError::IllegalUTF8String:
			errorMsg += "An illegal UTF8 sequence occurred in the input";
			break;
		case QJsonParseError::UnterminatedString:
			errorMsg += "A string wasn't terminated with a quote";
			break;
		case QJsonParseError::MissingObject:
			errorMsg += "An object was expected but couldn't be found";
			break;
		case QJsonParseError::DeepNesting:
			errorMsg += "The JSON document is too deeply nested for the parser to parse it";
			break;
		case QJsonParseError::DocumentTooLarge:
			errorMsg += "The JSON document is too large for the parser to parse it";
			break;
		case QJsonParseError::GarbageAtEnd:
			errorMsg += "The parsed document contains additional garbage characters at the end";
			break;
		default:
			errorMsg += "Undefined parse error";
			break;
	}
	errorMsg += " ( filePath: " + fileName + " >> offset: " + std::to_string(qjParseError.offset) + " )";
	addError(errorMsg, core::ErrorLevel::ERROR);
}

QJsonValue TracePlayer::deepAddMissingProperties(const QJsonValue& qjPrev, const QJsonValue& qjCurrent, std::vector<std::string>& propertyPath, int index) {
	if (qjCurrent.type() == QJsonValue::Object) {
		auto qjCurrObj{qjCurrent.toObject()};
		const auto qjPrevObj{qjPrev.toObject()};

		/// add missing properties to current frame
		for (auto qjPrevItr{qjPrevObj.constBegin()}; qjPrevItr != qjPrevObj.constEnd(); ++qjPrevItr) {
			const auto propName{qjPrevItr.key()};
			const auto qjCurrItr{qjCurrObj.find(propName)};
			propertyPath.push_back(propName.toStdString());
			if (qjCurrItr != qjCurrObj.end()) {
				qjCurrObj[qjCurrItr.key()] = deepAddMissingProperties(qjPrevItr.value(), qjCurrItr.value(), propertyPath, index);
			} else {
				qjCurrObj.insert(propName, qjPrevItr.value());
			}
			propertyPath.pop_back();
		}

		/// delete properties which are not in the scene
		auto qjCurrItr{qjCurrObj.begin()};
		while (qjCurrItr != qjCurrObj.end()) {
			if (qjPrevObj.find(qjCurrItr.key()) == qjPrevObj.constEnd()) {
				const auto propPathStream{streamKeysChain(propertyPath)};
				const auto timestamp = parseTimestamp(parseTracePlayerData(parseFrame(index)));
				const int lineNumber = getPropertyLineNumber(qjCurrItr.key());
				if (propertyPath.empty()) {
					addError("Step " + std::to_string(index) + ", Timestamp " + std::to_string(timestamp) + ", Trace line " + std::to_string(lineNumber) + ": Lua is not available in the scene! Lua and its properties are disregarded from trace ( luaObjName: " + propPathStream + qjCurrItr.key().toStdString() + " )", core::ErrorLevel::WARNING, false);
				} else if (!qjCurrItr->isUndefined() && !qjCurrItr->isNull()) {
					addError("Step " + std::to_string(index) + ", Timestamp " + std::to_string(timestamp) + ", Trace line " + std::to_string(lineNumber) + ": Lua property was not found in the scene! Property is disregarded from trace ( propName: " + propPathStream + "->" + qjCurrItr.key().toStdString() + " )", core::ErrorLevel::WARNING, false);
				} else {
					addError("Step " + std::to_string(index) + ", Timestamp " + std::to_string(timestamp) + ", Trace line " + std::to_string(lineNumber) + ": Unexpected JSON type! ( propName: " + qjCurrItr.key().toStdString() + " )", core::ErrorLevel::ERROR, false);
				}
				qjCurrItr = qjCurrObj.erase(qjCurrItr);
			} else {
				++qjCurrItr;
			}
		}

		return qjCurrObj;
	} else {
		return qjCurrent;
	}
}

void TracePlayer::makeFramesConsistent() {
	std::unordered_set<core::SEditorObject> sceneLuaList;
	for (int frameIndex{0}; frameIndex < getTraceLen(); ++frameIndex) {
		const auto qjSceneData{parseSceneData(parseFrame(frameIndex))};
		for (auto itr{qjSceneData.constBegin()}; itr != qjSceneData.constEnd(); ++itr) {
			if (const auto lua{findLua(itr.key().toStdString(), false)}) {
				sceneLuaList.emplace(lua);
			}
		}
		framesTsList_.emplace_back(parseTimestamp(parseTracePlayerData(parseFrame(frameIndex))));
	}

	if (sceneLuaList.empty()) {
		return;
	}

	rebuildFrameSceneData(0, buildFullFrameFromLua(sceneLuaList), parseSceneData(parseFrame(0)));
	for (int frameIndex{1}; frameIndex < getTraceLen(); ++frameIndex) {
		rebuildFrameSceneData(frameIndex, parseSceneData(parseFrame(frameIndex - 1)), parseSceneData(parseFrame(frameIndex)));
	}

	if (onLogChange_) {
		onLogChange_(logReport_, highestCriticality_);
	}
	traceFileLines_.clear();
}

void TracePlayer::rebuildFrameSceneData(int index, const QJsonValue& qjPrev, const QJsonValue& qjCurr) {
	auto qjFrameObj{parseFrame(index)};
	std::vector<std::string> propertyPath;
	readLinesForNextFrame();
	qjFrameObj["SceneData"] = deepAddMissingProperties(qjPrev, qjCurr, propertyPath, index);
	(*qjRoot_)[index] = qjFrameObj;
}

void TracePlayer::readLinesForNextFrame() {
	auto line = textStream_.readLine();

	traceFileLines_.push_front({0, line});
	keyLineNumber_++;

	while (!textStream_.atEnd()) {
		line = textStream_.readLine();
		keyLineNumber_++;
		traceFileLines_.push_front({keyLineNumber_, line});
		if (line.contains("TracePlayerData")) {
			break;
		}
	}
}

int TracePlayer::getPropertyLineNumber(const QString& propertyKey) {
	int propertyLine = -1;
	auto itToErase = traceFileLines_.before_begin();

	for (auto it = traceFileLines_.begin(); it != traceFileLines_.end(); ++it, ++itToErase) {
		if (it->second.contains("\"" + propertyKey + "\"")) {
			propertyLine = it->first;
			traceFileLines_.erase_after(itToErase);
			break;
		}
	}
	return propertyLine;
}

QJsonValue TracePlayer::deepCopyFromLua(core::ValueHandle const& luaValHandle) {
	switch (luaValHandle.type()) {
		case data_storage::PrimitiveType::Struct:
		case data_storage::PrimitiveType::Table: {
			/// check whether it's a JSON array or object
			const bool isArray{luaValHandle.query<core::ArraySemanticAnnotation>()};
			if (isArray) {
				QJsonArray qjPropertyArr;
				for (int arrElmntIndx{0}; arrElmntIndx < luaValHandle.size(); ++arrElmntIndx) {
					qjPropertyArr.insert(arrElmntIndx, deepCopyFromLua(luaValHandle[arrElmntIndx]));
				}
				return qjPropertyArr;
			} else {
				QJsonObject qjPropertyObj;
				for (int i{0}; i < luaValHandle.size(); ++i) {
					const auto childPropertyValHandle = luaValHandle[i];
					const auto childPropName = childPropertyValHandle.getPropName();
					qjPropertyObj.insert(childPropName.c_str(), deepCopyFromLua(childPropertyValHandle));
				}
				return qjPropertyObj;
			}
		}

		case data_storage::PrimitiveType::Bool:
			return luaValHandle.asBool();

		case data_storage::PrimitiveType::Int:
			return luaValHandle.asInt();

		case data_storage::PrimitiveType::Int64:
			return static_cast<qint64>(luaValHandle.asInt64());

		case data_storage::PrimitiveType::Double:
			return luaValHandle.asDouble();

		case data_storage::PrimitiveType::String:
			return luaValHandle.asString().c_str();

		default:
			assert("Unknown property type!" && false);
			return {};
	}
}

QJsonValue TracePlayer::buildFullFrameFromLua(std::unordered_set<core::SEditorObject> const& sceneLuaList) {
	QJsonObject fullFrame;
	for (const auto& lua : sceneLuaList) {
		const auto luaFullFrame{deepCopyFromLua(core::ValueHandle(lua, {"inputs"}))};

		if (luaFullFrame.isNull() || luaFullFrame.isUndefined()) {
			assert("Something went wrong!" && false);
		} else {
			fullFrame.insert(lua->objectName().c_str(), luaFullFrame);
		}
	}
	return fullFrame;
}

std::string const& TracePlayer::getFilePath() const {
	return filePath_;
}

int TracePlayer::getTraceLen() const {
	if (!qjRoot_) {
		return -1;
	}
	return qjRoot_->size();
}

bool TracePlayer::isLastFrame() const {
	return (playbackIndex_ == getTraceLen() - 1);
}

TracePlayer::PlayerState TracePlayer::getState() const {
	return state_;
}

int TracePlayer::getIndex() const {
	return playbackIndex_;
}

int TracePlayer::getTimestamp() const {
	return playbackTs_;
}

auto TracePlayer::getNextTs() -> timeInMilliSeconds {
	const auto nextIndex{playbackIndex_ + 1};
	return parseTimestamp(parseTracePlayerData(parseFrame(nextIndex)));
}

bool TracePlayer::parseFrameAndUpdateLua() {
	const auto qjFrame{parseFrame(playbackIndex_)};
	if (qjFrame.isEmpty()) {
		return false;
	}

	const auto qjSceneData{parseSceneData(qjFrame, playbackIndex_)};
	if (qjSceneData.isEmpty()) {
		return false;
	}

	const auto qjTracePlayerData{parseTracePlayerData(qjFrame, playbackIndex_)};
	if (qjTracePlayerData.isEmpty()) {
		return false;
	}

	const auto timestamp{parseTimestamp(qjTracePlayerData, playbackIndex_)};
	if (timestamp > 0) {
		playbackTs_ = timestamp;
	} else {
		addError("Invalid timestamp! ( frameNr: " + std::to_string(playbackIndex_) + " )", core::ErrorLevel::ERROR);
		return false;
	}

	/// iterate over all scripts/features of the frame
	int validLuaCount{0};
	for (auto itr{qjSceneData.constBegin()}; itr != qjSceneData.constEnd(); ++itr) {
		if (const auto lua{findLua(itr.key().toStdString())}) {
			if (!racoCoreInterface_->project().isCodeCtrldObj(lua)) {
				addError("Lua was unlocked during playback! ( luaObjName: " + lua->objectName() + " )", core::ErrorLevel::ERROR);
				return false;
			}
			++validLuaCount;
			std::vector<std::string> keysChain{"inputs"};
			updateLua(itr.value(), keysChain, lua);
			if (onLuaUpdate_) {
				onLuaUpdate_(playbackIndex_);
			}
		}
	}

	/// pause playback and log error if no Lua script is available
	if (!validLuaCount) {
		addError("No Lua script from trace was found in the scene!", core::ErrorLevel::ERROR);
		return false;
	}

	return true;
}

QJsonObject TracePlayer::parseFrame(int frameIndex) {
	if (frameIndex < 0 || frameIndex >= getTraceLen()) {
		/// handle out of range jumps
		addError("Frame index went out of range! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	/// parse and validate current frame
	const auto qjFrameVal{qjRoot_->at(frameIndex)};
	if (qjFrameVal == QJsonValue::Undefined) {
		addError("Frame entry was not found! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	const QJsonObject qjFrame{qjFrameVal.toObject()};
	if (qjFrame.isEmpty()) {
		addError("Invalid frame >> Empty frame! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	return qjFrame;
}

QJsonObject TracePlayer::parseSceneData(const QJsonObject& qjFrame, int frameIndex) {
	/// parse and validate SceneData
	const auto qjSceneDataVal{qjFrame.value("SceneData")};
	if (qjSceneDataVal == QJsonValue::Undefined && frameIndex != -1) {
		addError("Invalid frame >> SceneData not found! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	const auto qjSceneData{qjSceneDataVal.toObject()};
	if (qjSceneData.isEmpty() && frameIndex != -1) {
		addError("Invalid frame >> Empty SceneData! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	return qjSceneData;
}

QJsonObject TracePlayer::parseTracePlayerData(const QJsonObject& qjFrame, int frameIndex) {
	if (qjFrame.isEmpty()) {
		return QJsonObject();
	}

	/// parse and validate TracePlayerData
	const auto qjTracePlayerDataVal{qjFrame.value("TracePlayerData")};
	if (qjTracePlayerDataVal == QJsonValue::Undefined && frameIndex != -1) {
		addError("Invalid frame >> TracePlayerData was not found! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	const QJsonObject qjTracePlayerData{qjTracePlayerDataVal.toObject()};
	if (qjTracePlayerData.isEmpty() && frameIndex != -1) {
		addError("Invalid frame >> Empty TracePlayerData! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return QJsonObject();
	}

	return qjTracePlayerData;
}

int TracePlayer::parseTimestamp(const QJsonObject& qjTracePlayerData, int frameIndex) {
	if (qjTracePlayerData.isEmpty()) {
		return -1;
	}

	/// parse and validate timestamp(ms)
	const QJsonValue qjTimestampVal{qjTracePlayerData.value("timestamp(ms)")};
	if (qjTimestampVal == QJsonValue::Undefined && frameIndex != -1) {
		addError("Invalid frame >> TracePlayerData::timestamp(ms) was not found! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return -1;
	}
	if (qjTimestampVal.type() != QJsonValue::Double && frameIndex != -1) {
		addError("Invalid frame >> TracePlayerData::timestamp(ms) is not a number! ( frameNr: " + std::to_string(frameIndex) + " )", core::ErrorLevel::ERROR);
		return -1;
	}

	return qjTimestampVal.toInt();
}

core::SEditorObject TracePlayer::findLua(const std::string& luaObjName, bool logErrors) {
	/// find matching LuaScript or LuaInterface
	const auto isControllableLua{[&luaObjName](const core::SEditorObject& o) {
		return (
			((o->objectName() == luaObjName) && (!core::PrefabOperations::findContainingPrefab(o))) &&
			((o->isType<user_types::LuaInterface>()) ||
				(o->isType<user_types::LuaScript>() && (!core::PrefabOperations::findContainingPrefabInstance(o)))));
	}};

	const auto& instances{racoCoreInterface_->project().instances()};
	if (const auto itrLuaObj{std::find_if(instances.cbegin(), instances.cend(), isControllableLua)};
		itrLuaObj != instances.cend()) {
		/// make sure we have only one Lua object with that name in the scene
		if (const auto itrDuplicateLua{std::find_if(itrLuaObj + 1, instances.cend(), isControllableLua)};
			itrDuplicateLua != instances.cend()) {
			if (logErrors) {
				addError("Lua object was found more than once in the scene! ( luaObjName: " + luaObjName + " )", core::ErrorLevel::ERROR);
			}
			return nullptr;
		}

		const auto lua{*itrLuaObj};

		if (user_types::Queries::isReadOnly(lua)) {
			if (logErrors) {
				addError("Could not take control of Lua >> Object is read-only! ( luaObjName: " + lua->objectName() + " )", core::ErrorLevel::WARNING);
			}
			return nullptr;
		}
		clearError("Could not take control of Lua >> Object is read-only! ( luaObjName: " + lua->objectName() + " )", core::ErrorLevel::WARNING);

		clearError("Could not find Lua in the scene! ( luaObjName: " + luaObjName + " )", core::ErrorLevel::WARNING);
		return lua;
	}

	if (logErrors) {
		addError("Could not find Lua in the scene! ( luaObjName: " + luaObjName + " )", core::ErrorLevel::WARNING);
	}
	return nullptr;
}

void TracePlayer::updateLua(const QJsonValue& jsonChild, std::vector<std::string>& keysChain, const core::SEditorObject& lua) {
	const std::string keysStream{lua->objectName() + "->" + streamKeysChain(keysChain)};
	switch (jsonChild.type()) {
		case QJsonValue::Null: {
			addError("Invalid JSON value! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
			break;
		}

		case QJsonValue::Bool:
		case QJsonValue::Double:
		case QJsonValue::String: {
			updateLuaProperty(lua, keysChain, jsonChild);
			break;
		}
		case QJsonValue::Array: {
			const auto nestedArr = jsonChild.toArray();
			uint key{1};
			for (auto itr{nestedArr.constBegin()}; itr != nestedArr.constEnd(); ++itr) {
				keysChain.push_back(std::to_string(key));
				updateLua(*itr, keysChain, lua);
				keysChain.pop_back();
				++key;
			}
			break;
		}
		case QJsonValue::Object: {
			const auto nestedObj = jsonChild.toObject();
			for (auto itr{nestedObj.constBegin()}; itr != nestedObj.constEnd(); ++itr) {
				keysChain.push_back(itr.key().toStdString());
				updateLua(itr.value(), keysChain, lua);
				keysChain.pop_back();
			}
			break;
		}
		case QJsonValue::Undefined:
		default:
			/// trying to read an out of bounds value in an array or a non existent key in an object.
			addError("Out of bounds JSON value! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
			break;
	}
}

std::string TracePlayer::streamKeysChain(const std::vector<std::string>& keysChain) const {
	std::string keysStream{};
	int chainIndex{0};
	for (const auto& key : keysChain) {
		++chainIndex;
		if (key == "inputs") {
			continue;
		}
		keysStream.append(key);
		if (chainIndex != keysChain.size()) {
			keysStream.append("->");
		}
	}
	return keysStream;
}

void TracePlayer::updateLuaProperty(const core::SEditorObject& lua, const std::vector<std::string>& keysChain, const QJsonValue& jsonChild, bool logErrors) {
	const std::string keysStream{lua->objectName() + "->" + streamKeysChain(keysChain)};

	if (auto const valHandle{core::ValueHandle{lua, keysChain}}) {
		clearError("Property was not found! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);

		if (core::Queries::linkState(racoCoreInterface_->project(), valHandle).current != core::Queries::CurrentLinkState::NOT_LINKED) {
			if (logErrors) {
				addError("Can not set linked property! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
			}
			return;
		}
		clearError("Can not set linked property! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);

		switch (jsonChild.type()) {
			case QJsonValue::Bool: {
				if (valHandle.type() != data_storage::PrimitiveType::Bool) {
					if (logErrors) {
						addError("Property type mismatch >> Expected a Bool! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
					}
				} else {
					racoCoreInterface_->setCodeControlledValueHandle(valHandle, jsonChild.toBool());
					clearError("Property type mismatch >> Expected a Bool! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
				}
				break;
			}
			case QJsonValue::Double: {
				if (valHandle.type() == data_storage::PrimitiveType::Int) {
					racoCoreInterface_->setCodeControlledValueHandle(valHandle, static_cast<int>(jsonChild.toDouble()));
				} else if (valHandle.type() == data_storage::PrimitiveType::Double) {
					racoCoreInterface_->setCodeControlledValueHandle(valHandle, jsonChild.toDouble());
				} else {
					if (logErrors) {
						addError("Property type mismatch >> Expected a Number! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
					}
					break;
				}
				clearError("Property type mismatch >> Expected a Number! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
				break;
			}
			case QJsonValue::String: {
				if (valHandle.type() != data_storage::PrimitiveType::String) {
					if (logErrors) {
						addError("Property type mismatch >> Expected a String! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
					}
				} else {
					racoCoreInterface_->setCodeControlledValueHandle(valHandle, jsonChild.toString().toStdString());
					clearError("Property type mismatch >> Expected a String! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
				}
				break;
			}
			default:
				if (logErrors) {
					addError("Property type mismatch >> Unexpected data type! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
				}
				break;
		}
	} else {
		if (logErrors) {
			addError("Property was not found! ( propPath: " + keysStream + " )", core::ErrorLevel::WARNING);
		}
	}
}

void TracePlayer::addError(const std::string& msg, core::ErrorLevel level, bool callLogChange) {
	if (tracePlayerLog_.insert(std::make_pair(msg, level)).second) {
		const std::string errLvl = "[" + std::string(CriticalToString(level)) + "] ";
		logReport_.push_back(errLvl + msg);

		switch (level) {
			case core::ErrorLevel::ERROR:
				LOG_ERROR(log_system::TRACE_PLAYER, msg);
				break;
			case core::ErrorLevel::WARNING:
				LOG_WARNING(log_system::TRACE_PLAYER, msg);
				break;
			case core::ErrorLevel::INFORMATION:
				LOG_INFO(log_system::TRACE_PLAYER, msg);
				break;
			default:
				break;
		}

		if (highestCriticality_ < level) {
			highestCriticality_ = level;
		}
		if (onLogChange_ && callLogChange) {
			onLogChange_(logReport_, highestCriticality_);
		}
	}
}

void TracePlayer::clearError(const std::string& msg, core::ErrorLevel level) {
	if (tracePlayerLog_.erase(msg)) {
		const std::string errLvl = "[" + std::string(CriticalToString(level)) + "] ";
		logReport_.erase(std::remove(logReport_.begin(), logReport_.end(), errLvl + msg));
		if (onLogChange_) {
			onLogChange_(logReport_, highestCriticality_);
		}
	}
}

void TracePlayer::clearLog() {
	tracePlayerLog_.clear();
	logReport_.clear();
	highestCriticality_ = core::ErrorLevel::NONE;
	if (onLogChange_) {
		onLogChange_(logReport_, highestCriticality_);
	}
}

std::unordered_map<std::string, core::ErrorLevel> const& TracePlayer::getLog() const {
	return tracePlayerLog_;
}

void TracePlayer::reset() {
	playbackIndex_ = -1;
	playbackTs_ = parseTimestamp(parseTracePlayerData(parseFrame(0)));
	refreshTs_ = playbackTs_;
}

void TracePlayer::refresh(timeInMilliSeconds elapsedTimeSinceStart) {
	const timeInMilliSeconds refreshTime{elapsedTimeSinceStart - timeOfLastRefresh_};
	timeOfLastRefresh_ = elapsedTimeSinceStart;
	if (refreshTime < 0 || (state_ != PlayerState::Playing && state_ != PlayerState::Paused)) {
		return;
	}

	if (state_ == PlayerState::Playing) {
		/// increase refreshTs_ by at least 1 ms
		refreshTs_ += std::max<timeInMilliSeconds>(refreshTime * speed_, 1);
		/// update animation current timeline
		if (const auto lua{findLua("TracePlayerData", false)}; lua != nullptr && racoCoreInterface_->project().isCodeCtrldObj(lua)) {
			updateLuaProperty(lua, std::vector<std::string>{"inputs", "TracePlayerData", "timestamp_milli"}, static_cast<qint64>(refreshTs_), false);
		}

		if (refreshTs_ >= framesTsList_.front()) {
			if (refreshTs_ >= framesTsList_.back()) {
				playbackIndex_ = framesTsList_.size() - 1;
			} else {
				auto itr{std::upper_bound(framesTsList_.cbegin(), framesTsList_.cend(), refreshTs_)};
				--itr;
				playbackIndex_ = itr - framesTsList_.cbegin();
			}
		}
	}

	/// @note overwrite possible changes by a user Undo
	if (parseFrameAndUpdateLua()) {
		if (state_ == PlayerState::Playing) {
			if (isLastFrame()) {
				if (looping_) {
					/// loop playback
					play();
				} else {
					/// pause playback at end of trace
					pause();
					return;
				}
			}
		}
	} else {
		failSafe();
		return;
	}
}

void TracePlayer::lockLua() {
	core::SEditorObjectSet luaObjects;
	for (int frameIndex{0}; frameIndex < getTraceLen(); ++frameIndex) {
		const auto qjSceneData{parseSceneData(parseFrame(frameIndex))};
		for (auto itr{qjSceneData.constBegin()}; itr != qjSceneData.constEnd(); ++itr) {
			if (auto const lua{findLua(itr.key().toStdString())}) {
				if (!racoCoreInterface_->project().isCodeCtrldObj(lua)) {
					if (!user_types::Queries::isReadOnly(lua)) {
						luaObjects.insert(lua);
					} else {
						addError("Could not lock Lua >> Object is read-only! ( luaObjName: " + lua->objectName() + " )", core::ErrorLevel::WARNING);
					}
				} else {
					addError("Could not lock Lua >> Object is already locked! ( luaObjName: " + lua->objectName() + " )", core::ErrorLevel::WARNING);
				}
			}
		}
	}

	if (const auto luaTracePlayerData{findLua("TracePlayerData", false)}) {
		luaObjects.insert(luaTracePlayerData);
	}
	racoCoreInterface_->lockCodeControlledObjects(luaObjects);
}

void TracePlayer::setState(PlayerState newState) {
	state_ = newState;
	if (onStateChange_) {
		onStateChange_(newState);
	}
}

void TracePlayer::play() {
	switch (state_) {
		case PlayerState::Init:
			assert("Invalid transition to Playing state!" && false);
			return;
		case PlayerState::Faulty:
			/// do nothing
			return;
		case PlayerState::Playing:
			/// restart playback
			reset();
			break;
		case PlayerState::Stopped:
			/// store initial state once
			lockLua();
			break;
		case PlayerState::Paused:
			if (isLastFrame()) {
				/// restart playback
				reset();
			} else {
				/// resume playback
			}
			break;
		default:
			assert("Undefined TracePlayer State!" && false);
			break;
	}

	/// start playback
	setState(PlayerState::Playing);
}

void TracePlayer::pause() {
	switch (state_) {
		case PlayerState::Init:
			assert("Invalid transition to Paused state!" && false);
			return;
		case PlayerState::Paused:
		case PlayerState::Stopped:
		case PlayerState::Faulty:
			/// do nothing
			return;
		case PlayerState::Playing:
			/// pause playback
			break;
		default:
			assert("Undefined TracePlayer State!" && false);
			break;
	}

	/// pause playback
	setState(PlayerState::Paused);
}

void TracePlayer::stop() {
	switch (state_) {
		case PlayerState::Stopped:
			/// do nothing
			return;
		case PlayerState::Init:
		case PlayerState::Playing:
		case PlayerState::Paused:
		case PlayerState::Faulty:
			/// stop playback and put TracePlayer in initial state
			break;
		default:
			assert("Undefined TracePlayer State!" && false);
			break;
	}

	/// stop playback
	reset();
	racoCoreInterface_->unlockCodeControlledObjects();
	setState(PlayerState::Stopped);
}

void TracePlayer::jumpTo(int newIndex) {
	switch (state_) {
		case PlayerState::Init:
			assert("Invalid transition to Jumping state!" && false);
			return;
		case PlayerState::Faulty:
			/// do nothing
			return;
		case PlayerState::Stopped:
			lockLua();
			break;
		case PlayerState::Playing:
		case PlayerState::Paused:
			/// jump to newIndex and pause
			break;
		default:
			assert("Undefined TracePlayer State!" && false);
			break;
	}

	/// jump to new index
	playbackIndex_ = newIndex;
	refreshTs_ = parseTimestamp(parseTracePlayerData(parseFrame(playbackIndex_)));

	/// pause playback
	setState(PlayerState::Paused);
}

void TracePlayer::step(int step) {
	auto targetFrameIndex{playbackIndex_ + step};
	if (targetFrameIndex < 0) {
		targetFrameIndex = 0;
	} else if (targetFrameIndex > getTraceLen() - 1) {
		targetFrameIndex = getTraceLen() - 1;
	}
	jumpTo(targetFrameIndex);
}

void TracePlayer::failSafe() {
	setState(PlayerState::Faulty);
}

void TracePlayer::setSpeed(double speed) {
	speed_ = speed;
}

void TracePlayer::toggleLooping() {
	looping_ = !looping_;
}

bool TracePlayer::getLoopingStatus() const {
	return looping_;
}

}  // namespace raco::components
