/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/RaCoApplication.h"

#include "components/RaCoPreferences.h"

#include "utils/u8path.h"

#include "core/Context.h"
#include "core/Handles.h"
#include "components/TracePlayer.h"
#include "core/Context.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/ProjectMigration.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_base/BaseEngineBackend.h"
#include "user_types/Animation.h"

#include "core/Handles.h"

#ifdef OS_WINDOWS
// see: https://doc.qt.io/qt-5/qfileinfo.html#ntfs-permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

namespace raco::application {

RaCoApplicationLaunchSettings::RaCoApplicationLaunchSettings()
	: initialProject{},
	  createDefaultScene{true},
	  enableRamsesTrace{false},
	  newFileFeatureLevel{-1},
	  initialLoadFeatureLevel{-1},
	  runningInUI{false} {
}

RaCoApplicationLaunchSettings::RaCoApplicationLaunchSettings(QString argInitialProject, bool argCreateDefaultScene, bool argEnableRamsesTrace, int argNewFileFeatureLevel, int argInitialLoadFeatureLevel, bool argRunningInUI)
	: initialProject(argInitialProject),
	  createDefaultScene(argCreateDefaultScene),
	  enableRamsesTrace(argEnableRamsesTrace),
	  newFileFeatureLevel(argNewFileFeatureLevel),
	  initialLoadFeatureLevel{argInitialLoadFeatureLevel},
	  runningInUI(argRunningInUI) {
}

RaCoApplication::RaCoApplication(ramses_base::BaseEngineBackend& engine, const RaCoApplicationLaunchSettings& settings)
	: engine_{&engine},
	  newFileFeatureLevel_(settings.newFileFeatureLevel),
	  dataChangeDispatcher_{std::make_shared<components::DataChangeDispatcher>()},
	  dataChangeDispatcherPreviewScene_{std::make_shared<components::DataChangeDispatcher>()},
	  dataChangeDispatcherAbstractScene_{std::make_shared<components::DataChangeDispatcher>()},
	  previewSceneBackend_{new ramses_adaptor::SceneBackend(engine, dataChangeDispatcherPreviewScene_)},
	  externalProjectsStore_(this) {
	ramses_base::installRamsesLogHandler(settings.enableRamsesTrace);
	// Preferences need to be initalized before we have a fist initial project
	components::RaCoPreferences::init();

	runningInUI_ = settings.runningInUI;

	switchActiveRaCoProject(settings.initialProject, {}, settings.createDefaultScene, settings.initialLoadFeatureLevel);
}

RaCoApplication::~RaCoApplication() {
	/* an implicit destructor would force users of RaCoApplication class to include the definition of SceneBackend (due to unique_ptr) */
}

RaCoProject& RaCoApplication::activeRaCoProject() {
	return *activeProject_.get();
}

const RaCoProject& RaCoApplication::activeRaCoProject() const {
	return *activeProject_.get();
}

std::string RaCoApplication::activeProjectPath() const {
	if (activeProject_ && !activeProject_->project()->currentFileName().empty()) {
		return activeProject_->project()->currentPath();
	}
	return std::string();
}

std::string RaCoApplication::activeProjectFolder() const {
	if (activeProject_) {
		return activeProject_->project()->currentFolder();
	}
	return std::string();
}

void RaCoApplication::resetSceneBackend() {
	previewSceneBackend_->reset();
	abstractScene_.reset();
	recordingStats_  = false;
}

class WithRelinkCallback {
public:
	WithRelinkCallback(ExternalProjectsStore& externalProjects, std::function<std::string(const std::string&)> relinkCallback)
		: externalProjectsStore_(externalProjects) {
		externalProjectsStore_.setRelinkCallback(relinkCallback);
	}

	~WithRelinkCallback() {
		externalProjectsStore_.clearRelinkCallback();
	}

	ExternalProjectsStore& externalProjectsStore_;
};

void RaCoApplication::setupScene(bool optimizeForExport, bool setupAbstractScene) {
	auto featureLevel = static_cast<ramses::EFeatureLevel>(activeRaCoProject().project()->featureLevel());

	previewSceneBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors(), optimizeForExport, ramses_adaptor::SceneBackend::toSceneId(*activeRaCoProject().project()->settings()->sceneId_));
	previewSceneBackend_->logicEngine()->enableUpdateReport(recordingStats_ && !optimizeForExport);
	if (runningInUI_) {
		if (setupAbstractScene) {
			abstractScene_.reset();
			abstractScene_ = std::make_unique<ramses_adaptor::AbstractSceneAdaptor>(&engine_->client(), ramses_base::BaseEngineBackend::abstractSceneId(), activeRaCoProject().project(), dataChangeDispatcherAbstractScene_, &meshCache_, previewSceneBackend_->sceneAdaptor());
		} else if (abstractScene_) {
			abstractScene_->setPreviewAdaptor(previewSceneBackend_->sceneAdaptor());
		}
	}
}

void RaCoApplication::switchActiveRaCoProject(const QString& file, std::function<std::string(const std::string&)> relinkCallback, bool createDefaultScene, int featureLevel, bool generateNewObjectIDs) {
	externalProjectsStore_.clear();
	WithRelinkCallback withRelinkCallback(externalProjectsStore_, relinkCallback);

	activeProject_.reset();
	resetSceneBackend();
	resetStats();

	// The module cache should already by empty after removing the local and external projects but explicitly clear it anyway
	// to avoid potential problems.
	engine_->coreInterface()->clearModuleCache();
	dataChangeDispatcher_->assertEmpty();

	core::LoadContext loadContext;

	if (!(featureLevel == -1 ||
			featureLevel >= ramses_base::BaseEngineBackend::minFeatureLevel && featureLevel <= ramses_base::BaseEngineBackend::maxFeatureLevel)) {
		throw std::runtime_error(fmt::format("RamsesLogic feature level {} outside valid range ({} ... {})", featureLevel, static_cast<int>(ramses_base::BaseEngineBackend::minFeatureLevel), static_cast<int>(ramses_base::BaseEngineBackend::maxFeatureLevel)));
	}

	if (file.isEmpty()) {
		int newFeatureLevel = featureLevel == -1 ? newFileFeatureLevel_ : featureLevel;
		engine_->setFeatureLevel(static_cast<ramses::EFeatureLevel>(newFeatureLevel));
		activeProject_ = RaCoProject::createNew(this, createDefaultScene, newFeatureLevel);
	} else {
		auto fileFeatureLevel = RaCoProject::preloadFeatureLevel(file, featureLevel);
		engine_->setFeatureLevel(static_cast<ramses::EFeatureLevel>(fileFeatureLevel));
		activeProject_ = RaCoProject::loadFromFile(file, this, loadContext, false, featureLevel, generateNewObjectIDs);
	}

	externalProjectsStore_.setActiveProject(activeProject_.get());

	logicEngineNeedsUpdate_ = true;

	activeProject_->applyDefaultCachedPaths();
	activeProject_->setupCachedPathSubscriptions(dataChangeDispatcher_);

	setupScene(false, true);
	startTime_ = std::chrono::high_resolution_clock::now();
	doOneLoop();

	// Log all errors after the engine update to make sure that errors created by the adaptor classes are logged.
	activeProject_->errors()->logAllErrors();
}

bool RaCoApplication::saveAsWithNewIDs(const QString& newPath, std::string& outError, bool setProjectName) {
	if (activeRaCoProject().saveAs(newPath, outError, setProjectName)) {
		switchActiveRaCoProject(QString::fromStdString(activeProjectPath()), {}, true, -1, true);
		if (activeRaCoProject().save(outError)) {
			return true;
		}
	}
	return false;
}

core::ErrorLevel RaCoApplication::getExportSceneDescriptionAndStatus(std::vector<core::SceneBackendInterface::SceneItemDesc>& outDescription, std::string& outMessage) {
	setupScene(true, false);
	logicEngineNeedsUpdate_ = true;
	doOneLoop();

	outDescription = previewSceneBackend_->getSceneItemDescriptions();

	core::ErrorLevel errorLevel = previewSceneBackend_->sceneValid();
	if (errorLevel != core::ErrorLevel::NONE) {
		outMessage = sceneBackend()->getValidationReport(errorLevel);
	} else {
		outMessage = std::string();
	}

	setupScene(false, false);
	logicEngineNeedsUpdate_ = true;
	rendererDirty_ = true;

	return errorLevel;
}

bool RaCoApplication::exportProject(const std::string& ramsesExport, bool compress, std::string& outError, bool forceExportWithErrors, ELuaSavingMode luaSavingMode, bool warningsAsErrors) {
	setupScene(true, false);
	logicEngineNeedsUpdate_ = true;
	doOneLoop();

	bool status = exportProjectImpl(ramsesExport, compress, outError, forceExportWithErrors, luaSavingMode, warningsAsErrors);

	setupScene(false, false);
	logicEngineNeedsUpdate_ = true;
	rendererDirty_ = true;

	return status;
}

bool RaCoApplication::exportProjectImpl(const std::string& ramsesExport, bool compress, std::string& outError, bool forceExportWithErrors, ELuaSavingMode luaSavingMode, bool warningsAsErrors) const {
	// Flushing the scene prevents inconsistent states being saved which could lead to unexpected bevahiour after loading the scene:
	previewSceneBackend_->flush();

	if (!forceExportWithErrors) {
		if (activeRaCoProject().errors()->hasError(core::ErrorLevel::ERROR)) {
			outError = "Export failed: scene contains Composer errors:\n";
			for (const auto& [object, objErrors] : activeRaCoProject().errors()->getAllErrors()) {
				for (const auto& [handle, error] : objErrors) {
					if (error.level() >= core::ErrorLevel::ERROR) {
						outError.append(core::Errors::formatError(error));
					}
				}
			}
			return false;
		}
		core::ErrorLevel errorLevel = sceneBackend()->sceneValid();
		if (errorLevel == core::ErrorLevel::ERROR) {
			outError = "Export failed: scene contains Ramses errors:\n" + sceneBackend()->getValidationReport(core::ErrorLevel::WARNING);
			return false;
		} else if (errorLevel == core::ErrorLevel::WARNING) {
			if (warningsAsErrors) {
				outError = "Export failed: scene contains Ramses warnings (treated as errors):\n" + sceneBackend()->getValidationReport(core::ErrorLevel::WARNING);
				return false;
			} else {
				outError = "Export with Ramses warnings:\n" + sceneBackend()->getValidationReport(core::ErrorLevel::WARNING);
			}
		}
	}

	ramses::SaveFileConfig config;
	config.setCompressionEnabled(compress);
	// Use JSON format for the metadata string to allow future extensibility
	// CAREFUL: only include data here which we are certain all users agree to have included in the exported files.
	config.setMetadataString(fmt::format(
		R"___({{
	"generator" : "{}"
}})___",
		QCoreApplication::applicationName().toStdString()));
	config.setExporterVersion(RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH, serialization::RAMSES_PROJECT_FILE_VERSION);
	config.setLuaSavingMode(static_cast<ramses::ELuaSavingMode>(luaSavingMode));

	if (!previewSceneBackend_->currentScene()->saveToFile(ramsesExport.c_str(), config)) {
		outError = previewSceneBackend_->getLastError().value().message;
		return false;
	}

	return true;
}

void RaCoApplication::doOneLoop() {
	// write data into engine
	if (ramses_adaptor::SceneBackend::toSceneId(*activeRaCoProject().project()->settings()->sceneId_) != previewSceneBackend_->currentSceneId()) {
		// No need to setup the abstract scene again since its scene id never changes
		setupScene(false, false);
	}

	for (const auto& timerNode : previewSceneBackend_->logicEngine()->getCollection<ramses::TimerNode>()) {
		if (timerNode->getInputs()->getChild("ticker_us")->get<int64_t>() == 0) {
			logicEngineNeedsUpdate_ = true;
			break;
		}
	}

	int64_t elapsedMsec;
	if (getTime_) {
		elapsedMsec = getTime_();
	} else {
		auto elapsedTime = std::chrono::high_resolution_clock::now() - startTime_;
		elapsedMsec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count();
	}

	activeProject_->tracePlayer().refresh(elapsedMsec);

	auto dataChanges = activeProject_->recorder()->release();
	dataChangeDispatcherPreviewScene_->dispatch(dataChanges);
	if (logicEngineNeedsUpdate_ || !dataChanges.getAllChangedObjects(true, true, true).empty() || !dataChanges.getDeletedObjects().empty()) {
		if (!previewSceneBackend_->logicEngine()->update()) {
			auto issue = previewSceneBackend_->getLastError().value();
			LOG_ERROR(log_system::RAMSES_BACKEND, issue.message);
			previewSceneBackend_->sceneAdaptor()->updateRuntimeError(issue);
		} else {
			previewSceneBackend_->sceneAdaptor()->clearRuntimeError();
		}
		// read modified engine data
		previewSceneBackend_->readDataFromEngine(dataChanges);
		logicEngineNeedsUpdate_ = false;

		if (recordingStats_) {
			logicStats_.addSnapshot(previewSceneBackend_->getPerformanceReport());
			Q_EMIT performanceStatisticsUpdated();
		}
	}

	dataChangeDispatcherAbstractScene_->dispatch(dataChanges);

	dataChangeDispatcher_->dispatch(dataChanges);
}

bool RaCoApplication::canSaveActiveProject() const {
	for (auto item : activeProject_->project()->externalProjectsMap()) {
		auto absPath = activeProject_->project()->lookupExternalProjectPath(item.first);

		if (!externalProjectsStore_.isCurrent(absPath)) {
			return false;
		}
	}
	return true;
}

const core::SceneBackendInterface* RaCoApplication::sceneBackend() const {
	return previewSceneBackend_.get();
}

ramses_adaptor::SceneBackend* RaCoApplication::sceneBackendImpl() const {
	return previewSceneBackend_.get();
}

ramses_adaptor::AbstractSceneAdaptor* RaCoApplication::abstractScene() const {
	return abstractScene_.get();
}

components::SDataChangeDispatcher RaCoApplication::dataChangeDispatcher() {
	return dataChangeDispatcher_;
}

core::EngineInterface* RaCoApplication::engine() {
	return engine_->coreInterface();
}

QString RaCoApplication::generateApplicationTitle() const {
	const auto& project = activeRaCoProject();
	if (activeProjectPath().empty()) {
		return QCoreApplication::applicationName() + " - <New project>";
	}

	auto path = QString::fromStdString(activeProjectPath());
	auto windowTitle = QCoreApplication::applicationName() + " - " + project.name() + " (" + path + ")";
	auto fileInfo = QFileInfo(path);
	if (fileInfo.exists() && !fileInfo.isWritable()) {
		windowTitle += " <read-only>";
	} else {
#ifdef OS_WINDOWS
		// check NTFS permissions under Win, only after simple read-only check returns false
		// (the permissions may still forbid writing)
		qt_ntfs_permission_lookup++;
		fileInfo = QFileInfo(path);
		if (fileInfo.exists() && !fileInfo.isWritable()) {
			windowTitle += " <read-only>";
		}
		qt_ntfs_permission_lookup--;
#endif
	}
	return windowTitle;
}

bool RaCoApplication::isRunningInUI() const {
	return runningInUI_;
}

void RaCoApplication::overrideTime(std::function<int64_t()> getTime) {
	getTime_ = getTime;
}

int RaCoApplication::minFeatureLevel() {
	return static_cast<int>(ramses_base::BaseEngineBackend::minFeatureLevel);
}

int RaCoApplication::maxFeatureLevel() {
	return static_cast<int>(ramses_base::BaseEngineBackend::maxFeatureLevel);
}

const std::string& RaCoApplication::featureLevelDescriptions() {
	return ramses_base::BaseEngineBackend::featureLevelDescriptions;
}

void RaCoApplication::setNewFileFeatureLevel(int featureLevel) {
	newFileFeatureLevel_ = featureLevel;
}

int RaCoApplication::newFileFeatureLevel() const {
	return newFileFeatureLevel_;
}

const FeatureLevelLoadError* RaCoApplication::getFlError() const {
	return externalProjectsStore_.getFlError();
}

void RaCoApplication::setRecordingStats(bool enable) {
	recordingStats_ = enable;
	previewSceneBackend_->logicEngine()->enableUpdateReport(enable);
}

void RaCoApplication::resetStats() {
	logicStats_ = ReportStatistics{};
	Q_EMIT performanceStatisticsUpdated();
}

const ReportStatistics& RaCoApplication::getLogicStats() const {
	return logicStats_;
}

core::ExternalProjectsStoreInterface* RaCoApplication::externalProjects() {
	return &externalProjectsStore_;
}

core::MeshCache* RaCoApplication::meshCache() {
	return &meshCache_;
}

}  // namespace raco::application