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
#include "core/RamsesProjectMigration.h"
#include "core/Serialization.h"
#include "core/SerializationKeys.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/UserObjectFactory.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"
#include "utils/FileUtils.h"
#include "utils/PathUtils.h"
#include "core/CoreFormatter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "utils/stdfilesystem.h"
#include <functional>

#include "core/PrefabOperations.h"
#include "user_types/PrefabInstance.h"

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
	
	// Abort file loading if we encounter external reference RenderPasses or extref cameras outside a Prefab.
	// A bug in V0.9.0 allowed to create such projects.
	// TODO: remove this eventually when we are reasonably certain that no such projects have been encountered.
	for (const auto& object : context_->project()->instances()) {
		if ((&object->getTypeDescription() == &user_types::RenderPass::typeDescription) &&
			object->query<raco::core::ExternalReferenceAnnotation>()) {
			throw std::runtime_error("project contains external reference RenderPass.");
		}

		if ((&object->getTypeDescription() == &user_types::PerspectiveCamera::typeDescription ||
			&object->getTypeDescription() == &user_types::OrthographicCamera::typeDescription) &&
			object->query<raco::core::ExternalReferenceAnnotation>()) {
			if (!PrefabOperations::findContainingPrefab(object)) {
				throw std::runtime_error("file contains external reference camera outside a prefab");
			}
		}
	}

	// Detect and repair broken files with duplicate links.
	// Since link duplicates may differ in link validity we need to initialize the link validity from scratch.
	if (project_.checkLinkDuplicates()) {
		project_.deduplicateLinks();
		context_->initLinkValidity();
	}

	// TODO: the following code repairs URIs which have been "rerooted" incorrectly during paste.
	// This code should really be migration code but rewriting the code below to use only the JSON 
	// is much more complicated than the code below.
	// TODO: deal with this once we know how to do migration code in an easier way.
	for (auto& object : context_->project()->instances()) {
		if (auto prefabInst = PrefabOperations::findContainingPrefabInstance(object)) {
			if (auto prefab = *prefabInst->template_) {
				if (prefab->query<raco::core::ExternalReferenceAnnotation>()) {
					for (const auto& property : ValueTreeIteratorAdaptor(ValueHandle{object})) {
						if (property.query<URIAnnotation>()) {
							// The PrefabInstance::mapToInstance_ property is only filled for the outermost PrefabInstance if 
							// there are nested instances. So we need to search the topmost PrefabInstance first:
							while (auto parentPrefabInst = PrefabOperations::findContainingPrefabInstance(prefabInst->getParent())) {
								prefabInst = parentPrefabInst;
							}
							auto prefabObj = user_types::PrefabInstance::mapFromInstance(object, prefabInst);
							auto prefabProperty = ValueHandle::translatedHandle(property, prefabObj);

							if (property.asString() != prefabProperty.asString()) {
								LOG_WARNING(raco::log_system::PROJECT, "Rewrite URI property '{}': '{}' -> '{}' in project '{}'",
									property.getPropertyPath(),
									property.asString(), prefabProperty.asString(),
									project_.currentPath());
									context_->set(property, prefabProperty.asString());
							}
						}
					}
				}
			}
		}
	}
	
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
				if (property.query<URIAnnotation>()) {
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
	const auto& prefs = raco::components::RaCoPreferences::instance();
	generateProjectSubfolder(prefs.imageSubdirectory.toStdString());
	generateProjectSubfolder(prefs.meshSubdirectory.toStdString());
	generateProjectSubfolder(prefs.scriptSubdirectory.toStdString());
	generateProjectSubfolder(prefs.shaderSubdirectory.toStdString());
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
	auto sNode = result->context_->createObject(raco::user_types::Node::typeDescription.typeName, raco::components::Naming::format(raco::user_types::Node::typeDescription.typeName))->as<user_types::Node>();
	auto sCamera = result->context_->createObject(raco::user_types::PerspectiveCamera::typeDescription.typeName, raco::components::Naming::format(raco::user_types::PerspectiveCamera::typeDescription.typeName))->as<user_types::PerspectiveCamera>();
	auto sRenderPass = result->context_->createObject(raco::user_types::RenderPass::typeDescription.typeName, "MainRenderPass")->as<user_types::RenderPass>();
	auto sRenderLayer = result->context_->createObject(raco::user_types::RenderLayer::typeDescription.typeName, "MainRenderLayer")->as<user_types::RenderLayer>();

	auto settings = result->context_->createObject(ProjectSettings::typeDescription.typeName);
	
	result->context_->set({sRenderPass, &user_types::RenderPass::camera_}, sCamera);
	result->context_->set({sRenderPass, &user_types::RenderPass::layer0_}, sRenderLayer);
	result->context_->addProperty({sRenderLayer, &user_types::RenderLayer::renderableTags_}, "render_main", std::make_unique<data_storage::Value<int>>(0));

	result->context_->set({sNode, &user_types::Node::tags_}, std::vector<std::string>({"render_main"}));
	result->context_->set({sCamera, &user_types::Node::translation_, &data_storage::Vec3f::z}, 10.0);
	result->context_->moveScenegraphChildren({sMeshNode}, sNode);
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
	if (document.isNull()) {
		throw std::runtime_error("Loading JSON file resulted in a null document object");
	}
	file.close();

	auto fileVersion{raco::serialization::deserializeFileVersion(document)};
	if (fileVersion > raco::core::RAMSES_PROJECT_FILE_VERSION) {
		throw FutureFileVersion{fileVersion};
	}

	std::unordered_map<std::string, std::string> migrationObjWarnings;
	auto migratedJson{migrateProject(document, migrationObjWarnings)};

	auto newProject = loadFromJson(migratedJson, filename, app, pathStack);

	for (const auto& [objectID, infoMessage] : migrationObjWarnings) {
		if (const auto migratedObj = newProject->project()->getInstanceByID(objectID)) {
			newProject->errors()->addError(raco::core::ErrorCategory::MIGRATION_ERROR, ErrorLevel::WARNING, migratedObj, infoMessage);
		}
	}

	newProject->errors()->logAllErrors();

	return newProject;
}

std::unique_ptr<RaCoProject> RaCoProject::loadFromJson(const QJsonDocument& migratedJson, const QString& filename, RaCoApplication* app, std::vector<std::string>& pathStack) {
	auto result{ raco::serialization::deserializeProject(migratedJson,
		user_types::UserObjectFactoryInterface::deserializationFactory(&user_types::UserObjectFactory::getInstance())) };

	std::vector<core::SEditorObject> instances{};
	std::map<std::string, core::SEditorObject> instanceMap;
	instances.reserve(result.objectsDeserialization.objects.size());
	for (auto& d : result.objectsDeserialization.objects) {
		auto obj = std::dynamic_pointer_cast<core::EditorObject>(d);
		instances.push_back(obj);
		instanceMap[obj->objectID()] = obj;
	}
	for (const auto& pair : result.objectsDeserialization.references) {
		if (instanceMap.find(pair.second) != instanceMap.end()) {
			*pair.first = instanceMap.at(pair.second);
		} else {
			LOG_WARNING(raco::log_system::PROJECT, "Load: referenced object not found: {}", pair.second);
		}
	}

	Project p{ instances };
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
		pathStack });
}

QString RaCoProject::name() const {
	return QString::fromStdString(project_.settings()->objectName());
}

QJsonDocument RaCoProject::serializeProject(const std::unordered_map<std::string, std::vector<int>>& currentVersions) {
	const auto& instances{context_->project()->instances()};
	std::vector<std::shared_ptr<ReflectionInterface>> instancesInterface{instances.begin(), instances.end()};
	const auto& links{context_->project()->links()};
	std::vector<std::shared_ptr<ReflectionInterface>> linksInterface{links.begin(), links.end()};

	return serialization::serializeProject(
		currentVersions,
		instancesInterface, linksInterface,
		project_.externalProjectsMap(),
		[](const raco::data_storage::ValueBase& value) -> std::optional<std::string> {
			if (value.asRef()) {
				return value.asRef()->objectID();
			} else {
				return {};
			}
		});
}

bool RaCoProject::save() {
	const auto path(project_.currentPath());
	LOG_INFO(raco::log_system::PROJECT, "Saving project to {}", path);
	QFile file{path.c_str()};
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		LOG_ERROR(raco::log_system::PROJECT, "Saving project failed: Could not open file for writing: {} FileError {} {}", path, file.error(), file.errorString().toStdString());
		return false;
	}

	auto ramsesVersion = raco::ramses_base::getRamsesVersion();
	auto ramsesLogicEngineVersion = raco::ramses_base::getLogicEngineVersion();

	std::unordered_map<std::string, std::vector<int>> currentVersions = {
		{raco::serialization::keys::FILE_VERSION, {raco::core::RAMSES_PROJECT_FILE_VERSION}},
		{raco::serialization::keys::RAMSES_VERSION, {ramsesVersion.major, ramsesVersion.minor, ramsesVersion.patch}},
		{raco::serialization::keys::RAMSES_LOGIC_ENGINE_VERSION, {static_cast<int>(ramsesLogicEngineVersion.major), static_cast<int>(ramsesLogicEngineVersion.minor), static_cast<int>(ramsesLogicEngineVersion.patch)}},
		{raco::serialization::keys::RAMSES_COMPOSER_VERSION, {RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH}}};
	file.write(serializeProject(currentVersions).toJson());
	if (!file.flush() || file.error() != QFile::FileError::NoError) {
		LOG_ERROR(raco::log_system::PROJECT, "Saving project failed: Could not write to disk: FileError {} {}", file.error(), file.errorString().toStdString());
		file.close();
		return false;
	}
	file.close();
	generateAllProjectSubfolders();

	const auto& prefs = raco::components::RaCoPreferences::instance();
	PathManager::setAllCachedPathRoots(project_.currentFolder(),
		prefs.imageSubdirectory.toStdString(),
		prefs.meshSubdirectory.toStdString(),
		prefs.scriptSubdirectory.toStdString(),
		prefs.shaderSubdirectory.toStdString());

	dirty_ = false;
	LOG_INFO(raco::log_system::PROJECT, "Finished saving project to {}", path);
	return true;
}

bool RaCoProject::saveAs(const QString& fileName, bool setProjectName) {
	auto oldPath = project_.currentPath();
	auto oldProjectFolder = project_.currentFolder();
	auto newPath = fileName.toStdString();
	if (newPath == oldPath) {
		return save();
	} else {
		project_.setCurrentPath(newPath);
		onAfterProjectPathChange(oldProjectFolder, project_.currentFolder());
		if (setProjectName) {
			auto projName = std::filesystem::path(newPath).stem().string();
			auto settings = project_.settings();
			if (settings->objectName().empty()) {
				context_->set({settings, &raco::core::EditorObject::objectName_}, projName);
			}
		}
		if (save()) {
			undoStack_.reset();
			dirty_ = false;
			return true;
		} else {
			return false;
		}
			
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