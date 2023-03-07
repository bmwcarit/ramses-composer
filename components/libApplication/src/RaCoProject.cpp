/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "components/RaCoNameConstants.h"
#include "components/TracePlayer.h"
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
#include <filesystem>
#include "core/PrefabOperations.h"
#include "user_types/PrefabInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>
#include <functional>


namespace raco::application {

using namespace raco::core;

RaCoProject::RaCoProject(const QString& file, Project& p, EngineInterface* engineInterface, const UndoStack::Callback& callback, ExternalProjectsStoreInterface* externalProjectsStore, RaCoApplication* app, LoadContext& loadContext)
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
	for (const auto& object : project_.instances()) {
		if (object->isType<user_types::RenderPass>() &&
			object->query<raco::core::ExternalReferenceAnnotation>()) {
			throw std::runtime_error("project contains external reference RenderPass.");
		}

		if ((object->isType<user_types::PerspectiveCamera>() || object->isType<user_types::OrthographicCamera>()) &&
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

	// Create creation records for all PrefabInstances to force update of their children:
	// This is necessary since we don't save all the children of the PrefabInstances anymore.
	for (auto object : project_.instances()) {
		if (&object->getTypeDescription() == &user_types::PrefabInstance::typeDescription) {
			context_->modelChanges().recordCreateObject(object);
		}
	}
	context_->performExternalFileReload(project_.instances());

	// Push currently loading project on the project load stack to enable project loop detection to work.
	loadContext.pathStack.emplace_back(file.toStdString());
	context_->updateExternalReferences(loadContext);
	loadContext.pathStack.pop_back();

	undoStack_.reset();
	context_->changeMultiplexer().reset();

	if (!project_.currentFileName().empty()) {
		updateActiveFileListener();
	}
	dirty_ = false;

	tracePlayer_ = std::make_unique<raco::components::TracePlayer>(*project(), context_->uiChanges(), *undoStack());
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
	auto instances{project_.instances()};
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
	undoStack_.push("Rewrite uri paths.");
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
	generateProjectSubfolder(settings->defaultResourceDirectories_->interfaceSubdirectory_.asString());
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
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Interface, raco::utils::u8path(defaultFolders->interfaceSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
	PathManager::setCachedPath(PathManager::FolderTypeKeys::Shader, raco::utils::u8path(defaultFolders->shaderSubdirectory_.asString()).normalizedAbsolutePath(projectFolder));
}

void RaCoProject::setupCachedPathSubscriptions(const raco::components::SDataChangeDispatcher& dataChangeDispatcher) {
	lifecycleSubscription_ = dataChangeDispatcher->registerOnObjectsLifeCycle([this, dataChangeDispatcher](SEditorObject obj) {
		if (obj->isType<ProjectSettings>()) {
			subscribeDefaultCachedPathChanges(dataChangeDispatcher);
		} 
	}, 
	[](auto) {});
	subscribeDefaultCachedPathChanges(dataChangeDispatcher);
}

void RaCoProject::subscribeDefaultCachedPathChanges(const raco::components::SDataChangeDispatcher& dataChangeDispatcher) {
	auto project = this->project();
	auto settings = project_.settings();

	imageSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::imageSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->imageSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Image, path);
		});

	meshSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::meshSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->meshSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Mesh, path);
		});

	scriptSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::scriptSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->scriptSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Script, path);
		});

	interfaceSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::interfaceSubdirectory_},
		[project, settings]() {
			auto path = raco::utils::u8path(settings->defaultResourceDirectories_->interfaceSubdirectory_.asString()).normalizedAbsolutePath(project->currentFolder());
			PathManager::setCachedPath(PathManager::FolderTypeKeys::Interface, path);
		});

	shaderSubdirectoryUpdateSubscription_ = dataChangeDispatcher->registerOn({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::shaderSubdirectory_},
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
		instance->onBeforeDeleteObject(*context_);
	}
}

std::unique_ptr<RaCoProject> RaCoProject::createNew(RaCoApplication* app, bool createDefaultScene, int featureLevel) {
	LOG_INFO(raco::log_system::PROJECT, "Creating new project.");
	Project p{};
	p.setCurrentPath(components::RaCoPreferences::instance().userProjectsDirectory.toStdString());

	if (featureLevel == -1) {
		featureLevel = static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel);
	}

	LoadContext loadContext;
	loadContext.featureLevel = featureLevel;
	auto result = std::unique_ptr<RaCoProject>(new RaCoProject(
		QString{},
		p,
		app->engine(),
		[app]() { app->dataChangeDispatcher()->setUndoChanged(); },
		app->externalProjects(),
		app,
		loadContext));

	const auto& prefs = raco::components::RaCoPreferences::instance();
	auto settings = result->context_->createObject(ProjectSettings::typeDescription.typeName);
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::imageSubdirectory_}, prefs.imageSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::meshSubdirectory_}, prefs.meshSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::scriptSubdirectory_}, prefs.scriptSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::interfaceSubdirectory_}, prefs.interfaceSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::shaderSubdirectory_}, prefs.shaderSubdirectory.toStdString());
	result->context_->set({settings, &ProjectSettings::featureLevel_}, featureLevel);

	if (createDefaultScene) {
		auto sMeshNode = result->context_->createObject(raco::user_types::MeshNode::typeDescription.typeName, raco::components::Naming::format(raco::user_types::MeshNode::typeDescription.typeName));
		auto sNode = result->context_->createObject(raco::user_types::Node::typeDescription.typeName, raco::components::Naming::format(raco::user_types::Node::typeDescription.typeName))->as<user_types::Node>();
		auto sCamera = result->context_->createObject(raco::user_types::PerspectiveCamera::typeDescription.typeName, raco::components::Naming::format(raco::user_types::PerspectiveCamera::typeDescription.typeName))->as<user_types::PerspectiveCamera>();
		auto sRenderPass = result->context_->createObject(raco::user_types::RenderPass::typeDescription.typeName, "MainRenderPass")->as<user_types::RenderPass>();
		auto sRenderLayer = result->context_->createObject(raco::user_types::RenderLayer::typeDescription.typeName, "MainRenderLayer")->as<user_types::RenderLayer>();

		result->context_->set({sRenderPass, &user_types::RenderPass::camera_}, sCamera);
		result->context_->set({sRenderPass, &user_types::RenderPass::layer0_}, sRenderLayer);
		result->context_->addProperty({sRenderLayer, &user_types::RenderLayer::renderableTags_}, "render_main", std::make_unique<data_storage::Property<int, LinkEndAnnotation>>(0, LinkEndAnnotation(3)));

		result->context_->set({sNode, &user_types::Node::tags_}, std::vector<std::string>({"render_main"}));
		result->context_->set({sCamera, &user_types::Node::translation_, &Vec3f::z}, 10.0);
		result->context_->moveScenegraphChildren({sMeshNode}, sNode);
	}

	result->undoStack_.reset();
	result->context_->changeMultiplexer().reset();
	result->dirty_ = false;

	Consistency::checkProjectSettings(result->project_);

	return result;
}

std::unique_ptr<RaCoProject> RaCoProject::loadFromFile(const QString& filename, RaCoApplication* app, LoadContext& loadContext, bool logErrors, int featureLevel, bool generateNewObjectIDs) {
	LOG_INFO(raco::log_system::PROJECT, "Loading project from {}", filename.toLatin1());

	QFileInfo path(filename);
	QString absPath = path.absoluteFilePath();
	if (path.suffix().compare(raco::names::PROJECT_FILE_EXTENSION, Qt::CaseInsensitive) != 0) {
		throw std::runtime_error(fmt::format("Load file: wrong extension {}", path.filePath().toStdString()));
	}

	if (!raco::utils::u8path(absPath.toStdString()).existsFile()) {
		throw std::runtime_error(fmt::format("File not found {}", absPath.toLatin1()));
	}

	if (!raco::utils::u8path(absPath.toStdString()).userHasReadAccess()) {
		throw std::runtime_error(fmt::format("Project file could not be read {}", absPath.toLatin1()));
	}

	QFile file{absPath};
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error(fmt::format("Error opening file {}", absPath.toLatin1()));
	}

	auto fileContents = file.readAll();
	file.close();

	if (fileContents.size() < 4) {
		throw std::runtime_error(fmt::format("File {} has invalid content", absPath.toLatin1()));
	}

	if (raco::utils::zip::isZipFile({fileContents.begin(), fileContents.begin() + 4})) {
		auto unzippedProj = raco::utils::zip::zipToProject(fileContents, fileContents.size());

		if (unzippedProj.success) {
			fileContents = QByteArray(unzippedProj.payload.c_str());
		} else {
			throw std::runtime_error(fmt::format("Can't read zipped file {}:\n{}", absPath.toLatin1(), unzippedProj.payload));
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
	if (fileVersion == 0) {
		throw std::runtime_error("File is not a RamsesComposer file.");
	}

	auto result{raco::serialization::deserializeProject(document, absPath.toStdString())};

	for (const auto& instance : result.objects) {
		instance->onAfterDeserialization();
	}

	// Ordering constraint: generateNewObjectIDs needs the pointers created by the onAfterDeserialization handlers above.
	if (generateNewObjectIDs) {
		BaseContext::generateNewObjectIDs(result.objects);
	}

	Project p{result.objects};
	p.setCurrentPath(absPath.toStdString());
	for (const auto& link : result.links) {
		p.addLink(link);
	}
	for (auto [id, info] : result.externalProjectsMap) {
		auto absPath = raco::utils::u8path(info.path).normalizedAbsolutePath(p.currentFolder());
		p.addExternalProjectMapping(id, absPath.string(), info.name);
	}
	if (featureLevel != -1) {
		if (featureLevel >= p.featureLevel() && featureLevel <= static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel)) {
			p.setFeatureLevel(featureLevel);
		} else {
			if (featureLevel < p.featureLevel()) {
				throw FeatureLevelLoadError(fmt::format("New Feature level {} smaller than project feature level {}.", featureLevel, p.featureLevel()),
					featureLevel, p.featureLevel(), absPath.toStdString());
			} else {
				throw std::runtime_error(fmt::format("RamsesLogic feature level {} outside valid range ({} ... {})", featureLevel, static_cast<int>(raco::ramses_base::BaseEngineBackend::minFeatureLevel), static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel)));
			}
		}
	}

	if (loadContext.featureLevel == -1) {
		loadContext.featureLevel = p.featureLevel();
	}

	Consistency::checkProjectSettings(p);

	auto newProject = new RaCoProject{
		absPath,
		p,
		app->engine(),
		[app]() { app->dataChangeDispatcher()->setUndoChanged(); },
		app->externalProjects(),
		app,
		loadContext};

	for (const auto& [objectID, infoMessage] : result.migrationObjWarnings) {
		if (const auto migratedObj = newProject->project()->getInstanceByID(objectID)) {
			newProject->errors()->addError(raco::core::ErrorCategory::MIGRATION, ErrorLevel::WARNING, migratedObj, infoMessage);
		}
	}

	// Selectively log errors:
	// - we need to log errors when loading external projects here
	// - we don't need to log errors here when called from RaCoApplication::switchActiveRacoProject; see comment there
	if (logErrors) {
		newProject->errors()->logAllErrors();
	}

	LOG_INFO(raco::log_system::PROJECT, "Finished loading project from {}", absPath.toLatin1());

	return std::unique_ptr<RaCoProject>(newProject);
}

QString RaCoProject::name() const {
	return QString::fromStdString(project_.settings()->objectName());
}


QJsonDocument RaCoProject::serializeProjectData(const std::unordered_map<std::string, std::vector<int>>& currentVersions) {
	// Sort instances and links serialized to file:
	// needed for file comparison via diff.
	auto instances{project_.instances()};
	std::sort(instances.begin(), instances.end(), [](const SEditorObject& left, const SEditorObject& right) {
		return left->objectID() < right->objectID();
	});

	std::vector<SLink> links;
	std::copy(project_.links().begin(), project_.links().end(), std::back_inserter(links));
	std::sort(links.begin(), links.end(), [](const SLink& left, const SLink& right) {
		return LinkDescriptor::lessThanByObjectID(left->descriptor(), right->descriptor());
	});

	std::vector<std::shared_ptr<ReflectionInterface>> instancesInterface{instances.begin(), instances.end()};
	std::vector<std::shared_ptr<ReflectionInterface>> linksInterface{links.begin(), links.end()};

	return serialization::serializeProject(
		currentVersions,
		project_.featureLevel(),
		instancesInterface, linksInterface,
		project_.externalProjectsMap());
}

QJsonDocument RaCoProject::serializeProject(const std::unordered_map<std::string, std::vector<int>>& currentVersions) {
	// On discarding objects during save
	// - we discard all objects which can not be recreated by the external reference and/or prefab update during load.
	//
	// The rules in detail
	// - local PrefabInstance children are discarded
	//   - except the modifiable lua interface scripts
	//   - except cameras and their scenegraph parent chain up to the scenegraph root.
	//     these can be referenced by render passes
	// - all extref objects are removed
	//   - except objects directly referenced by a local non-discarded object
	//     this exception applies to extref prefabs used by local prefab instances.
	// 	   this exception does not apply to meshes referenced by meshnodes inside local prefabinstances.
	//   - except objects which have a link starting on them and the link endpoint is a local object.
	//     this applies to extref lua scripts linked to local material uniforms.
	// - implications
	//   - all unused extref objects are removed during save
	//   - extref scenes only indirectly used are removed from the external projects mapping in the saved file.
	// - rationale for the rules:
	//	 - we rely on the extref and prefab update during load to recreate objects and links. we can only discard
	//     objects or links which can be fully recreated.
	//   - an object can't be discarded if it is referenced by a non-discarded object or part of a non-discarded link.
	// 	   otherwise we would loose the references in the saved file.

	// Implementation note: we destructively delete the discarded objects and links here and restore them after serialization
	// of the project from the current undo stack entry.

	// Reset model changes to allow data loss detection below.
	// This is safe since the undo stack restore below would do this anyway and there is no undo stack push in here.
	context_->modelChanges().reset();

	// reconstrucible properties
	// - all properties of extref objects
	// - children property of prefab instances
	// - all properties of prefab instance children except interface properties of interface lua scripts
	std::function<bool(const ValueHandle&)> canReconstructPropertyPredicate = [](const ValueHandle& prop) -> bool {
		auto obj = prop.rootObject();
		if (obj->query<ExternalReferenceAnnotation>()) {
			return true;
		}

		if (obj->as<user_types::PrefabInstance>() && (prop.isRefToProp(&EditorObject::children_) || ValueHandle(obj, &EditorObject::children_).contains(prop))) {
			return true;
		}

		if (PrefabOperations::findContainingPrefabInstance(obj->getParent()) && !PrefabOperations::isInterfaceProperty(prop)) {
			return true;
		}

		return false;
	};

	// reconstructible links
	// - links ending on extref objects
	// - links ending on prefab instance children except the interface lua scripts
	std::function<bool(const LinkDescriptor&)> canReconstructLinkPredicate = [](const LinkDescriptor& link) {
		auto endObj = link.end.object();

		if (endObj->query<ExternalReferenceAnnotation>()) {
			return true;
		}

		if (PrefabOperations::findContainingPrefabInstance(endObj->getParent()) && !PrefabOperations::isInterfaceObject(endObj)) {
			return true;
		}

		return false;
	};

	{
		std::vector<SEditorObject> instances{project_.instances()};

		auto partitionPoint = instances.begin();

		// Partition all instances with discardable objects moved to the front

		// Initial partition: separate objects which are discardable in principle
		// Discardable in prinple are
		// - extref objects
		// - children of prefab instances but only if read-only, i.e. no interface lua scripts
		auto newPartitionPoint = std::partition(instances.begin(), instances.end(), [](SEditorObject object) -> bool {
			return (PrefabOperations::findContainingPrefabInstance(object->getParent()) && Queries::isReadOnly(object)) ||
				   object->query<ExternalReferenceAnnotation>();
		});

		// Iteratively remove objects from the discardable set until we achieved a stable discardable set
		while (newPartitionPoint != partitionPoint && newPartitionPoint != instances.begin()) {
			partitionPoint = newPartitionPoint;

			newPartitionPoint = std::partition(instances.begin(), partitionPoint, [this, partitionPoint, &instances, &canReconstructPropertyPredicate, &canReconstructLinkPredicate](SEditorObject object) -> bool {

				// Can't discard object when a child is non-discarded and object is inside a prefab instance
				// since the prefab update needs to determine the set of current prefab instance children
				if (PrefabOperations::findContainingPrefabInstance(object->getParent())) {
					for (const auto& child : object->children_->asVector<SEditorObject>()) {
						if (std::find(partitionPoint, instances.end(), child) != instances.end()) {
							return false;
						}
					}
				}

				// Can't discard object when referenced by non-discarded object via a non-reconstructible property.
				for (const auto& weakReferencingObj : object->referencesToThis()) {
					auto referencingObj = weakReferencingObj.lock();
					if (referencingObj && std::find(partitionPoint, instances.end(), referencingObj) != instances.end()) {
						for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(referencingObj))) {
							if (prop.type() == PrimitiveType::Ref) {
								if (object == prop.asTypedRef<EditorObject>() && !canReconstructPropertyPredicate(prop)) {
									return false;
								}
							}
						}
					}
				}

				// Can't discard LuaScriptModules which are used since this might lead to a LuaScript to lose its
				// properties as a side effect of the reference removal which is not allowed for interface lua scripts.
				// This needs to take into account potential nesting of modules so it is not sufficient to check if
				// the referencing object is an interface lua script.
				if (object->as<user_types::LuaScriptModule>()) {
					if (!object->referencesToThis().empty()) {
						return false;
					}
				}

				// Can't discard object if connected to non-reconstructible link
				for (const auto& link : Queries::getLinksConnectedToObject(project_, object, true, true)) {
					if (!canReconstructLinkPredicate(link->descriptor())) {
						return false;
					}
				}

				return true;
			});
		}

		std::vector<SEditorObject> toDelete{instances.begin(), newPartitionPoint};
		context_->deleteObjects(toDelete);

		project_.gcExternalProjectMapping();
	}


	auto serializedJson = serializeProjectData(currentVersions);


	// Check for non-recoverable data loss arising from the object deletions above
	bool lostData = false;

	// Check if we lost any non-reconstructible property values
	for (const auto& [id, handles] : context_->modelChanges().getChangedValues()) {
		for (const auto& handle : handles) {
			if (!canReconstructPropertyPredicate(handle)) {
				LOG_WARNING(raco::log_system::PROJECT, "serializeProject: optimization changed property {}; using non-optimized fallback instead.", handle);
				lostData = true;
			}
		}
	}

	// Check if we lost any non-reconstructible links
	for (const auto& [id, links] : context_->modelChanges().getRemovedLinks()) {
		for (const auto& link : links) {
			// Only consider links ending on objects not deleted, i.e. objects which can still be found in the project:
			auto endID = link.end.object()->objectID();
			auto endObj = project_.getInstanceByID(endID);
			if (endObj && !canReconstructLinkPredicate(link)) {
				LOG_WARNING(raco::log_system::PROJECT, "serializeProject: optimization removed link {}; using non-optimized fallback instead.", link);
				lostData = true;
			}
		}
	}


	// Force restoring the project from the current undo stack entry.
	undoStack_.setIndex(undoStack_.getIndex(), true);


#ifdef NDEBUG
	// If optimization leads to data loss, return unoptimized json instead:
	if (lostData) {
		serializedJson = serializeProjectData(currentVersions);
	}
#else
	// In a debug build data loss will abort to allow the unit tests to have a chance at detecting problems.
	assert(!lostData);
#endif

	return serializedJson;
}

bool RaCoProject::save(std::string& outError) {
	outError.clear();
	const auto path(project_.currentPath());
	LOG_INFO(raco::log_system::PROJECT, "Saving project to {}", path);
	QFile file{path.c_str()};
	auto settings = project_.settings();
	auto saveAsZip = *settings->saveAsZip_;
	auto flags = (saveAsZip) ? QIODevice::WriteOnly : QIODevice::WriteOnly | QIODevice::Text;

	if (!file.open(flags)) {
		std::string msg = fmt::format("Saving project failed: Could not open file for writing: {} FileError {} {}", path, file.error(), file.errorString().toStdString());
		LOG_ERROR(raco::log_system::PROJECT, msg);
		outError = msg;
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
		std::string msg = fmt::format("Saving project failed: Could not open file for writing: {} FileError {} {}", path, file.error(), file.errorString().toStdString());
		LOG_ERROR(raco::log_system::PROJECT, msg);
		outError = msg;
		return false;
	}
	file.close();

	generateAllProjectSubfolders();

	dirty_ = false;
	LOG_INFO(raco::log_system::PROJECT, "Finished saving project to {}", path);
	Q_EMIT projectSuccessfullySaved();
	return true;
}

bool RaCoProject::saveAs(const QString& fileName, std::string& outError, bool setProjectName) {
	auto oldPath = project_.currentPath();
	auto oldProjectFolder = project_.currentFolder();
	QString absPath = QFileInfo(fileName).absoluteFilePath();
	auto newPath = absPath.toStdString();
	if (newPath == oldPath) {
		return save(outError);
	} else {
		// Note on the undo stack:
		// The project name change will be recorded in the change recorder by the context operations, but no undo stack entry will be created.
		// But we need an undo stack entry since the save below wil perform a restore from the undo stack.
		// However the onAfterProjectPathChange will perform an unconditional undo stack push.
		// This means that using the context in the operations below is OK as long as we perform the onAfterProjectPathChange afterwards.
		if (setProjectName) {
			auto projName = raco::utils::u8path(newPath).stem().string();
			auto settings = project_.settings();
			if (settings->objectName().empty()) {
				context_->set({settings, &raco::core::EditorObject::objectName_}, projName);
			}
		}
		project_.setCurrentPath(newPath);
		// Note: this will perform an undo stack push
		onAfterProjectPathChange(oldProjectFolder, project_.currentFolder());
		// Note: save will perform a restore from the undo stack as part of the file save optimization
		if (save(outError)) {
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

void RaCoProject::updateExternalReferences(core::LoadContext& loadContext) {
	context_->updateExternalReferences(loadContext);
}

Project* RaCoProject::project() {
	return &project_;
}

Errors* RaCoProject::errors() {
	return &errors_;
}

const Errors* RaCoProject::errors() const {
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

raco::components::TracePlayer& RaCoProject::tracePlayer() {
	return *tracePlayer_;
}

}  // namespace raco::application