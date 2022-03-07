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

#include <ramses_base/LogicEngineFormatter.h>
#include "core/Handles.h"

#ifdef OS_WINDOWS
// see: https://doc.qt.io/qt-5/qfileinfo.html#ntfs-permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

namespace raco::application {

RaCoApplication::RaCoApplication(ramses_base::BaseEngineBackend& engine, const QString& initialProject)
	: engine_{&engine},
	  dataChangeDispatcher_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  dataChangeDispatcherEngine_{std::make_shared<raco::components::DataChangeDispatcher>()},
	  scenesBackend_{new ramses_adaptor::SceneBackend(&engine, dataChangeDispatcherEngine_)},
	  externalProjectsStore_(this) {
	ramses_base::installLogicLogHandler();
	// Preferences need to be initalized before we have a fist initial project
	raco::components::RaCoPreferences::init();
	std::vector<std::string> stack;
	activeProject_ = initialProject.isEmpty() ? RaCoProject::createNew(this) : RaCoProject::loadFromFile(initialProject, this, stack);
	externalProjectsStore_.setActiveProject(activeProject_.get());

	logicEngineNeedsUpdate_ = true;
	scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors());

	activeProject_->applyDefaultCachedPaths();
	activeProject_->subscribeDefaultCachedPathChanges(dataChangeDispatcher_);

	startTime_ = std::chrono::high_resolution_clock::now();
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

void RaCoApplication::resetScene() {
	scenesBackend_->reset();
}

void RaCoApplication::switchActiveRaCoProject(const QString& file) {
	externalProjectsStore_.clear();
	activeProject_.reset();
	scenesBackend_->reset();

	dataChangeDispatcher_->assertEmpty();

	std::vector<std::string> stack;
	activeProject_ = file.isEmpty() ? RaCoProject::createNew(this) : RaCoProject::loadFromFile(file, this, stack);

	externalProjectsStore_.setActiveProject(activeProject_.get());

	logicEngineNeedsUpdate_ = true;

	activeProject_->applyDefaultCachedPaths();
	activeProject_->subscribeDefaultCachedPathChanges(dataChangeDispatcher_);

	scenesBackend_->setScene(activeRaCoProject().project(), activeRaCoProject().errors());
}

bool RaCoApplication::exportProject(const RaCoProject& project, const std::string& ramsesExport, const std::string& logicExport, bool compress, std::string& outError) const {
	// we currently only support export of active project currently
	assert(&project == &activeRaCoProject());
	auto status = scenesBackend_->currentScene()->saveToFile(ramsesExport.c_str(), compress);
	if (status != ramses::StatusOK) {
		outError = scenesBackend_->currentScene()->getStatusMessage(status);
		return false;
	}
	if (!engine_->logicEngine().saveToFile(logicExport.c_str())) {
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

	auto animNodes = engine_->logicEngine().getCollection<rlogic::AnimationNode>();
	logicEngineNeedsUpdate_ |= (animNodes.size() > 0);

	auto elapsedTime = std::chrono::high_resolution_clock::now() - startTime_;
	auto elapsedMsec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count();

	auto activeProjectRunsTimer = activeRaCoProject().project()->settings()->runTimer_.asBool();
	if (activeProjectRunsTimer) {
		auto loadedScripts = engine_->logicEngine().getCollection<rlogic::LuaScript>();
		for (auto* loadedScript : loadedScripts) {
			if (auto* timerInput = loadedScript->getInputs()->getChild("time_ms")) {
				timerInput->set(static_cast<int32_t>(elapsedMsec));
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

	auto msecDiff = elapsedMsec - totalElapsedMsec_;
	totalElapsedMsec_ = elapsedMsec;
	// keep Ramses logic animation nodes dirty so they will keep running in the next loop
	for (auto animNode : animNodes) {
		animNode->getInputs()->getChild("timeDelta")->set(msecDiff / 1000.0F);
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
		return APPLICATION_NAME + " - <New project>";
	}

	auto path = QString::fromStdString(activeProjectPath());
	auto windowTitle = APPLICATION_NAME + " - " + project.name() + " (" + path + ")";
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