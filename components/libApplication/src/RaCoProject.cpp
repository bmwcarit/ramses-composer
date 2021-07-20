/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/RaCoProject.h"

#include "core/Consistency.h"
#include "core/Context.h"
#include "core/ExtrefOperations.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "ramses_base/BaseEngineBackend.h"
#include "components/FileChangeMonitorImpl.h"
#include "components/Naming.h"
#include "application/RaCoApplication.h"
#include "components/RaCoPreferences.h"
#include "components/RamsesProjectMigration.h"
#include "serialization/Serialization.h"
#include "serialization/SerializationKeys.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/UserObjectFactory.h"
#include "utils/FileUtils.h"
#include "utils/PathUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "utils/stdfilesystem.h"
#include <functional>

namespace raco::application {

using namespace raco::core;

RaCoProject::RaCoProject(const QString& file, Project& p, EngineInterface* engineInterface, const UndoStack::Callback& callback, ExternalProjectsStoreInterface* externalProjectsStore, RaCoApplication* app, std::vector<std::string>& pathStack)
	: recorder_{},
	  errors_{&recorder_},
	  project_{p},
	  context_{std::make_shared<BaseContext>(&project_, engineInterface, &user_types::UserObjectFactory::getInstance(), &recorder_, &errors_)},
	  undoStack_(context_.get(), [this, callback]() {
		  dirty_ = true;
		  callback();
	  }),
	  commandInterface_(context_.get(), &undoStack_),
	  fileChangeMonitor_{app->fileChangeMonitor()},
	  meshCache_{app->meshCache()} {
	context_->setMeshCache(meshCache_);
	context_->setExternalProjectsStore(externalProjectsStore);
	context_->setFileChangeMonitor(fileChangeMonitor_);
	context_->performExternalFileReload(project_.instances());

	// Push currently loading project on the project load stack to enable project loop detection to work.
	pathStack.emplace_back(file.toStdString());
	context_->updateExternalReferences(pathStack);
	pathStack.pop_back();

	undoStack_.reset();
	context_->changeMultiplexer().reset();
	dirty_ = false;
}

void RaCoProject::onAfterProjectPathChange(const std::string& oldPath, const std::string& newPath) {
	for (auto& object : context_->project()->instances()) {
		if (PathQueries::isPathRelativeToCurrentProject(object)) {
			for (const auto& property : ValueTreeIteratorAdaptor(ValueHandle{object})) {
				if (property.query<data_storage::URIAnnotation>()) {
					auto uriPath = property.asString();
					if (!uriPath.empty() && std::filesystem::path{uriPath}.is_relative()) {
						context_->set(property, PathManager::rerootRelativePath(uriPath, oldPath, newPath));
					}
				}
			}
		}
	}
	project_.rerootExternalProjectPaths(oldPath, newPath);
}

void RaCoProject::generateProjectSubfolder(const std::string& subFolderPath) {
	auto folderPath = project_.currentFolder() + "/" + subFolderPath;
	if (!raco::utils::path::isExistingDirectory(folderPath)) {
		std::filesystem::create_directories(folderPath);
	}
}

void RaCoProject::generateAllProjectSubfolders() {
	generateProjectSubfolder(PathManager::IMAGE_SUB_DIRECTORY);
	generateProjectSubfolder(PathManager::MESH_SUB_DIRECTORY);
	generateProjectSubfolder(PathManager::SCRIPT_SUB_DIRECTORY);
	generateProjectSubfolder(PathManager::SHADER_SUB_DIRECTORY);
}

RaCoProject::~RaCoProject() {
	for (const auto& instance : project_.instances()) {
		instance->onBeforeDeleteObject(errors_);
	}
}

std::unique_ptr<RaCoProject> RaCoProject::createNew(RaCoApplication* app) {
	LOG_INFO(raco::log_system::PROJECT, "");
	Project p{};
	p.setCurrentPath(components::RaCoPreferences::instance().userProjectsDirectory.toStdString());

	std::vector<std::string> stack;
	auto result = std::unique_ptr<RaCoProject>(new RaCoProject(
		QString{},
		p,
		app->engine(),
		[app]() { app->dataChangeDispatcher()->setUndoChanged(); },
		app->externalProjects(),
		app,
		stack));
	auto sMeshNode = result->context_->createObject(raco::user_types::MeshNode::typeDescription.typeName, raco::components::Naming::format(raco::user_types::MeshNode::typeDescription.typeName));
	auto sNode = result->context_->createObject(raco::user_types::Node::typeDescription.typeName, raco::components::Naming::format(raco::user_types::Node::typeDescription.typeName));
	auto sCamera = result->context_->createObject(raco::user_types::PerspectiveCamera::typeDescription.typeName, raco::components::Naming::format(raco::user_types::PerspectiveCamera::typeDescription.typeName));

	auto settings = result->context_->createObject(ProjectSettings::typeDescription.typeName);

	result->context_->set({sCamera, {"translation", "z"}}, 10.0);
	result->context_->moveScenegraphChild(sMeshNode, sNode);
	result->undoStack_.reset();
	result->context_->changeMultiplexer().reset();
	result->dirty_ = false;

	Consistency::checkProjectSettings(*result->context_->project());

	return result;
}

std::unique_ptr<RaCoProject> RaCoProject::loadFromFile(const QString& filename, RaCoApplication* app, std::vector<std::string>& pathStack) {
	LOG_INFO(raco::log_system::PROJECT, "Loading project from {}", filename.toLatin1());

	if (!raco::utils::path::isExistingFile(filename.toStdString())) {
		LOG_WARNING(raco::log_system::PROJECT, "File not found {}", filename.toLatin1());
		return {};
	}

	QFile file{filename};
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		LOG_WARNING(raco::log_system::PROJECT, "Can't read file {}", filename.toLatin1());
		return {};
	}
	auto document{QJsonDocument::fromJson(file.readAll())};
	file.close();

	auto fileVersion{raco::serialization::deserializeFileVersion(document)};
	if (fileVersion > raco::components::RAMSES_PROJECT_FILE_VERSION) {
		throw FutureFileVersion{fileVersion};
	}
	auto migratedJson{raco::components::migrateProject(document)};

	auto result{raco::serialization::deserializeProject(migratedJson,
		user_types::UserObjectFactoryInterface::deserializationFactory(&user_types::UserObjectFactory::getInstance()))};

	std::vector<core::SEditorObject> instances{};
	std::map<std::string, core::SEditorObject> instanceMap;
	instances.reserve(result.objectsDeserialization.objects.size());
	for (auto& d : result.objectsDeserialization.objects) {
		auto obj = std::dynamic_pointer_cast<core::EditorObject>(d);
		instances.push_back(obj);
		instanceMap[obj->objectID()] = obj;
	}
	for (const auto& pair : result.objectsDeserialization.references) {
		*pair.first = instanceMap.at(pair.second);
	}

	Project p{instances};
	p.setCurrentPath(filename.toStdString());
	for (const auto& instance : instances) {
		instance->onAfterDeserialization();
	}
	for (const auto& link : result.objectsDeserialization.links) {
		p.addLink(std::dynamic_pointer_cast<Link>(link));
	}
	for (auto [id, info] : result.objectsDeserialization.externalProjectsMap) {
		auto absPath = PathManager::constructAbsolutePath(p.currentFolder(), info.path);
		p.addExternalProjectMapping(id, absPath, info.name);
	}
	LOG_INFO(raco::log_system::PROJECT, "Finished loading project from {}", filename.toLatin1());

	Consistency::checkProjectSettings(p);

	return std::unique_ptr<RaCoProject>(new RaCoProject{
		filename,
		p,
		app->engine(),
		[app]() { app->dataChangeDispatcher()->setUndoChanged(); },
		app->externalProjects(), 
		app,
		pathStack});
}

QString RaCoProject::name() const {
	return QString::fromStdString(project_.settings()->objectName());
}

void RaCoProject::save() {
	const auto path(project_.currentPath());
	LOG_INFO(raco::log_system::PROJECT, "Saving project to {}", path);
	QFile file{path.c_str()};
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;
	const auto& instances{context_->project()->instances()};
	std::vector<std::shared_ptr<ReflectionInterface>> instancesInterface{instances.begin(), instances.end()};
	const auto& links{context_->project()->links()};
	std::vector<std::shared_ptr<ReflectionInterface>> linksInterface{links.begin(), links.end()};

	auto ramsesVersion = raco::ramses_base::getRamsesVersion();
	auto ramsesLogicEngineVersion = raco::ramses_base::getLogicEngineVersion();

	std::unordered_map<std::string, std::vector<int>> currentVersions = {
		{raco::serialization::keys::FILE_VERSION, {raco::components::RAMSES_PROJECT_FILE_VERSION}},
		{raco::serialization::keys::RAMSES_VERSION, {ramsesVersion.major, ramsesVersion.minor, ramsesVersion.patch}},
		{raco::serialization::keys::RAMSES_LOGIC_ENGINE_VERSION, {static_cast<int>(ramsesLogicEngineVersion.major), static_cast<int>(ramsesLogicEngineVersion.minor), static_cast<int>(ramsesLogicEngineVersion.patch)}},
		{raco::serialization::keys::RAMSES_COMPOSER_VERSION, {RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH}}};
	file.write(serialization::serializeProject(
		currentVersions,
		instancesInterface, linksInterface,
		project_.externalProjectsMap(),
		[](const raco::data_storage::ValueBase& value) -> std::optional<std::string> {
			if (value.asRef()) {
				return value.asRef()->objectID();
			} else {
				return {};
			}
		}).toJson());
	file.close();
	generateAllProjectSubfolders();
	PathManager::setAllCachedPathRoots(project_.currentFolder());

	dirty_ = false;
	LOG_INFO(raco::log_system::PROJECT, "Finished saving project to {}", path);
}

void RaCoProject::saveAs(const QString& fileName, bool setProjectName) {
	auto oldPath = project_.currentPath();
	auto oldProjectFolder = project_.currentFolder();
	auto newPath = fileName.toStdString();
	if (newPath == oldPath) {
		save();
	} else {
		project_.setCurrentPath(newPath);
		onAfterProjectPathChange(oldProjectFolder, project_.currentFolder());
		if (setProjectName) {
			auto projName = std::filesystem::path(newPath).stem().string();
			auto settings = project_.settings();
			if (settings->objectName().empty()) {
				context_->set({settings, {"objectName"}}, projName);
			}
		}
		save();
		undoStack_.reset();
		dirty_ = false;
	}
}

bool RaCoProject::dirty() const noexcept {
	return dirty_;
}

void RaCoProject::updateExternalReferences(std::vector<std::string>& pathStack) {
	context_->updateExternalReferences(pathStack);
}

Project* RaCoProject::project() {
	return &project_;
}

Errors* RaCoProject::errors() {
	return &errors_;
}

DataChangeRecorder* RaCoProject::recorder() {
	return &recorder_;
}

CommandInterface* RaCoProject::commandInterface() {
	return &commandInterface_;
}

FileChangeMonitor* RaCoProject::fileChangeMonitor() {
	return fileChangeMonitor_;
}

UndoStack* RaCoProject::undoStack() {
	return &undoStack_;
}

MeshCache* RaCoProject::meshCache() {
	return meshCache_;
}

}  // namespace raco::application