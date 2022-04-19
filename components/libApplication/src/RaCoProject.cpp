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
#include "core/ProxyObjectFactory.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "ramses_base/BaseEngineBackend.h"
#include "components/Naming.h"
#include "application/RaCoApplication.h"
#include "components/RaCoPreferences.h"
#include "core/ProjectMigration.h"
#include "core/Serialization.h"
#include "core/SerializationKeys.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
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
#include "utils/ZipUtils.h"
#include "utils/u8path.h"
#include "core/CoreFormatter.h"
#include "utils/stdfilesystem.h"
#include "core/PrefabOperations.h"
#include "user_types/PrefabInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>
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
	  meshCache_{app->meshCache()} {
	context_->setMeshCache(meshCache_);
	context_->setExternalProjectsStore(externalProjectsStore);

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

	context_->performExternalFileReload(project_.instances());

	// Push currently loading project on the project load stack to enable project loop detection to work.
	pathStack.emplace_back(file.toStdString());
	context_->updateExternalReferences(pathStack);
	pathStack.pop_back();

	undoStack_.reset();
	context_->changeMultiplexer().reset();

	if (!project_.currentFileName().empty()) {
		updateActiveFileListener();
	}
	dirty_ = false;
}

void RaCoProject::onAfterProjectPathChange(const std::string& oldPath, const std::string& newPath) {
	// Somewhat outdated description of a problem here:
	// We need the LuaScripts to be processed before the LuaScriptModules:
	// otherwise we will loose the references to the modules in the LuaScript module properties.
	// Mechanism for losing the module references:
	// - reroot / set LuaModule uri property
	// - call LuaScript::onAfterReferencedObjectChanged as side effect of operation above
	// - LuaScript module sync will now remove the module properties because the lua file can't be loaded because 
	//   the LuaScript uri property has not been rerooted yet but is resolved using the new project path.
	// - reroot / set LuaScript uri property
	//   restores the module properties again but the reference values are now lost (not anymore, caching works again)
	// Solutions
	// - get module caching in LuaScript working again (this now works again)
	// - set all uri properties before calling the side effect handlers; problematic because we would be opening the
	//   BaseContext::set function and reordering the steps it takes. might be necessary though.
	// update: we now have module caching in the LuaScript working again;
	// but: we still potentially call some callback handlers multiple times; 
	// it would be nice the optimize this but that needs an extension of the second solution above.
	auto instances{context_->project()->instances()};
	for (auto& object : instances) {
		if (PathQueries::isPathRelativeToCurrentProject(object)) {
			for (const auto& property : ValueTreeIteratorAdaptor(ValueHandle{object})) {
				if (auto anno = property.query<URIAnnotation>(); anno && !anno->isProjectSubdirectoryURI()) {
					auto uriPath = property.asString();
					if (!uriPath.empty() && raco::utils::u8path(uriPath).is_relative()) {
						context_->set(property, raco::utils::u8path(uriPath).rerootRelativePath(oldPath, newPath).string());
					}
				}
			}
		}
	}
	project_.rerootExternalProjectPaths(oldPath, newPath);
	updateActiveFileListener();
}

void RaCoProject::generateProjectSubfolder(const std::string& subFolderPath) {
	auto path = raco::utils::u8path(subFolderPath).normalizedAbsolutePath(project_.currentFolder());
	if (!path.existsDirectory()) {
		std::filesystem::create_directories(path);
	}
}

void RaCoProject::generateAllProjectSubfolders() {
	const auto& settings = project_.settings();
	generateProjectSubfolder(settings->defaultResourceDirectories_->imageSubdirectory_.asString());
	generateProjectSubfolder(settings->defaultResourceDirectories_->meshSubdirectory_.asString());
	generateProjectSubfolder(settings->defaultResourceDirectories_->scriptSubdirectory_.asString());
	generateProjectSubfolder(settings->defaultResourceDirectories_->shaderSubdirectory_.asString());

	applyDefaultCachedPaths();
}

void RaCoProject::applyDefaultCachedPaths() {
	auto settings = project_.settings();
	auto defaultFolders = settings->defaultResourceDirectories_;

	auto projectFolder = project_.currentFolder();
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Project, projectFolder);
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Image, raco::utils::u8path(defaultFolders->imageSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Mesh, raco::utils::u8path(defaultFolders->meshSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Script, raco::utils::u8path(defaultFolders->scriptSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Shader, raco::utils::u8path(defaultFolders->shaderSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
}

void RaCoProject::subscribeDefaultCachedPathChanges(const raco::components::SDataChangeDispatcher& dataChangeDispatcher) {
	auto project = this->project();
	auto settings = project_.settings();

	imageSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::imageSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->imageSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Image, path);
		});

	meshSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::meshSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->meshSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Mesh, path);
		});

	scriptSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::scriptSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->scriptSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Script, path);
		});

	shaderSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::shaderSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->shaderSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Shader, path);
		});
}

void RaCoProject::updateActiveFileListener() {
	activeProjectFileChangeListener_ = activeProjectFileChangeMonitor_.registerFileChangedHandler(project_.currentPath(),
		[this]() {
			Q_EMIT activeProjectFileChanged();
		});
}

RaCoProject::~RaCoProject() {
	for (const auto& instance : project_.instances()) {
		instance->onBeforeDeleteObject(errors_);
	}
}

std::unique_ptr<RaCoProject> RaCoProject::createNew(RaCoApplication* app) {
	LOG_INFO(raco::log_system::PROJECT, "Creating new project.");
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

	const auto& prefs = raco::components::RaCoPreferences::instance();
	auto settings = result->context_->createObject(ProjectSettings::typeDescription.typeName);
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::imageSubdirectory_}, prefs.imageSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::meshSubdirectory_}, prefs.meshSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::scriptSubdirectory_}, prefs.scriptSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::shaderSubdirectory_}, prefs.shaderSubdirectory.toStdString());

	result->context_->set({sRenderPass, &user_types::RenderPass::camera_}, sCamera);
	result->context_->set({sRenderPass, &user_types::RenderPass::layer0_}, sRenderLayer);
	result->context_->addProperty({sRenderLayer, &user_types::RenderLayer::renderableTags_}, "render_main", std::make_unique<data_storage::Value<int>>(0));

	result->context_->set({sNode, &user_types::Node::tags_}, std::vector<std::string>({"render_main"}));
	result->context_->set({sCamera, &user_types::Node::translation_, &Vec3f::z}, 10.0);
	result->context_->moveScenegraphChildren({sMeshNode}, sNode);
	result->undoStack_.reset();
	result->context_->changeMultiplexer().reset();
	result->dirty_ = false;

	Consistency::checkProjectSettings(*result->context_->project());

	return result;
}

std::unique_ptr<RaCoProject> RaCoProject::loadFromFile(const QString& filename, RaCoApplication* app, std::vector<std::string>& pathStack) {
	LOG_INFO(raco::log_system::PROJECT, "Loading project from {}", filename.toLatin1());

	if (!raco::utils::u8path(filename.toStdString()).existsFile()) {
		LOG_WARNING(raco::log_system::PROJECT, "File not found {}", filename.toLatin1());
		return {};
	}

	QFile file{filename};
	if (!file.open(QIODevice::ReadOnly)) {
		LOG_WARNING(raco::log_system::PROJECT, "Can't read file {}", filename.toLatin1());
		return {};
	}

	auto fileContents = file.readAll();
	file.close();

	if (fileContents.size() < 4) {
		LOG_WARNING(raco::log_system::PROJECT, "File {} has invalid content", filename.toLatin1());
		return {};
	}

	if (raco::utils::zip::isZipFile({fileContents.begin(), fileContents.begin() + 4})) {
		auto unzippedProj = raco::utils::zip::zipToProject(fileContents, fileContents.size());

		if (unzippedProj.success) {
			fileContents = QByteArray(unzippedProj.payload.c_str());
		} else {
			throw std::runtime_error(fmt::format("Can't read zipped file {}:\n{}", filename.toLatin1(), unzippedProj.payload));
		}
	}

	auto document{QJsonDocument::fromJson(fileContents)};
	if (document.isNull()) {
		throw std::runtime_error("Loading JSON file resulted in a null document object");
	}

	auto fileVersion{raco::serialization::deserializeFileVersion(document)};
	if (fileVersion > raco::serialization::RAMSES_PROJECT_FILE_VERSION) {
		throw FutureFileVersion{fileVersion};
	}

	auto result{raco::serialization::deserializeProject(document, filename.toStdString())};

	Project p{result.objects};
	p.setCurrentPath(filename.toStdString());
	for (const auto& instance : result.objects) {
		instance->onAfterDeserialization();
	}
	for (const auto& link : result.links) {
		p.addLink(link);
	}
	for (auto [id, info] : result.externalProjectsMap) {
		auto absPath = raco::utils::u8path(info.path).normalizedAbsolutePath(p.currentFolder());
		p.addExternalProjectMapping(id, absPath.string(), info.name);
	}
	LOG_INFO(raco::log_system::PROJECT, "Finished loading project from {}", filename.toLatin1());

	Consistency::checkProjectSettings(p);

	auto newProject = new RaCoProject{
		filename,
		p,
		app->engine(),
		[app]() { app->dataChangeDispatcher()->setUndoChanged(); },
		app->externalProjects(),
		app,
		pathStack};

	for (const auto& [objectID, infoMessage] : result.migrationObjWarnings) {
		if (const auto migratedObj = newProject->project()->getInstanceByID(objectID)) {
			newProject->errors()->addError(raco::core::ErrorCategory::MIGRATION_ERROR, ErrorLevel::WARNING, migratedObj, infoMessage);
		}
	}

	newProject->errors()->logAllErrors();

	return std::unique_ptr<RaCoProject>(newProject);
}

QString RaCoProject::name() const {
	return QString::fromStdString(project_.settings()->objectName());
}

QJsonDocument RaCoProject::serializeProject(const std::unordered_map<std::string, std::vector<int>>& currentVersions) {
	// Sort instances and links serialized to file:
	// needed for file comparison via diff.
	auto instances{context_->project()->instances()};
	std::sort(instances.begin(), instances.end(), [](const SEditorObject& left, const SEditorObject& right) {
		return left->objectID() < right->objectID();
	});

	auto links{context_->project()->links()};
	std::sort(links.begin(), links.end(), [](const SLink& left, const SLink& right) {
		return LinkDescriptor::lessThanByObjectID(left->descriptor(), right->descriptor());
	});

	std::vector<std::shared_ptr<ReflectionInterface>> instancesInterface{instances.begin(), instances.end()};
	std::vector<std::shared_ptr<ReflectionInterface>> linksInterface{links.begin(), links.end()};

	return serialization::serializeProject(
		currentVersions,
		instancesInterface, linksInterface,
		project_.externalProjectsMap());
}

bool RaCoProject::save() {
	const auto path(project_.currentPath());
	LOG_INFO(raco::log_system::PROJECT, "Saving project to {}", path);
	QFile file{path.c_str()};
	auto settings = project_.settings();
	auto saveAsZip = *settings->saveAsZip_;
	auto flags = (saveAsZip) ? QIODevice::WriteOnly : QIODevice::WriteOnly | QIODevice::Text;

	if (!file.open(flags)) {
		LOG_ERROR(raco::log_system::PROJECT, "Saving project failed: Could not open file for writing: {} FileError {} {}", path, file.error(), file.errorString().toStdString());
		return false;
	}

	auto ramsesVersion = raco::ramses_base::getRamsesVersion();
	auto ramsesLogicEngineVersion = raco::ramses_base::getLogicEngineVersion();

	std::unordered_map<std::string, std::vector<int>> currentVersions = {
		{raco::serialization::keys::FILE_VERSION, {raco::serialization::RAMSES_PROJECT_FILE_VERSION}},
		{raco::serialization::keys::RAMSES_VERSION, {ramsesVersion.major, ramsesVersion.minor, ramsesVersion.patch}},
		{raco::serialization::keys::RAMSES_LOGIC_ENGINE_VERSION, {static_cast<int>(ramsesLogicEngineVersion.major), static_cast<int>(ramsesLogicEngineVersion.minor), static_cast<int>(ramsesLogicEngineVersion.patch)}},
		{raco::serialization::keys::RAMSES_COMPOSER_VERSION, {RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH}}};
	auto projectFileData = serializeProject(currentVersions).toJson();

	if (saveAsZip) {
		auto zippedFile = (raco::utils::zip::projectToZip(projectFileData.constData(), (project_.currentFileName() + ".json").c_str()));
		if (zippedFile.empty()) {
			LOG_ERROR(raco::log_system::PROJECT, "Saving project failed: Error while zipping project file");
			return false;
		}

		projectFileData.resize(zippedFile.size());

		std::memcpy(projectFileData.data(), zippedFile.data(), zippedFile.size());
	}

	file.write(projectFileData);
	if (!file.flush() || file.error() != QFile::FileError::NoError) {
		LOG_ERROR(raco::log_system::PROJECT, "Saving project failed: Could not write to disk: FileError {} {}", file.error(), file.errorString().toStdString());
		file.close();
		return false;
	}
	file.close();

	generateAllProjectSubfolders();

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
			auto projName = raco::utils::u8path(newPath).stem().string();
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

UndoStack* RaCoProject::undoStack() {
	return &undoStack_;
}

MeshCache* RaCoProject::meshCache() {
	return meshCache_;
}

}  // namespace raco::application