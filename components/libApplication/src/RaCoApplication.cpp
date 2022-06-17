/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/RaCoApplication.h"

#include "components/RaCoPreferences.h"

#include "utils/u8path.h"

#include "core/PathManager.h"
#include "core/Project.h"
#include "core/Context.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"
#include "user_types/Animation.h"
#include "core/ProjectMigration.h"

#include <ramses_base/LogicEngineFormatter.h>
#include "core/Handles.h"

#ifdef OS_WINDOWS
// see: https://doc.qt.io/qt-5/qfileinfo.html#ntfs-permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

namespace raco::application {

RaCoApplication::RaCoApplication(ramses_base::BaseEngineBackend& engine, const QString& initialProject, bool createDefaultScene)
	: engine_{&engine},
	  dataChangeDispatcher_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  dataChangeDispatcherEngine_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  scenesBackend_{new ramses_adaptor::SceneBackend(&engine, dataChangeDispatcherEngine_)},
	  externalProjectsStore_(this) {
	ramses_base::installLogicLogHandler();
	// Preferences need to be initalized before we have a fist initial project
	raco::components::RaCoPreferences::init();

	switchActiveRaCoProject(initialProject, createDefaultScene);
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

void RaCoApplication::switchActiveRaCoProject(const QString& file, bool createDefaultScene) {
	externalProjectsStore_.clear();
	activeProject_.reset();
	scenesBackend_->reset();

	dataChangeDispatcher_->assertEmpty();

	std::vector<std::string> stack;
	activeProject_ = file.isEmpty() ? RaCoProject::createNew(this, createDefaultScene) : RaCoProject::loadFromFile(file, this, stack);

	externalProjectsStore_.setActiveProject(activeProject_.get());

	logicEngineNeedsUpdate_ = true;

	activeProject_->applyDefaultCachedPaths();
	activeProject_->subscribeDefaultCachedPathChanges(dataChangeDispatcher_);

	scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors());
	startTime_ = std::chrono::high_resolution_clock::now();
	doOneLoop();
}

bool RaCoApplication::exportProject(const std::string& ramsesExport, const std::string& logicExport, bool compress, std::string& outError, bool forceExportWithErrors) const {
	// Flushing the scene prevents inconsistent states being saved which could lead to unexpected bevahiour after loading the scene:
	scenesBackend_->flush();

	// we currently only support export of active project currently
	if (activeRaCoProject().errors()->hasError(raco::core::ErrorLevel::ERROR)) {
		outError = "Export failed: scene contains Composer errors";
		return false;
	}
	if (!sceneBackend()->sceneValid()) {
		outError = "Export failed: scene contains Ramses errors";
		return false;
	}
	auto status = scenesBackend_->currentScene()->saveToFile(ramsesExport.c_str(), compress);
	if (status != ramses::StatusOK) {
		outError = scenesBackend_->currentScene()->getStatusMessage(status);
		return false;
	}
	rlogic::SaveFileConfig metadata;
	metadata.setValidationEnabled(!forceExportWithErrors);
	// Use JSON format for the metadata string to allow future extensibility
	// CAREFUL: only include data here which we are certain all users agree to have included in the exported files.
	metadata.setMetadataString(fmt::format(
R"___({{
	"generator" : "{}"
}})___",
		QCoreApplication::applicationName().toStdString()));
	metadata.setExporterVersion(RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH, raco::serialization::RAMSES_PROJECT_FILE_VERSION);

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
		scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors());
	}

	for (const auto& timerNode : engine_->logicEngine().getCollection<rlogic::TimerNode>()) {
		if (timerNode->getInputs()->getChild("ticker_us")->get<int64_t>() == 0) {
			logicEngineNeedsUpdate_ = true;
			break;
		}
	}

	auto elapsedTime = std::chrono::high_resolution_clock::now() - startTime_;
	auto elapsedMsec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count();

	auto activeProjectRunsTimer = activeRaCoProject().project()->settings()->runTimer_.asBool();
	if (activeProjectRunsTimer) {
		auto loadedScripts = engine_->logicEngine().getCollection<rlogic::LuaScript>();
		for (auto* loadedScript : loadedScripts) {
			if (loadedScript->getInputs()->hasChild("time_ms")) {
				loadedScript->getInputs()->getChild("time_ms")->set(static_cast<int32_t>(elapsedMsec));
			}
		}
	}

	auto dataChanges = activeProject_->recorder()->release();
	dataChangeDispatcherEngine_->dispatch(dataChanges);
	if (activeProjectRunsTimer || logicEngineNeedsUpdate_ || !dataChanges.getAllChangedObjects(true, true, true).empty()) {
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

core::ExternalProjectsStoreInterface* RaCoApplication::externalProjects() {
	return &externalProjectsStore_;
}

raco::core::MeshCache* RaCoApplication::meshCache() {
	return &meshCache_;
}

}  // namespace raco::application