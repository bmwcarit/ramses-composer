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
#include "ramses_base/BaseEngineBackend.h"
#include "user_types/Animation.h"

#include "core/Handles.h"
#include <ramses_base/LogicEngineFormatter.h>

#ifdef OS_WINDOWS
// see: https://doc.qt.io/qt-5/qfileinfo.html#ntfs-permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

namespace raco::application {

RaCoApplicationLaunchSettings::RaCoApplicationLaunchSettings()
	: initialProject{},
	  createDefaultScene{true},
	  enableRamsesTrace{false},
	  featureLevel{-1},
	  runningInUI{false} {
}

RaCoApplicationLaunchSettings::RaCoApplicationLaunchSettings(QString argInitialProject, bool argCreateDefaultScene, bool argEnableRamsesTrace, int argFeatureLevel, bool argRunningInUI)
	: initialProject(argInitialProject),
	  createDefaultScene(argCreateDefaultScene),
	  enableRamsesTrace(argEnableRamsesTrace),
	  featureLevel(argFeatureLevel),
	  runningInUI(argRunningInUI) {
}

RaCoApplication::RaCoApplication(ramses_base::BaseEngineBackend& engine, const RaCoApplicationLaunchSettings& settings)
	: engine_{&engine},
	  applicationFeatureLevel_(settings.featureLevel),
	  dataChangeDispatcher_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  dataChangeDispatcherEngine_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  scenesBackend_{new ramses_adaptor::SceneBackend(engine, dataChangeDispatcherEngine_)},
	  externalProjectsStore_(this) {
	ramses_base::installRamsesLogHandler(settings.enableRamsesTrace);
	ramses_base::installLogicLogHandler();
	// Preferences need to be initalized before we have a fist initial project
	raco::components::RaCoPreferences::init();

	runningInUI_ = settings.runningInUI;

	switchActiveRaCoProject(settings.initialProject, {}, settings.createDefaultScene, settings.featureLevel);
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
	scenesBackend_->reset();
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

void RaCoApplication::setupScene(bool optimizeForExport) {
	engine_->setFeatureLevel(static_cast<rlogic::EFeatureLevel>(activeRaCoProject().project()->featureLevel()));
	scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors(), optimizeForExport);
}

void RaCoApplication::switchActiveRaCoProject(const QString& file, std::function<std::string(const std::string&)> relinkCallback, bool createDefaultScene, int featureLevel, bool generateNewObjectIDs) {
	externalProjectsStore_.clear();
	WithRelinkCallback withRelinkCallback(externalProjectsStore_, relinkCallback);

	activeProject_.reset();
	scenesBackend_->reset();

	// The module cache should already by empty after removing the local and external projects but explicitly clear it anyway
	// to avoid potential problems.
	engine_->coreInterface()->clearModuleCache();
	dataChangeDispatcher_->assertEmpty();

	core::LoadContext loadContext;

	if (!(featureLevel == -1 ||
			featureLevel >= ramses_base::BaseEngineBackend::minFeatureLevel && featureLevel <= ramses_base::BaseEngineBackend::maxFeatureLevel)) {
		throw std::runtime_error(fmt::format("RamsesLogic feature level {} outside valid range ({} ... {})", featureLevel, static_cast<int>(raco::ramses_base::BaseEngineBackend::minFeatureLevel), static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel)));
	}
	if (file.isEmpty()) {
		activeProject_ = RaCoProject::createNew(this, createDefaultScene, static_cast<int>(featureLevel == -1 ? applicationFeatureLevel_ : featureLevel));
	} else {
		activeProject_ = RaCoProject::loadFromFile(file, this, loadContext, false, featureLevel, generateNewObjectIDs);
	}

	externalProjectsStore_.setActiveProject(activeProject_.get());

	logicEngineNeedsUpdate_ = true;

	activeProject_->applyDefaultCachedPaths();
	activeProject_->setupCachedPathSubscriptions(dataChangeDispatcher_);

	setupScene(false);
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
	setupScene(true);
	logicEngineNeedsUpdate_ = true;
	doOneLoop();

	outDescription = scenesBackend_->getSceneItemDescriptions();

	core::ErrorLevel errorLevel = scenesBackend_->sceneValid();
	if (errorLevel != core::ErrorLevel::NONE) {
		outMessage = sceneBackend()->getValidationReport(errorLevel);
	} else {
		outMessage = std::string();
	}

	setupScene(false);
	logicEngineNeedsUpdate_ = true;
	rendererDirty_ = true;

	return errorLevel;
}

bool RaCoApplication::exportProject(const std::string& ramsesExport, const std::string& logicExport, bool compress, std::string& outError, bool forceExportWithErrors, ELuaSavingMode luaSavingMode) {
	setupScene(true);
	logicEngineNeedsUpdate_ = true;
	doOneLoop();

	bool status = exportProjectImpl(ramsesExport, logicExport, compress, outError, forceExportWithErrors, luaSavingMode);

	setupScene(false);
	logicEngineNeedsUpdate_ = true;
	rendererDirty_ = true;

	return status;
}

bool RaCoApplication::exportProjectImpl(const std::string& ramsesExport, const std::string& logicExport, bool compress, std::string& outError, bool forceExportWithErrors, ELuaSavingMode luaSavingMode) const {
	// Flushing the scene prevents inconsistent states being saved which could lead to unexpected bevahiour after loading the scene:
	scenesBackend_->flush();

	if (!forceExportWithErrors) {
		if (activeRaCoProject().errors()->hasError(raco::core::ErrorLevel::ERROR)) {
			outError = "Export failed: scene contains Composer errors:\n";
			for (const auto& [object, objErrors] : activeRaCoProject().errors()->getAllErrors()) {
				for (const auto& [handle, error] : objErrors) {
					if (error.level() >= raco::core::ErrorLevel::ERROR) {
						outError.append(raco::core::Errors::formatError(error));
					}
				}
			}
			return false;
		}
		if (sceneBackend()->sceneValid() != core::ErrorLevel::NONE) {
			outError = "Export failed: scene contains Ramses errors:\n" + sceneBackend()->getValidationReport(core::ErrorLevel::WARNING);
			return false;
		}
	}
	auto status = scenesBackend_->currentScene()->saveToFile(ramsesExport.c_str(), compress);
	if (status != ramses::StatusOK) {
		outError = scenesBackend_->currentScene()->getStatusMessage(status);
		return false;
	}
	rlogic::SaveFileConfig metadata;
	// Since the SceneBackend filters some validation warnings we have to disable validation when saving, because
	// we can't selectively disable some validation warnings here:
	metadata.setValidationEnabled(false);
	// Use JSON format for the metadata string to allow future extensibility
	// CAREFUL: only include data here which we are certain all users agree to have included in the exported files.
	metadata.setMetadataString(fmt::format(
		R"___({{
	"generator" : "{}"
}})___",
		QCoreApplication::applicationName().toStdString()));
	metadata.setExporterVersion(RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH, raco::serialization::RAMSES_PROJECT_FILE_VERSION);

	metadata.setLuaSavingMode(static_cast<rlogic::ELuaSavingMode>(luaSavingMode));

	if (!engine_->logicEngine().saveToFile(logicExport.c_str(), metadata)) {
		if (engine_->logicEngine().getErrors().size() > 0) {
			outError = engine_->logicEngine().getErrors().at(0).message;
		} else {
			outError = "Unknown Errror: ramses-logic failed to export.";
		}
		return false;
	}
	return true;
}

void RaCoApplication::doOneLoop() {
	// write data into engine
	if (ramses_adaptor::SceneBackend::toSceneId(*activeRaCoProject().project()->settings()->sceneId_) != scenesBackend_->currentSceneId()) {
		scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors(), false);
	}

	for (const auto& timerNode : engine_->logicEngine().getCollection<rlogic::TimerNode>()) {
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
	dataChangeDispatcherEngine_->dispatch(dataChanges);
	if (logicEngineNeedsUpdate_ || !dataChanges.getAllChangedObjects(true, true, true).empty()) {
		if (!engine_->logicEngine().update()) {
			LOG_ERROR_IF(raco::log_system::RAMSES_BACKEND, !engine_->logicEngine().getErrors().empty(), "{}", LogicEngineErrors{engine_->logicEngine()});
		}
		// read modified engine data / runtime errors
		scenesBackend_->readDataFromEngine(dataChanges);
		logicEngineNeedsUpdate_ = false;
	}

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
	return scenesBackend_.get();
}

raco::ramses_adaptor::SceneBackend* RaCoApplication::sceneBackendImpl() const {
	return scenesBackend_.get();
}

raco::components::SDataChangeDispatcher RaCoApplication::dataChangeDispatcher() {
	return dataChangeDispatcher_;
}

raco::core::EngineInterface* RaCoApplication::engine() {
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
	return static_cast<int>(raco::ramses_base::BaseEngineBackend::minFeatureLevel);
}

int RaCoApplication::maxFeatureLevel() {
	return static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel);
}

const std::string& RaCoApplication::featureLevelDescriptions() {
	return raco::ramses_base::BaseEngineBackend::featureLevelDescriptions;
}

void RaCoApplication::setApplicationFeatureLevel(int featureLevel) {
	applicationFeatureLevel_ = featureLevel;
}

int RaCoApplication::applicationFeatureLevel() const {
	return applicationFeatureLevel_;
}

const FeatureLevelLoadError* RaCoApplication::getFlError() const {
	return externalProjectsStore_.getFlError();
}

core::ExternalProjectsStoreInterface* RaCoApplication::externalProjects() {
	return &externalProjectsStore_;
}

raco::core::MeshCache* RaCoApplication::meshCache() {
	return &meshCache_;
}

}  // namespace raco::application