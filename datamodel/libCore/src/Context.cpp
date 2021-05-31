/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"

#include "core/EditorObject.h"
#include "core/ExtrefOperations.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Link.h"
#include "core/PrefabOperations.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/CoreFormatter.h"
#include "log_system/log.h"
#include "serialization/Serialization.h"
#include "serialization/SerializationFunctions.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"

#include <core/PathManager.h>
#include <spdlog/fmt/fmt.h>

namespace raco::core {

BaseContext::BaseContext(Project* project, EngineInterface* engineInterface, UserObjectFactoryInterface* objectFactory, DataChangeRecorder* changeRecorder, Errors* errors)
	: project_(project), engineInterface_(engineInterface), objectFactory_(objectFactory), errors_{errors}, uiChanges_(changeRecorder) {
	changeMultiplexer_.addRecorder(uiChanges_);
	changeMultiplexer_.addRecorder(&modelChanges_);
}

Project* BaseContext::project() {
	return project_;
}

ExternalProjectsStoreInterface* BaseContext::externalProjectsStore() {
	return externalProjectsStore_;
}

void BaseContext::setExternalProjectsStore(ExternalProjectsStoreInterface* store) {
	externalProjectsStore_ = store;
}

MeshCache* BaseContext::meshCache() {
	return meshCache_;
}

void BaseContext::setMeshCache(MeshCache* cache) {
	meshCache_ = cache;
}

FileChangeMonitor* BaseContext::fileChangeMonitor() {
	return fileChangeMonitor_;
}

void BaseContext::setFileChangeMonitor(FileChangeMonitor* monitor) {
	fileChangeMonitor_ = monitor;
}

EngineInterface& BaseContext::engineInterface() {
	return *engineInterface_;
}

MultiplexedDataChangeRecorder& BaseContext::changeMultiplexer() {
	return changeMultiplexer_;
}

DataChangeRecorder& BaseContext::modelChanges() {
	return modelChanges_;
}

DataChangeRecorder& BaseContext::uiChanges() {
	return *uiChanges_;
}

UserObjectFactoryInterface* BaseContext::objectFactory() {
	return objectFactory_;
}

Errors& BaseContext::errors() {
	return *errors_;
}

void BaseContext::callReferencedObjectChangedHandlers(SEditorObject const& changedObject) {
	ValueHandle changedObjHandle(changedObject);
	for (auto weakObject : changedObject->referencesToThis_) {
		auto object = weakObject.lock();
		if (object) {
			object->onAfterReferencedObjectChanged(*this, changedObjHandle);
		}
	}
}

void BaseContext::performExternalFileReload(const std::vector<SEditorObject>& objects) {
	for (const auto& object : objects) {
		object->onAfterContextActivated(*this);
		callReferencedObjectChangedHandlers(object);
	}
}

template <typename T>
void BaseContext::setT(ValueHandle const& handle, T const& value) {
	ValueBase* v = handle.valueRef();
	v->set(value);

	handle.object_->onAfterValueChanged(*this, handle);

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);
}

template <>
void BaseContext::setT<SEditorObject>(ValueHandle const& handle, SEditorObject const& value) {
	ValueBase* v = handle.valueRef();

	auto oldValue = v->asRef();
	if (oldValue) {
		oldValue->onBeforeRemoveReferenceToThis(handle);
	}

	*v = value;

	auto newValue = v->asRef();
	if (newValue) {
		newValue->onAfterAddReferenceToThis(handle);
	}

	handle.object_->onAfterValueChanged(*this, handle);

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);
}

void BaseContext::set(ValueHandle const& handle, bool const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, int const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, double const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, std::string const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, std::vector<std::string> const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, SEditorObject const& value) {
	setT(handle, value);
}

ValueBase* BaseContext::addProperty(const ValueHandle& handle, std::string name, std::unique_ptr<ValueBase>&& newProperty) {
	Table& table{handle.valueRef()->asTable()};

	ValueBase* newValue = table.addProperty(name, std::move(newProperty));

	// Cache/Restore links starting or ending on parent properties:
	// The structure on one side of the link has changed and links need to be revalidated.
	for (auto link : Queries::getLinksConnectedToPropertyParents(*project_, handle, true)) {
		updateLinkValidity(link);
	}

	// Cache/Restore links starting or ending on the property or its child properties
	for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, handle.get(name), true, true)) {
		updateLinkValidity(link);
	}

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);

	return newValue;
}

void BaseContext::removeProperty(const ValueHandle& handle, size_t index) {
	Table& table{handle.valueRef()->asTable()};

	{
		auto propHandle = handle[index];

		// Cache links starting or ending on the property or its child properties
		for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, propHandle, true, true)) {
			if (*link->isValid_) {
				link->isValid_ = false;
				changeMultiplexer_.recordChangeValidityOfLink(link->descriptor());
			}
		}

		if (propHandle.type() == PrimitiveType::Ref) {
			auto refValue = propHandle.valueRef()->asRef();
			if (refValue) {
				refValue->onBeforeRemoveReferenceToThis(propHandle);
			}
		} else if (propHandle.hasSubstructure()) {
			for (auto prop : ValueTreeIteratorAdaptor(propHandle)) {
				if (prop.type() == PrimitiveType::Ref) {
					auto refValue = prop.valueRef()->asRef();
					if (refValue) {
						refValue->onBeforeRemoveReferenceToThis(prop);
					}
				}
			}
		}

		table.removeProperty(index);
	}

	// Cache/Restore links starting or ending on parent properties:
	// The structure on one side of the link has changed and links need to be revalidated.
	for (auto link : Queries::getLinksConnectedToPropertyParents(*project_, handle, true)) {
		updateLinkValidity(link);
	}

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);
}

void BaseContext::removeProperty(const ValueHandle& handle, const std::string& name) {
	Table& table{handle.valueRef()->asTable()};
	removeProperty(handle, table.index(name));
}

void BaseContext::removeAllProperties(const ValueHandle& handle) {
	while (handle.size() > 0) {
		removeProperty(handle, 0);
	}
}

namespace {
std::vector<SEditorObject> collectObjectsForCopyOrCutOperations(const std::vector<SEditorObject>& objects, bool deep) {
	std::set<SEditorObject> toCheck{objects.begin(), objects.end()};
	std::vector<SEditorObject> objectsNeededForCopy{objects.begin(), objects.end()};
	while (toCheck.size() > 0) {
		auto current{toCheck.extract(toCheck.begin()).value()};
		for (const auto& ref : Queries::findAllReferences(current)) {
			if (ref && (Queries::isChildHandle(ref) || deep) && ref.asRef() && std::find(objectsNeededForCopy.begin(), objectsNeededForCopy.end(), ref.asRef()) == objectsNeededForCopy.end()) {
				objectsNeededForCopy.push_back(ref.asRef());
				toCheck.insert(ref.asRef());
			}
		}
	}
	return objectsNeededForCopy;
}

std::vector<SLink> collectLinksForCopyOrCutOperation(const Project& project, const std::vector<SEditorObject>& objects) {
	std::vector<SLink> links{};
	for (const auto& obj : objects) {
		auto objTerminals{Queries::getLinksConnectedToObject(project, obj, false, true)};
		links.insert(links.end(), objTerminals.begin(), objTerminals.end());
	}
	return links;
}

}  // namespace

std::string BaseContext::copyObjects(const std::vector<SEditorObject>& objects, bool deepCopy) {
	auto allObjects{collectObjectsForCopyOrCutOperations(objects, deepCopy)};
	return raco::serialization::serialize(allObjects, collectLinksForCopyOrCutOperation(*project_, allObjects), project_->currentFolder(), project_->currentFileName(), project_->projectID(), project_->projectName(), project_->externalProjectsMap()).c_str();
}

std::string BaseContext::cutObjects(const std::vector<SEditorObject>& objects, bool deepCut) {
	auto allObjects{collectObjectsForCopyOrCutOperations(objects, deepCut)};
	auto allLinks{collectLinksForCopyOrCutOperation(*project_, allObjects)};
	std::string serialization{raco::serialization::serialize(allObjects, allLinks, project_->currentFolder(), project_->currentFileName(), project_->projectID(), project_->projectName(), project_->externalProjectsMap()).c_str()};
	deleteObjects(allObjects);
	return serialization;
}

void BaseContext::rerootRelativePaths(const SEditorObject& editorObject, const std::string& originFolder) {
	for (auto property : core::ValueTreeIteratorAdaptor(core::ValueHandle{editorObject})) {
		if (!property.rootObject()->query<core::ExternalReferenceAnnotation>()) {
			if (property.query<data_storage::URIAnnotation>()) {
				auto uriPath = property.asString();
				if (!originFolder.empty() && !uriPath.empty() && std::filesystem::path{uriPath}.is_relative()) {
					if (PathManager::pathsShareSameRoot(originFolder, this->project()->currentPath())) {
						property.valueRef()->set(PathManager::rerootRelativePath(uriPath, originFolder, this->project()->currentFolder()));
					} else {
						property.valueRef()->set(PathManager::constructAbsolutePath(originFolder, uriPath));
					}
				}
			}
		}
	}
}


bool BaseContext::extrefPasteDiscardObject(SEditorObject editorObject, raco::serialization::ObjectsDeserialization& deserialization) {
	// filter objects:
	// - keep only top-level prefabs and resources,
	if (!(editorObject->getTypeDescription().isResource || editorObject->as<user_types::Prefab>())) {
		return true;
	}

	auto localObj = Queries::findById(*project_, editorObject->objectID());
	if (!localObj) {
		return false;
	}

	auto localAnno = localObj->query<ExternalReferenceAnnotation>();
	if (!localAnno) {
		throw ExtrefError("Cant' paste existing local object as external reference.");
	}

	// We know we want to discard the object at this point.
	// But first we need to check that the existing local and the pasted object originate from the same project.

	std::string originProjectID;
	std::string originProjectPath;
	if (auto anno = editorObject->query<ExternalReferenceAnnotation>()) {
		originProjectID = *anno->projectID_;
		auto it = deserialization.externalProjectsMap.find(originProjectID);
		if (it != deserialization.externalProjectsMap.end()) {
			originProjectPath = PathManager::constructAbsolutePath(deserialization.originFolder, it->second.path);
		} else {
			throw ExtrefError("Paste: can't resolve external project path");
		}
	} else {
		originProjectID = deserialization.originProjectID;
		originProjectPath = deserialization.originPath();
	}

	if (originProjectID != *localAnno->projectID_) {
		throw ExtrefError("Attempting to paste duplicate object from different project.");
	}

	if (originProjectPath != project_->lookupExternalProjectPath(*localAnno->projectID_)) {
		throw ExtrefError("Attempting to paste from duplicate external project with different file path.");
	}

	return true;
}

void BaseContext::adjustExtrefAnnotationsForPaste(std::vector<SEditorObject>& newObjects, raco::serialization::ObjectsDeserialization& deserialization, bool pasteAsExtref) {
	for (auto& editorObject : newObjects) {
		if (pasteAsExtref) {
			if (!editorObject->query<ExternalReferenceAnnotation>()) {
				editorObject->addAnnotation(std::make_shared<ExternalReferenceAnnotation>(deserialization.originProjectID));
				project_->addExternalProjectMapping(deserialization.originProjectID, deserialization.originPath(), deserialization.originProjectName);
			} else {
				auto anno = editorObject->query<ExternalReferenceAnnotation>();
				std::string extProjID = *anno->projectID_;

				auto it = deserialization.externalProjectsMap.find(extProjID);
				if (it != deserialization.externalProjectsMap.end()) {
					project_->addExternalProjectMapping(extProjID, PathManager::constructAbsolutePath(deserialization.originFolder, it->second.path), it->second.name);
				} else {
					throw ExtrefError("Paste: can't resolve external project path");
				}
			}
		} else {
			if (auto anno = editorObject->query<ExternalReferenceAnnotation>()) {
				editorObject->removeAnnotation(anno);
			}
		}
	}
}

void BaseContext::restoreReferences(const Project& project, std::vector<SEditorObject>& newObjects, raco::serialization::ObjectsDeserialization& deserialization) {
	std::map<std::string, SEditorObject> oldIdToEditorObject{};
	for (auto& object : newObjects) {
		auto editorObject{std::dynamic_pointer_cast<core::EditorObject>(object)};
		oldIdToEditorObject[editorObject->objectID()] = editorObject;
	}

	// Point the references to the appropriate objects (either newly copied reference remapping or to already existing references within the project)
	for (const auto& ref : deserialization.references) {
		auto it{oldIdToEditorObject.find(ref.second)};
		if (it != oldIdToEditorObject.end()) {
			*ref.first = it->second;
		} else {
			*ref.first = Queries::findById(project, ref.second);
		}
	}
}

std::vector<SEditorObject> BaseContext::pasteObjects(const std::string& seralizedObjects, const SEditorObject& target, bool pasteAsExtref) {
	auto deserialization{raco::serialization::deserializeObjects(seralizedObjects, 
		raco::user_types::UserObjectFactoryInterface::deserializationFactory(objectFactory_))};

	if (deserialization.objects.size() == 0) {
		return {};
	}

	if (pasteAsExtref && project_->projectID() == deserialization.originProjectID) {
		throw ExtrefError("Paste: external reference project loop detected (based on project ID).");
	}

	// When pasting extref objects:
	// - keep existing ExternalReferenceAnnotation
	// - add to external project mapping:
	//   this needs the external project map from the origin project of the paste.
	// - don't perform scenegraph move
	// - don't make object name unique

	std::vector<SEditorObject> newObjects{};
	std::set<SEditorObject> discardedObjects{};

	// Filter out objects that need to be discarded in paste
	for (auto& object : deserialization.objects) {
		auto editorObject{std::dynamic_pointer_cast<core::EditorObject>(object)};

		if (pasteAsExtref && extrefPasteDiscardObject(editorObject, deserialization)) {
			discardedObjects.insert(editorObject);
		} else {
			newObjects.emplace_back(editorObject);
		}
	}

	BaseContext::restoreReferences(*project_, newObjects, deserialization);

	try {
		adjustExtrefAnnotationsForPaste(newObjects, deserialization, pasteAsExtref);
	} catch (const ExtrefError& e) {
		// Cleanup external projects that have been added by addExternalProjectMapping above but that are not used since we abort here.
		project_->gcExternalProjectMapping();
		throw e;
	}

	// Generate new ids, reroot relative paths and call appropriate context functions for object creation
	for (auto& editorObject : newObjects) {
		// change object id and reroot uris of non-extref objects
		if (!editorObject->query<ExternalReferenceAnnotation>()) {
			auto newId{EditorObject::normalizedObjectID(std::string())};
			editorObject->setObjectID(newId);
			rerootRelativePaths(editorObject, deserialization.originFolder);
		}

		project_->addInstance(editorObject);
		changeMultiplexer_.recordCreateObject(editorObject);
	}

	for (auto& i : deserialization.links) {
		auto link{std::dynamic_pointer_cast<raco::core::Link>(i)};
		// Drop links if the start/end object doesn't exist, it violates prefab constraints or creates a loop.
		// Keep links if the property doesn't exist or the types don't match: these links are only (temporarily) invalid.
		if (*link->startObject_ && *link->endObject_ &&
			discardedObjects.find(*link->startObject_) == discardedObjects.end() &&
			discardedObjects.find(*link->endObject_) == discardedObjects.end() &&
			Queries::linkWouldBeAllowed(*project_, link->startProp(), link->endProp())) {
			project_->addLink(link);
			changeMultiplexer_.recordAddLink(link->descriptor());
		} else {
			LOG_INFO(log_system::CONTEXT, "Discard invalid link {}", link);
		}
	}

	for (auto& instance : newObjects) {
		instance->onAfterDeserialization();
	}

	performExternalFileReload(newObjects);

	// collect all top level objects (e.g. everything which doesn't have a parent)
	std::vector<SEditorObject> topLevelObjects{};
	std::copy_if(newObjects.begin(), newObjects.end(), std::back_inserter(topLevelObjects), [](const SEditorObject& object) {
		return object->getParent() == nullptr;
	});

	if (!pasteAsExtref) {
		// move all top level objects onto the target if it is allowed.
		for (const SEditorObject& obj : topLevelObjects) {
			if (!obj->query<ExternalReferenceAnnotation>() &&
				Queries::canMoveScenegraphChild(*project_, obj, target)) {
				moveScenegraphChild(obj, target);
			}
		}

		std::vector<SEditorObject> rootNodes;
		std::copy_if(project_->instances().begin(), project_->instances().end(), std::back_inserter(rootNodes), [](const SEditorObject& obj) { return obj->getParent() == nullptr; });

		for (const auto& obj : newObjects) {
			if (!obj->query<ExternalReferenceAnnotation>()) {
				auto parent = obj->getParent();
				auto baseName = obj->objectName();

				const std::string uniqueName = parent ? project_->findAvailableUniqueName(parent->begin(), parent->end(), obj, obj->objectName()) : project_->findAvailableUniqueName(rootNodes.begin(), rootNodes.end(), obj, obj->objectName());

				obj->setObjectName(uniqueName);
			}
		}
	}

	if (pasteAsExtref) {
		changeMultiplexer_.recordExternalProjectMapChanged();

		std::vector<std::string> stack;
		stack.emplace_back(project_->currentPath());
		ExtrefOperations::updateExternalObjects(*this, project_, *externalProjectsStore_, stack);
	}

	return topLevelObjects;
}

ValueTreeIterator BaseContext::erase(const ValueTreeIterator& it) {
	removeProperty(it->parent(), it->indices_.back());
	return ValueTreeIterator::normalized(it);
}

void BaseContext::updateLinkValidity(SLink link) {
	if (!link->isValid() && Queries::linkWouldBeValid(*project(), link->startProp(), link->endProp())) {
		link->isValid_ = true;
		changeMultiplexer_.recordChangeValidityOfLink(link->descriptor());
	} else if (link->isValid() && !Queries::linkWouldBeValid(*project(), link->startProp(), link->endProp())) {
		link->isValid_ = false;
		changeMultiplexer_.recordChangeValidityOfLink(link->descriptor());
	}
}

SEditorObject BaseContext::createObject(std::string type, std::string name, std::string id) {
	SEditorObject object = objectFactory_->createObject(type, name, id);

	project_->addInstance(object);

	// We need on after create handler to get initial errors to work
	object->onAfterContextActivated(*this);

	changeMultiplexer_.recordCreateObject(object);

	return object;
}

std::set<SEditorObject> allChildren(std::vector<SEditorObject> const& objects) {
	std::set<SEditorObject> children;
	for (auto obj : objects) {
		std::copy(TreeIteratorAdaptor(obj).begin(), TreeIteratorAdaptor(obj).end(), std::inserter(children, children.end()));
	}
	return children;
}

void BaseContext::removeReferencesTo(std::set<SEditorObject> const& objects) {
	std::set<SEditorObject> srcObjects;
	for (auto obj : objects) {
		for (auto srcWeakObj : obj->referencesToThis_) {
			if (auto srcObj = srcWeakObj.lock()) {
				srcObjects.insert(srcObj);
			}
		}
	}
	for (auto instance : srcObjects) {
		if (objects.find(instance) == objects.end()) {
			auto adaptor = ValueTreeIteratorAdaptor(ValueHandle(instance));
			auto it = adaptor.begin();
			while (it != adaptor.end()) {
				bool step = true;
				if (it->type() == PrimitiveType::Ref) {
					auto refValue = it->asTypedRef<EditorObject>();
					if (refValue && (objects.find(refValue) != objects.end())) {
						auto parent = it->parent();
						if (parent && parent.depth() > 0 && parent.query<ArraySemanticAnnotation>()) {
							it = erase(it);
							step = false;
						} else {
							set(*it, SEditorObject());
						}
					}
				}
				if (step) {
					++it;
				}
			}
		}
	}
}

bool BaseContext::deleteWithVolatileSideEffects(Project* project, const std::set<SEditorObject>& objects, Errors& errors, bool gcExternalProjectMap) {
	// Pretend to remove references from the deleted into the non-deleted objects:
	// only call the onBeforeRemoveReferenceToThis handler; needed for removing backpointers
	std::vector<ValueHandle> outgoingRefs;
	outgoingRefs = Queries::findAllReferencesFrom(objects);

	for (auto value : outgoingRefs) {
		auto oldValue = value.valueRef()->asRef();
		if (oldValue) {
			oldValue->onBeforeRemoveReferenceToThis(value);
		}
	}

	// Call onBeforeDeleteObject handlers to allow objects to deregister file watcher handlers
	for (auto obj : objects) {
		obj->onBeforeDeleteObject(errors);
	}

	// Remove objects from project instance pool
	return project->removeInstances(objects, gcExternalProjectMap);
}

size_t BaseContext::deleteObjects(std::vector<SEditorObject> const& objects, bool gcExternalProjectMap) {
	auto toRemove = allChildren(objects);

	// Remove links starting or ending on any of the deleted objects
	for (auto obj : toRemove) {
		for (auto link : Queries::getLinksConnectedToObject(*project_, obj, true, true)) {
			removeLink(link->endProp());
		}
	}

	// Remove references from project objects to removed objects
	removeReferencesTo(toRemove);

	if (deleteWithVolatileSideEffects(project_, toRemove, *errors_, gcExternalProjectMap)) {
		changeMultiplexer_.recordExternalProjectMapChanged();
	}

	// Record change
	for (auto obj : toRemove) {
		changeMultiplexer_.recordDeleteObject(obj);
	}

	return toRemove.size();
}

void BaseContext::moveScenegraphChild(SEditorObject const& object, SEditorObject const& newParent, int insertBeforeIndex) {
	if (!object) {
		return;
	}

	auto oldParent = object->parent_.lock();
	int oldChildIndex = -1;
	if (oldParent) {
		oldChildIndex = oldParent->findChildIndex(object.get());
	}

	// Moving the object to before itself or to before its successor is a NOP:
	if (oldParent == newParent &&
		(oldChildIndex == insertBeforeIndex || oldChildIndex + 1 == insertBeforeIndex)) {
		return;
	}

	if (oldParent) {
		// Special case: move inside the same object:
		// if moving towards the end we need to adjust the insertion index since removing the
		// object in its current position will shift the insertion index by one.
		if (oldParent == newParent && insertBeforeIndex > oldChildIndex) {
			--insertBeforeIndex;
		}

		ValueHandle oldParentChildren{oldParent, {"children"}};
		removeProperty(oldParentChildren, oldChildIndex);
	}

	if (newParent) {
		ValueBase* newChildEntry = newParent->children_->addProperty(PrimitiveType::Ref, insertBeforeIndex);
		*newChildEntry = object;

		int newChildIndex = newParent->findChildIndex(object.get());

		ValueHandle newParentChildren{newParent, {"children"}};
		object->onAfterAddReferenceToThis(newParentChildren[newChildIndex]);

		callReferencedObjectChangedHandlers(object);

		changeMultiplexer_.recordValueChanged(newParentChildren);
	}

	// Remove links attached to the moved object subtree that are not allowed with the new parent by the prefab-related constraints.
	std::vector<SLink> linksToRemove;
	for (auto child : TreeIteratorAdaptor(object)) {
		for (auto link : Queries::getLinksConnectedToObject(*project_, child, true, true)) {
			if (!Queries::linkSatisfiesPrefabConstraints(link->startProp(), link->endProp())) {
				removeLink(link->endProp());
				LOG_WARNING(log_system::CONTEXT, "Removed link violating prefab constraints: {}", link);
			}
		}
	}
}

bool BaseContext::importAssetScenegraph(const std::string& absPath, SEditorObject const& parent) {
	MeshDescriptor descriptor{absPath, 0, false};
	if (auto meshData = meshCache()->loadMesh(descriptor)) {
		auto relativeFilePath = PathManager::constructRelativePath(descriptor.absPath, project()->currentFolder());
		std::vector<SEditorObject> meshScenegraphMeshes;
		std::vector<SEditorObject> meshScenegraphNodes;

		auto findOrCreateObject = [this](auto& type, auto& name, auto& objectPoolToSearch) {
			auto objectInPool = Queries::findByName(objectPoolToSearch, name);

			if (objectInPool) {
				LOG_DEBUG(log_system::CONTEXT, "Found existing {} with name {}", type, name);
				return objectInPool;
			}
			LOG_DEBUG(log_system::CONTEXT, "Did not find existing {} with name {}, creating one instead...", type, name);
			return createObject(type, name);
		};

		LOG_INFO(log_system::CONTEXT, "Importing scenegraph...");
		auto meshScenegraph = meshCache()->getMeshScenegraph(descriptor.absPath, descriptor.bakeAllSubmeshes);

		LOG_INFO(log_system::CONTEXT, "Importing all meshes...");
		auto projectMeshes = Queries::filterByTypeName(project()->instances(), {raco::user_types::Mesh::typeDescription.typeName});
		for (size_t i{0}; i < meshScenegraph.meshes.size(); ++i) {
			auto currentSubmesh = meshScenegraphMeshes.emplace_back(findOrCreateObject(raco::user_types::Mesh::typeDescription.typeName, meshScenegraph.meshes[i], projectMeshes));
			auto currentSubmeshHandle = ValueHandle{currentSubmesh};

			set(currentSubmeshHandle.get("bakeMeshes"), false);
			set(currentSubmeshHandle.get("meshIndex"), static_cast<int>(i));
			set(currentSubmeshHandle.get("uri"), relativeFilePath);
		}
		LOG_INFO(log_system::CONTEXT, "All meshes imported.");

		LOG_INFO(log_system::CONTEXT, "Importing scenegraph nodes...");
		std::vector<SEditorObject> topLevelObjects{};
		std::copy_if(project_->instances().begin(), project_->instances().end(), std::back_inserter(topLevelObjects), [](const SEditorObject& object) {
			return object->getParent() == nullptr;
		});

		auto meshPath = std::filesystem::path(relativeFilePath).filename().string();
		meshPath = project_->findAvailableUniqueName(topLevelObjects.begin(), topLevelObjects.end(), nullptr, meshPath);
		auto sceneRootNode = createObject(raco::user_types::Node::typeDescription.typeName, meshPath);
		if (parent && core::Queries::canMoveScenegraphChild(*project(), sceneRootNode, parent)) {
			moveScenegraphChild(sceneRootNode, parent);
		}

		LOG_DEBUG(log_system::CONTEXT, "Traversing through scenegraph nodes...");
		auto projectMaterials = Queries::filterByTypeName(project()->instances(), {user_types::Material::typeDescription.typeName});
		for (size_t i{0}; i < meshScenegraph.nodes.size(); ++i) {
			auto meshScenegraphNode = meshScenegraph.nodes[i];
			SEditorObject newNode;
			if (meshScenegraphNode.subMeshIndeces.empty()) {
				LOG_DEBUG(log_system::CONTEXT, "Found node {} with no submeshes -> creating Node...", meshScenegraphNode.name);
				newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name));
			} else {
				SEditorObject submeshRootNode;
				if (meshScenegraphNode.subMeshIndeces.size() == 1) {
					LOG_DEBUG(log_system::CONTEXT, "Found node {} with singular submesh -> creating MeshNode...", meshScenegraphNode.name);
					newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::MeshNode::typeDescription.typeName, meshScenegraphNode.name));
					submeshRootNode = newNode;
				} else {
					LOG_DEBUG(log_system::CONTEXT, "Found node {} with multiple submeshes -> creating MeshNode for each submesh...", meshScenegraphNode.name);
					newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name));
					submeshRootNode = createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name + "_meshnodes");
					moveScenegraphChild(submeshRootNode, newNode);
				}

				for (size_t submeshIndex{0}; submeshIndex < meshScenegraphNode.subMeshIndeces.size(); ++submeshIndex) {
					auto assignedSubmeshIndex = meshScenegraphNode.subMeshIndeces[submeshIndex];
					SEditorObject submeshNode;
					if (meshScenegraphNode.subMeshIndeces.size() == 1) {
						submeshNode = newNode;
					} else {
						submeshNode = createObject(raco::user_types::MeshNode::typeDescription.typeName, meshScenegraphNode.name + "_meshnode_" + std::to_string(submeshIndex));
						moveScenegraphChild(submeshNode, submeshRootNode);
					}

					set(ValueHandle{submeshNode}.get("mesh"), meshScenegraphMeshes[assignedSubmeshIndex]);

					auto glTFMaterialName = meshScenegraph.materials[assignedSubmeshIndex];
					if (!glTFMaterialName.empty()) {
						LOG_DEBUG(log_system::CONTEXT, "Searching for material {} which belongs to MeshNode {}", glTFMaterialName, meshScenegraphNode.name);
						auto foundMaterial = Queries::findByName(projectMaterials, glTFMaterialName);

						if (foundMaterial && foundMaterial->getTypeDescription().typeName == user_types::Material::typeDescription.typeName) {
							LOG_DEBUG(log_system::CONTEXT, "Found matching material {} in project resources, will reassign current MeshNode material to it", glTFMaterialName);
							set(ValueHandle{submeshNode}.get("materials")[0].get("material"), foundMaterial);
						}
					}
				}
			}

			if (!meshScenegraphNode.hasParent() && core::Queries::canMoveScenegraphChild(*project(), newNode, sceneRootNode)) {
				moveScenegraphChild(newNode, sceneRootNode);
			}
			LOG_DEBUG(log_system::CONTEXT, "All nodes traversed.");

			LOG_DEBUG(log_system::CONTEXT, "Applying scenegraph node transformations...");
			ValueHandle newNodeHandle{newNode};

			auto transformNode = [this](auto& valueHandle, auto& propertyName, auto& vec3f) {
				set(valueHandle.get(propertyName).get("x"), vec3f[0]);
				set(valueHandle.get(propertyName).get("y"), vec3f[1]);
				set(valueHandle.get(propertyName).get("z"), vec3f[2]);
			};

			transformNode(newNodeHandle, "scale", meshScenegraphNode.transformations.scale);
			transformNode(newNodeHandle, "rotation", meshScenegraphNode.transformations.rotation);
			transformNode(newNodeHandle, "translation", meshScenegraphNode.transformations.translation);
			LOG_DEBUG(log_system::CONTEXT, "All scenegraph node transformations applied.");
		}
		LOG_INFO(log_system::CONTEXT, "All scenegraph nodes imported.");

		LOG_INFO(log_system::CONTEXT, "Restoring scenegraph structure...");
		for (size_t i{0}; i < meshScenegraph.nodes.size(); ++i) {
			auto meshScenegraphNode = meshScenegraph.nodes[i];

			if (meshScenegraphNode.hasParent() && core::Queries::canMoveScenegraphChild(*project(), meshScenegraphNodes[i], meshScenegraphNodes[meshScenegraphNode.parentIndex])) {
				moveScenegraphChild(meshScenegraphNodes[i], meshScenegraphNodes[meshScenegraphNode.parentIndex]);
			}
		}
		LOG_INFO(log_system::CONTEXT, "Scenegraph structure restored.");

		return true;
	}

	return false;
}

SLink BaseContext::addLink(const ValueHandle& start, const ValueHandle& end) {
	// Remove existing links ending on the property or any nested child properties
	for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, end, false, true)) {
		removeLink(link->endProp());
	}

	auto link = std::make_shared<Link>(start.getDescriptor(), end.getDescriptor());

	project_->addLink(link);
	changeMultiplexer_.recordAddLink(link->descriptor());
	return link;
}

void BaseContext::removeLink(const PropertyDescriptor& end) {
	if (auto link = Queries::getLink(*project_, end)) {
		project_->removeLink(link);
		changeMultiplexer_.recordRemoveLink(link->descriptor());
	}
}

void BaseContext::updateExternalReferences(std::vector<std::string>& pathStack) {
	ExtrefOperations::updateExternalObjects(*this, project(), *externalProjectsStore(), pathStack);
	PrefabOperations::globalPrefabUpdate(*this, modelChanges());
}

}  // namespace raco::core
