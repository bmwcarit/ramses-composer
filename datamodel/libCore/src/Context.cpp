/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"

#include "core/CodeControlledPropertyModifier.h"
#include "core/CoreFormatter.h"
#include "core/EditorObject.h"
#include "core/ErrorItem.h"
#include "core/Errors.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/ExtrefOperations.h"
#include "core/Iterators.h"
#include "core/Link.h"
#include "core/MeshCacheInterface.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/Queries.h"
#include "core/Serialization.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"
#include "log_system/log.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Skin.h"

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
	// Note: object->onAfterReferencedObjectChanged inside the loop may remove obects from changedObject->referencesToThis_
	// leading to iterator invalidation if we naively iterate over changedObject->referencesToThis_
	// solution: iterate over copy and check that each object is still in changedObject->referencesToThis_
	// before calling object->onAfterReferencedObjectChanged.
	// Avoids iterator invalidation from removing objects from referencesToThis_.
	auto refCopy = changedObject->referencesToThis_;
	for (auto weakObject : refCopy) {
		if (changedObject->referencesToThis_.find(weakObject) != changedObject->referencesToThis_.end()) {
			auto object = weakObject.lock();
			if (object) {
				object->onAfterReferencedObjectChanged(*this, changedObjHandle);
			}
		}
	}
}

void BaseContext::performExternalFileReload(const std::vector<SEditorObject>& objects) {
	// TODO: the implementation below is correct but sometimes leads to duplicate work:
	// Objects implementing both onAfterReferencedObjectChanged and onAfterContextActivated handlers
	// (currently MeshNodes and Animations) may perform snyc operations twice.
	// This is not straightforward to fix since there are also situation where only one of the two handlers 
	// will be called so we can't just remove one of the calls.
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

template<void (EditorObject::*Handler)(ValueHandle const&) const>
void BaseContext::callReferenceToThisHandler(ValueHandle const& vh) {
	if (vh.type() == PrimitiveType::Ref) {
		auto vref = vh.asRef();
		if (vref) {
			(vref.get()->*Handler)(vh);
		}
	} else if (vh.hasSubstructure()) {
		for (int i = 0; i < vh.size(); ++i) {
			callReferenceToThisHandler<Handler>(vh[i]);
		}
	}
}

template <>
void BaseContext::setT<Table>(ValueHandle const& handle, Table const& value) {
	ValueBase* v = handle.valueRef();

	callReferenceToThisHandler<&EditorObject::onBeforeRemoveReferenceToThis>(handle);

	v->set(value);

	// Cache/Restore links starting or ending on parent properties:
	// The structure on one side of the link has changed and links need to be revalidated.
	for (auto link : Queries::getLinksConnectedToPropertyParents(*project_, handle, true)) {
		updateLinkValidity(link);
	}

	// Cache/Restore links starting or ending on the property or its child properties
	for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, handle, true, true)) {
		updateLinkValidity(link);
	}

	callReferenceToThisHandler<&EditorObject::onAfterAddReferenceToThis>(handle);

	handle.object_->onAfterValueChanged(*this, handle);

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);
}

template <>
void BaseContext::setT<StructBase>(ValueHandle const& handle, StructBase const& value) {
	ValueBase* v = handle.valueRef();

	callReferenceToThisHandler<&EditorObject::onBeforeRemoveReferenceToThis>(handle);

	v->setStruct(value);

	callReferenceToThisHandler<&EditorObject::onAfterAddReferenceToThis>(handle);

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

void BaseContext::set(ValueHandle const& handle, int64_t const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, double const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, std::string const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, char const* value) {
	setT(handle, std::string(value));
}

void BaseContext::set(ValueHandle const& handle, std::vector<std::string> const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, SEditorObject const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, Table const& value) {
	setT(handle, value);
}

void BaseContext::set(ValueHandle const& handle, std::array<double, 2> const& value) {
	Vec2f vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, std::array<double, 3> const& value) {
	Vec3f vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, std::array<double, 4> const& value) {
	Vec4f vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, std::array<int, 2> const& value) {
	Vec2i vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, std::array<int, 3> const& value) {
	Vec3i vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, std::array<int, 4> const& value) {
	Vec4i vecValue;
	vecValue = value;
	setT(handle, static_cast<StructBase const&>(vecValue));
}

void BaseContext::set(ValueHandle const& handle, StructBase const& value) {
	setT(handle, value);
}

ValueBase* BaseContext::addProperty(const ValueHandle& handle, std::string name, std::unique_ptr<ValueBase>&& newProperty, int indexBefore) {
	Table& table{handle.valueRef()->asTable()};

	ValueBase* newValue = table.addProperty(name, std::move(newProperty), indexBefore);

	ValueHandle newHandle = indexBefore == -1 ? handle[handle.size() - 1] : handle[indexBefore];

	// Cache/Restore links starting or ending on parent properties:
	// The structure on one side of the link has changed and links need to be revalidated.
	for (auto link : Queries::getLinksConnectedToPropertyParents(*project_, handle, true)) {
		updateLinkValidity(link);
	}

	// Cache/Restore links starting or ending on the property or its child properties
	for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, newHandle, true, true)) {
		updateLinkValidity(link);
	}

	callReferenceToThisHandler<&EditorObject::onAfterAddReferenceToThis>(newHandle);

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);

	return newValue;
}

void BaseContext::removeProperty(const ValueHandle& handle, size_t index) {
	Table& table{handle.valueRef()->asTable()};

	SEditorObjectSet updateLinkErrors;

	{
		auto propHandle = handle[index];

		// Cache links starting or ending on the property or its child properties
		for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, propHandle, true, true)) {
			if (*link->isValid_) {
				link->isValid_ = false;
				changeMultiplexer_.recordChangeValidityOfLink(link->descriptor());
				// recordValueChanged is needed to force the undo stack to save the current value of the endpoint property.
				changeMultiplexer_.recordValueChanged(ValueHandle(link->endProp()));
			}
			updateLinkErrors.insert(*link->endObject_);
		}

		callReferenceToThisHandler<&EditorObject::onBeforeRemoveReferenceToThis>(propHandle);

		table.removeProperty(index);
	}

	// We need to update the broken link errors _after_ removing the property since links are only considered
	// broken if the endpoint propery exists and removing properties therefore affects the broken status.
	for (auto obj : updateLinkErrors) {
		updateBrokenLinkErrors(obj);
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

void BaseContext::swapProperties(const ValueHandle& handle, size_t index_1, size_t index_2) {
	Table& table{handle.valueRef()->asTable()};
	table.swapProperties(index_1, index_2);

	callReferencedObjectChangedHandlers(handle.object_);

	changeMultiplexer_.recordValueChanged(handle);
}


namespace {
std::vector<SEditorObject> collectObjectsForCopyOrCutOperations(const std::vector<SEditorObject>& objects, bool deep) {
	SEditorObjectSet toCheck{objects.begin(), objects.end()};
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

std::vector<std::string> findRootObjectIDs(const std::vector<SEditorObject>& objects) {
	std::vector<std::string> rootObjectIDs;
	for (auto obj : objects) {
		if (!obj->getParent()) {
			rootObjectIDs.emplace_back(obj->objectID());
		}
	}
	return rootObjectIDs;
}

}  // namespace


std::map<std::string, std::string> findOriginFolders(const Project& project, const std::vector<SEditorObject>& objects) {
	std::map<std::string, std::string> folderMap;
	for (auto object : objects) {
		if (!PathQueries::isPathRelativeToCurrentProject(object)) {
			folderMap[object->objectID()] = PathQueries::baseFolderForRelativePath(project, object);
		}	
	}
	return folderMap;
}

std::string BaseContext::copyObjects(const std::vector<SEditorObject>& objects, bool deepCopy) {
	auto allObjects{collectObjectsForCopyOrCutOperations(objects, deepCopy)};
	auto rootObjectIDs{findRootObjectIDs(allObjects)};
	auto originFolders{findOriginFolders(*project_, allObjects)};
	return raco::serialization::serializeObjects(allObjects, 
		rootObjectIDs, 
		collectLinksForCopyOrCutOperation(*project_, allObjects), 
		project_->currentFolder(), 
		project_->currentFileName(),
		project_->projectID(),
		project_->projectName(),
		project_->externalProjectsMap(),
		originFolders, 
		project_->featureLevel()).c_str();
}

std::string BaseContext::cutObjects(const std::vector<SEditorObject>& objects, bool deepCut) {
	auto allObjects{collectObjectsForCopyOrCutOperations(objects, deepCut)};
	auto allLinks{collectLinksForCopyOrCutOperation(*project_, allObjects)};
	auto rootObjectIDs{findRootObjectIDs(allObjects)};
	auto originFolders{findOriginFolders(*project_, allObjects)};
	std::string serialization{raco::serialization::serializeObjects(allObjects, 
		rootObjectIDs, 
		allLinks, 
		project_->currentFolder(), 
		project_->currentFileName(), 
		project_->projectID(), 
		project_->projectName(), 
		project_->externalProjectsMap(), 
		originFolders,
		project_->featureLevel()).c_str()};
	deleteObjects(Queries::filterForDeleteableObjects(*project_, allObjects));
	return serialization;
}

void BaseContext::rerootRelativePaths(std::vector<SEditorObject>& newObjects, raco::serialization::ObjectsDeserialization& deserialization) {
	for (auto object : newObjects) {
		if (PathQueries::isPathRelativeToCurrentProject(object)) {
			for (auto property : core::ValueTreeIteratorAdaptor(core::ValueHandle{object})) {
				if (property.query<URIAnnotation>()) {
					auto uriPath = property.asString();
					std::string originFolder;
					auto it = deserialization.objectOriginFolders.find(object->objectID());
					if (it != deserialization.objectOriginFolders.end()) {
						originFolder = it->second;
					} else {
						originFolder = deserialization.originFolder;
					}
					if (!originFolder.empty() && !uriPath.empty() && raco::utils::u8path(uriPath).is_relative()) {
						if (raco::utils::u8path::areSharingSameRoot(originFolder, this->project()->currentPath())) {
							property.valueRef()->set(raco::utils::u8path(uriPath).rerootRelativePath(originFolder, this->project()->currentFolder()).string());
						} else {
							property.valueRef()->set(raco::utils::u8path(uriPath).normalizedAbsolutePath(originFolder).string());
						}
					}
				}
			}
		}
	}
}


bool BaseContext::extrefPasteDiscardObject(SEditorObject editorObject, raco::serialization::ObjectsDeserialization& deserialization) {
	// filter objects:
	// - keep only top-level prefabs, top-level lua script and resources
	if (!Queries::canPasteObjectAsExternalReference(editorObject, deserialization.rootObjectIDs.find(editorObject->objectID()) != deserialization.rootObjectIDs.end())) {
		return true;
	}

	auto localObj = Queries::findById(*project_, editorObject->objectID());
	if (!localObj) {
		return false;
	}

	auto localAnno = localObj->query<ExternalReferenceAnnotation>();
	if (!localAnno) {
		throw ExtrefError("Can't paste existing local object as external reference.");
	}

	// We know we want to discard the object at this point.
	// But first we need to check that the existing local and the pasted object originate from the same project.

	std::string originProjectID;
	std::string originProjectPath;
	if (auto anno = editorObject->query<ExternalReferenceAnnotation>()) {
		originProjectID = *anno->projectID_;
		auto it = deserialization.externalProjectsMap.find(originProjectID);
		if (it != deserialization.externalProjectsMap.end()) {
			originProjectPath = raco::utils::u8path(it->second.path).normalizedAbsolutePath(deserialization.originFolder).string();
		} else {
			throw ExtrefError("Paste: can't resolve external project path");
		}
	} else {
		originProjectID = deserialization.originProjectID;
		originProjectPath = deserialization.originPath();
	}

	if (originProjectID != *localAnno->projectID_) {
		throw ExtrefError(fmt::format("Attempting to paste duplicate object from different project.\nAn object with the same ID was already added: {} from {} ({})",
			localObj->objectName(),
			project_->lookupExternalProjectName(*localAnno->projectID_),
			project_->lookupExternalProjectPath(*localAnno->projectID_)));
	}

	if (originProjectPath != project_->lookupExternalProjectPath(*localAnno->projectID_)) {
		throw ExtrefError(fmt::format("Attempting to paste from duplicate external project with different file path.\nAn object with the same ID was already added: {} from {} ({})",
			localObj->objectName(),
			project_->lookupExternalProjectName(*localAnno->projectID_),
			project_->lookupExternalProjectPath(*localAnno->projectID_)));
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
					project_->addExternalProjectMapping(extProjID, raco::utils::u8path(it->second.path).normalizedAbsolutePath(deserialization.originFolder).string(), it->second.name);
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
	for (auto& editorObject : newObjects) {
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

void BaseContext::generateNewObjectIDs(std::vector<raco::core::SEditorObject>& newObjects) {
	// Generate new ids:
	// Pass 1: everything except PrefabInstance children
	std::map<std::string, std::string> prefabInstanceIDMap;
	for (auto& editorObject : newObjects) {
		if (!editorObject->query<ExternalReferenceAnnotation>()) {
			if (!PrefabOperations::findOuterContainingPrefabInstance(editorObject->getParent())) {
				auto newId{EditorObject::normalizedObjectID(std::string())};
				prefabInstanceIDMap[newId] = editorObject->objectID();
				editorObject->setObjectID(newId);
			}
		}
	}
	// Pass 2: PrefabInstance children
	// these are calculated from the containing PrefabInstance object id and need to know the old and new PrefabInstance ID
	for (auto& editorObject : newObjects) {
		if (!editorObject->query<ExternalReferenceAnnotation>()) {
			if (auto inst = PrefabOperations::findOuterContainingPrefabInstance(editorObject->getParent())) {
				auto newInstID = inst->objectID();
				auto oldInstID = prefabInstanceIDMap[newInstID];
				auto newID = EditorObject::XorObjectIDs(EditorObject::XorObjectIDs(editorObject->objectID(), oldInstID), newInstID);
				editorObject->setObjectID(newID);
			}
		}
	}
}

std::vector<SEditorObject> BaseContext::pasteObjects(const std::string& seralizedObjects, const SEditorObject& target, bool pasteAsExtref) {
	auto deserialization_opt{raco::serialization::deserializeObjects(seralizedObjects)};
	if (!deserialization_opt) {
		throw std::runtime_error("Paste Objects: data has invalid format.");
	}
	
	auto& deserialization{*deserialization_opt};

	if (deserialization.objects.size() == 0) {
		return {};
	}

	if (deserialization.featureLevel > project_->featureLevel()) {
		throw std::runtime_error(fmt::format("Pasted data feature level {} bigger than current project feature level {}", deserialization.featureLevel, project_->featureLevel()));
	}

	if (pasteAsExtref && project_->projectID() == deserialization.originProjectID) {
		throw ExtrefError("Paste: external reference project loop detected (based on same project ID).");
	}

	// When pasting extref objects:
	// - keep existing ExternalReferenceAnnotation
	// - add to external project mapping:
	//   this needs the external project map from the origin project of the paste.
	// - don't perform scenegraph move
	// - don't make object name unique

	std::vector<SEditorObject> newObjects{};
	std::set<std::string> discardedObjects{};

	// Filter out objects that need to be discarded in paste
	for (auto& editorObject : deserialization.objects) {
		if (pasteAsExtref && extrefPasteDiscardObject(editorObject, deserialization)) {
			discardedObjects.insert(editorObject->objectID());
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

	for (auto& instance : newObjects) {
		instance->onAfterDeserialization();
	}

	// Ordering constraints
	// - needs parent weak_ptrs: must run after onAfterDeserialization callbacks
	// - needs original object IDs: must run before changing object IDs
	if (!pasteAsExtref) {
		rerootRelativePaths(newObjects, deserialization);
	}
		
	BaseContext::generateNewObjectIDs(newObjects);

	for (auto& editorObject : newObjects) {
		project_->addInstance(editorObject);
		changeMultiplexer_.recordCreateObject(editorObject);
	}

	// collect all top level objects (e.g. everything which doesn't have a parent)
	std::vector<SEditorObject> topLevelObjects{};
	std::copy_if(newObjects.begin(), newObjects.end(), std::back_inserter(topLevelObjects), [](const SEditorObject& object) {
		return object->getParent() == nullptr;
	});

	if (!pasteAsExtref) {
		// move all top level objects onto the target if it is allowed.
		for (const SEditorObject& obj : topLevelObjects) {
			if (!obj->query<ExternalReferenceAnnotation>()) {
				moveScenegraphChildren(Queries::filterForMoveableScenegraphChildren(*project_, {obj}, target), target);
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

	for (auto& link : deserialization.links) {
		// Drop links if the start/end object doesn't exist, it violates prefab constraints or creates a loop.
		// Keep links if the property doesn't exist or the types don't match: these links are only (temporarily) invalid.
		if (*link->startObject_ && *link->endObject_ &&
			discardedObjects.find((*link->endObject_)->objectID()) == discardedObjects.end()) {
			if (Queries::linkWouldBeAllowed(*project_, link->startProp(), link->endProp(), *link->isWeak_)) {
				link->isValid_ = Queries::linkWouldBeValid(*project_, link->startProp(), link->endProp());
				project_->addLink(link);
				changeMultiplexer_.recordAddLink(link->descriptor());
			} else {
				LOG_WARNING(log_system::CONTEXT, "Discard invalid link {}", link);
			}
		}
	}

	performExternalFileReload(newObjects);

	if (pasteAsExtref) {
		changeMultiplexer_.recordExternalProjectMapChanged();

		LoadContext loadContext;
		loadContext.featureLevel = project_->featureLevel();
		loadContext.pathStack.emplace_back(project_->currentPath());
		ExtrefOperations::updateExternalObjects(*this, project_, *externalProjectsStore_, loadContext);
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
		// recordValueChanged is needed to force the undo stack to save the current value of the endpoint property.
		ValueHandle handle(link->endProp());
		if (handle) {
			changeMultiplexer_.recordValueChanged(handle);
		}
	}

	updateBrokenLinkErrors(*link->endObject_);
}

void BaseContext::initLinkValidity() {
	for (const auto& linksEnd : project()->linkEndPoints()) {
		for (const auto& link : linksEnd.second) {
			link->isValid_ = Queries::linkWouldBeValid(*project(), link->startProp(), link->endProp());
		}
	}
}

void BaseContext::initBrokenLinkErrors() {
	SEditorObjectSet brokenLinkEndObjects;
	for (auto link : project_->links()) {
		if (!link->isValid()) {
			brokenLinkEndObjects.insert(*link->endObject_);
		}
	}
	for (auto obj : brokenLinkEndObjects) {
		auto msg = Queries::getBrokenLinksErrorMessage(*project(), obj);
		if (!msg.empty()) {
			errors_->addError(ErrorCategory::GENERAL, ErrorLevel::WARNING, obj, msg);
		}
	}
}

void BaseContext::updateBrokenLinkErrors(SEditorObject endObject) {
	if (errors_->hasError(endObject)) {
		auto error = errors_->getError(endObject);
		if (error.category() == ErrorCategory::PARSING) {
			return;		
		}
		if (error.category() == ErrorCategory::GENERAL) {
			errors_->removeError(endObject);
		}
	}
	
	auto msg = Queries::getBrokenLinksErrorMessage(*project(), endObject);
	if (!msg.empty()) {
		errors_->addError(ErrorCategory::GENERAL, ErrorLevel::WARNING, endObject, msg);
	}
}

void BaseContext::updateBrokenLinkErrorsAttachedTo(SEditorObject object) {
	SEditorObjectSet updateLinkErrors;
	for (auto link : Queries::getLinksConnectedToObject(*project_, object, true, true)) {
		updateLinkErrors.insert(*link->endObject_);
	}
	for (auto obj : updateLinkErrors) {
		updateBrokenLinkErrors(obj);
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

void BaseContext::removeReferencesTo_If(SEditorObjectSet const& objects, std::function<bool(const ValueHandle& handle, SEditorObject object)> pred) {
	SEditorObjectSet srcObjects;
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
					if (refValue && (objects.find(refValue) != objects.end()) && pred(*it, refValue)) {
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

void BaseContext::removeReferencesTo(SEditorObjectSet const& objects) {
	removeReferencesTo_If(objects, [](const ValueHandle& handle, SEditorObject object) {
		return true;
	});
}

void BaseContext::removeReferencesFrom_If(SEditorObjectSet const& objects, std::function<bool(const ValueHandle& handle, SEditorObject object)> pred) {
	for (auto object : objects) {
		auto adaptor = ValueTreeIteratorAdaptor(ValueHandle(object));
		auto it = adaptor.begin();
		while (it != adaptor.end()) {
			bool step = true;
			if (it->type() == PrimitiveType::Ref) {
				auto refValue = it->asTypedRef<EditorObject>();
				if (refValue && (objects.find(refValue) == objects.end()) && pred(*it, refValue)) {
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

bool BaseContext::deleteWithVolatileSideEffects(Project* project, const SEditorObjectSet& objects, Errors& errors, bool gcExternalProjectMap) {
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
		obj->onBeforeDeleteObject(*this);
	}

	// Remove objects from project instance pool
	return project->removeInstances(objects, gcExternalProjectMap);
}

size_t BaseContext::deleteObjects(std::vector<SEditorObject> const& objects, bool gcExternalProjectMap, bool includeChildren) {
	SEditorObjectSet toRemove;
	if (includeChildren) {
		toRemove = Queries::collectAllChildren(objects);
	} else {
		std::copy(objects.begin(), objects.end(), std::inserter(toRemove, toRemove.end()));
	}

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

void BaseContext::moveScenegraphChildren(std::vector<SEditorObject> const& objects, SEditorObject const& newParent, int insertBeforeIndex) {
	if (objects.size() == 0) {
		return;
	}

	for (const auto& object : objects) {
		auto oldParent = object->parent_.lock();

		if (oldParent) {
			int oldChildIndex = oldParent->findChildIndex(object.get());

			if (oldParent == newParent) {
				if (oldChildIndex == insertBeforeIndex || oldChildIndex + 1 == insertBeforeIndex) {
					// Moving the object to before itself or to before its successor is a NOP: 
					++insertBeforeIndex;
					continue;
				} else if (insertBeforeIndex > oldChildIndex) {
					// Special case: move inside the same object:
					// if moving towards the end we need to adjust the insertion index since removing the
					// object in its current position will shift the insertion index by one.
					--insertBeforeIndex;
				}
			}

			removeProperty({oldParent, &EditorObject::children_}, oldChildIndex);
		}

		if (newParent) {
			addProperty({newParent, &EditorObject::children_}, std::string(), std::make_unique<Value<SEditorObject>>(object), insertBeforeIndex);
		}

		if (insertBeforeIndex != -1) {
			++insertBeforeIndex;
		}
	}

	for (const auto& object : objects) {
		// Remove links attached to the moved object subtree that are not allowed with the new parent by the prefab-related constraints.
		std::vector<SLink> linksToRemove;
		for (auto child : TreeIteratorAdaptor(object)) {
			for (auto link : Queries::getLinksConnectedToObject(*project_, child, true, true)) {
				if (!Queries::linkSatisfiesConstraints(link->startProp(), link->endProp())) {
					removeLink(link->endProp());
					LOG_WARNING(log_system::CONTEXT, "Removed link violating prefab constraints: {}", link);
				}
			}
		}
	}

	// Remove incoming invalid references
	SEditorObjectSet movedObjects = Queries::collectAllChildren(objects);
	removeReferencesTo_If(movedObjects, [this](const ValueHandle& handle, SEditorObject object) -> bool {
		return !Queries::isValidReferenceTarget(*project_, handle, object);
	});

	// Remove outgoing invalid references
	removeReferencesFrom_If(movedObjects, [this](const ValueHandle& handle, SEditorObject object) -> bool {
		return !Queries::isValidReferenceTarget(*project_, handle, object);
	});
}

void BaseContext::insertAssetScenegraph(const raco::core::MeshScenegraph& scenegraph, const std::string& absPath, SEditorObject const& parent) {
	auto relativeFilePath = raco::utils::u8path(absPath).normalizedRelativePath(project()->currentFolder());
	std::vector<SEditorObject> meshScenegraphMeshes;
	std::vector<SEditorObject> meshScenegraphNodes;

	LOG_INFO(log_system::CONTEXT, "Importing all meshes...");
	auto projectMeshes = Queries::filterByTypeName(project()->instances(), {raco::user_types::Mesh::typeDescription.typeName});
	std::map<std::tuple<bool, int, std::string>, SEditorObject> propertiesToMeshMap;
	std::map<std::tuple<std::string, int, int>, SEditorObject> propertiesToChannelMap;

	for (const auto& instance : project()->instances()) {
		if (instance->as<raco::user_types::Mesh>() && !instance->query<raco::core::ExternalReferenceAnnotation>()) {
			propertiesToMeshMap[{instance->get("bakeMeshes")->asBool(), instance->get("meshIndex")->asInt(), instance->get("uri")->asString()}] = instance;
		} else if (instance->as<raco::user_types::AnimationChannel>() && !instance->query<raco::core::ExternalReferenceAnnotation>()) {
			auto absPath = core::PathQueries::resolveUriPropertyToAbsolutePath(*project_, ValueHandle(instance, &user_types::AnimationChannel::uri_));
			propertiesToChannelMap[{absPath, instance->get("animationIndex")->asInt(), instance->get("samplerIndex")->asInt()}] = instance;
		}
	}

	for (size_t i{0}; i < scenegraph.meshes.size(); ++i) {
		if (!scenegraph.meshes[i].has_value()) {
			LOG_DEBUG(log_system::CONTEXT, "Found disabled mesh at index {}, ignoring Mesh...", i);
			meshScenegraphMeshes.emplace_back(nullptr);
			continue;
		}

		auto meshWithSameProperties = propertiesToMeshMap.find({false, static_cast<int>(i), relativeFilePath.string()});
		if (meshWithSameProperties == propertiesToMeshMap.end()) {
			LOG_DEBUG(log_system::CONTEXT, "Did not find existing local Mesh with same properties as asset mesh, creating one instead...");
			auto &currentSubmesh = meshScenegraphMeshes.emplace_back(createObject(raco::user_types::Mesh::typeDescription.typeName, *scenegraph.meshes[i]));
			auto currentSubmeshHandle = ValueHandle{currentSubmesh};

			set(currentSubmeshHandle.get("bakeMeshes"), false);
			set(currentSubmeshHandle.get("meshIndex"), static_cast<int>(i));
			set(currentSubmeshHandle.get("uri"), relativeFilePath.string());
		} else {
			LOG_DEBUG(log_system::CONTEXT, "Found existing local Mesh {} with same properties as asset mesh, using this Mesh...", *scenegraph.meshes[i]);
			meshScenegraphMeshes.emplace_back(meshWithSameProperties->second);
		}
	}
	LOG_INFO(log_system::CONTEXT, "All meshes imported.");

	LOG_INFO(log_system::CONTEXT, "Importing scenegraph nodes...");
	std::vector<SEditorObject> topLevelObjects{};
	std::copy_if(project_->instances().begin(), project_->instances().end(), std::back_inserter(topLevelObjects), [](const SEditorObject& object) {
		return object->getParent() == nullptr;
	});

	auto meshPath = relativeFilePath.filename().string();
	meshPath = project_->findAvailableUniqueName(topLevelObjects.begin(), topLevelObjects.end(), nullptr, meshPath);
	auto sceneRootNode = createObject(raco::user_types::Node::typeDescription.typeName, meshPath);
	if (parent) {
		moveScenegraphChildren(core::Queries::filterForMoveableScenegraphChildren(*project(), {sceneRootNode}, parent), parent);
	}

	LOG_DEBUG(log_system::CONTEXT, "Traversing through scenegraph nodes...");
	auto projectMaterials = Queries::filterByTypeName(project()->instances(), {user_types::Material::typeDescription.typeName});
	for (size_t i{0}; i < scenegraph.nodes.size(); ++i) {
		if (!scenegraph.nodes[i].has_value()) {
			LOG_DEBUG(log_system::CONTEXT, "Found disabled node at index {}, ignoring Node...", i);
			meshScenegraphNodes.emplace_back(nullptr);
			continue;
		}
		auto meshScenegraphNode = scenegraph.nodes[i].value();

		SEditorObject newNode;
		if (meshScenegraphNode.subMeshIndices.empty()) {
			LOG_DEBUG(log_system::CONTEXT, "Found node {} with no submeshes -> creating Node...", meshScenegraphNode.name);
			newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name));
		} else {
			SEditorObject submeshRootNode;
			if (meshScenegraphNode.subMeshIndices.size() == 1) {
				LOG_DEBUG(log_system::CONTEXT, "Found node {} with singular submesh -> creating MeshNode...", meshScenegraphNode.name);
				newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::MeshNode::typeDescription.typeName, meshScenegraphNode.name));
				submeshRootNode = newNode;
			} else {
				LOG_DEBUG(log_system::CONTEXT, "Found node {} with multiple submeshes -> creating MeshNode for each submesh...", meshScenegraphNode.name);
				newNode = meshScenegraphNodes.emplace_back(createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name));
				submeshRootNode = createObject(raco::user_types::Node::typeDescription.typeName, meshScenegraphNode.name + "_meshnodes");
				moveScenegraphChildren({submeshRootNode}, newNode);
			}

			for (size_t submeshIndex{0}; submeshIndex < meshScenegraphNode.subMeshIndices.size(); ++submeshIndex) {
				auto submesh = meshScenegraphNode.subMeshIndices[submeshIndex];
				if (!submesh.has_value()) {
					LOG_DEBUG(log_system::CONTEXT, "Found disabled submesh at index {}.{}, ignoring MeshNode...", meshScenegraphNode.name, submeshIndex);
					continue;
				}
				auto assignedSubmeshIndex = *meshScenegraphNode.subMeshIndices[submeshIndex];

				SEditorObject submeshNode;
				if (meshScenegraphNode.subMeshIndices.size() == 1) {
					submeshNode = newNode;
				} else {
					submeshNode = createObject(raco::user_types::MeshNode::typeDescription.typeName, meshScenegraphNode.name + "_meshnode_" + std::to_string(submeshIndex));
					moveScenegraphChildren({submeshNode}, submeshRootNode);
				}

				if (assignedSubmeshIndex < 0) {
					LOG_DEBUG(log_system::CONTEXT, "Found empty Mesh index for submesh {}.{}, created an empty MeshNode...", meshScenegraphNode.name, submeshIndex);
					continue;
				}

				set(ValueHandle{submeshNode}.get("mesh"), meshScenegraphMeshes[assignedSubmeshIndex]);

				const auto& glTFMaterial = scenegraph.materials[assignedSubmeshIndex];
				if (glTFMaterial.has_value()) {
					const auto& glTFMaterialName = *glTFMaterial;
					LOG_DEBUG(log_system::CONTEXT, "Searching for material {} which belongs to MeshNode {}", glTFMaterialName, meshScenegraphNode.name);
					auto foundMaterial = Queries::findByName(projectMaterials, glTFMaterialName);

					if (foundMaterial && foundMaterial->isType<user_types::Material>()) {
						LOG_DEBUG(log_system::CONTEXT, "Found matching material {} in project resources, will reassign current MeshNode material to it", glTFMaterialName);
						set(ValueHandle{submeshNode}.get("materials")[0].get("material"), foundMaterial);
					}
				}
			}
		}

		if (!meshScenegraphNode.hasParent()) {
			moveScenegraphChildren(core::Queries::filterForMoveableScenegraphChildren(*project(), {newNode}, sceneRootNode), sceneRootNode);
		}
		LOG_DEBUG(log_system::CONTEXT, "All nodes traversed.");

		LOG_DEBUG(log_system::CONTEXT, "Applying scenegraph node transformations...");
		ValueHandle newNodeHandle{newNode};

		auto transformNode = [this](auto& valueHandle, auto& propertyName, auto& vec3f) {
			set(valueHandle.get(propertyName).get("x"), vec3f[0]);
			set(valueHandle.get(propertyName).get("y"), vec3f[1]);
			set(valueHandle.get(propertyName).get("z"), vec3f[2]);
		};

		transformNode(newNodeHandle, "scaling", meshScenegraphNode.transformations.scale);
		transformNode(newNodeHandle, "rotation", meshScenegraphNode.transformations.rotation);
		transformNode(newNodeHandle, "translation", meshScenegraphNode.transformations.translation);
		LOG_DEBUG(log_system::CONTEXT, "All scenegraph node transformations applied.");
	}
	LOG_INFO(log_system::CONTEXT, "All scenegraph nodes imported.");

	LOG_INFO(log_system::CONTEXT, "Restoring scenegraph structure...");
	for (size_t i{0}; i < scenegraph.nodes.size(); ++i) {
		auto meshScenegraphNode = scenegraph.nodes[i];
		if (meshScenegraphNode.has_value() && meshScenegraphNode->hasParent()) {
			moveScenegraphChildren(core::Queries::filterForMoveableScenegraphChildren(*project(), {meshScenegraphNodes[i]}, meshScenegraphNodes[meshScenegraphNode->parentIndex]), meshScenegraphNodes[meshScenegraphNode->parentIndex]);
		}
	}
	LOG_INFO(log_system::CONTEXT, "Scenegraph structure restored.");


	LOG_INFO(log_system::CONTEXT, "Importing animation samplers...");
	std::map<int, std::vector<SEditorObject>> sceneChannels;
	for (auto animIndex = 0; animIndex < scenegraph.animationSamplers.size();  ++animIndex) {
		auto& samplers = scenegraph.animationSamplers[animIndex];
		for (auto samplerIndex = 0; samplerIndex < samplers.size(); ++samplerIndex) {
			auto& meshAnimSampler = scenegraph.animationSamplers.at(animIndex)[samplerIndex];
			if (!meshAnimSampler.has_value()) {
				LOG_DEBUG(log_system::CONTEXT, "Found disabled mesh animation sampler at index {}.{}, ignoring AnimationChannel creat�on...", animIndex, samplerIndex);
				sceneChannels[animIndex].emplace_back(nullptr);
				continue;
			}

			auto samplerWithSameProperties = propertiesToChannelMap.find({absPath, animIndex, samplerIndex});
			if (samplerWithSameProperties == propertiesToChannelMap.end()) {
				LOG_DEBUG(log_system::CONTEXT, "Did not find existing local AnimationChannel with same properties as asset animation sampler, creating one instead...");
				auto& sampler = sceneChannels[animIndex].emplace_back(createObject(raco::user_types::AnimationChannel::typeDescription.typeName, fmt::format("{}", *meshAnimSampler)));
				set({sampler, {"uri"}}, relativeFilePath.string());
				set({sampler, {"animationIndex"}}, animIndex);
				set({sampler, {"samplerIndex"}}, samplerIndex);
			} else {
				LOG_DEBUG(log_system::CONTEXT, "Found existing local AnimationChannel '{}' with same properties as asset animation sampler, using this AnimationChannel...", *meshAnimSampler);
				sceneChannels[animIndex].emplace_back(samplerWithSameProperties->second);
			}

		}
	}
	LOG_INFO(log_system::CONTEXT, "Animation samplers imported.");

	LOG_INFO(log_system::CONTEXT, "Importing animations...");
	for (auto animationIndex = 0; animationIndex < scenegraph.animations.size(); ++animationIndex) {
		std::vector<SEditorObject> scenegraphAnims;
		if (!scenegraph.animations[animationIndex].has_value()) {
			LOG_DEBUG(log_system::CONTEXT, "Found disabled animation at index {}, ignoring Animation...", animationIndex);
			scenegraphAnims.emplace_back(nullptr);
			continue;
		}

		auto& meshAnim = *scenegraph.animations[animationIndex];
		auto samplerSize = scenegraph.animationSamplers.at(animationIndex).size();
		auto& newAnim = scenegraphAnims.emplace_back(createObject(raco::user_types::Animation::typeDescription.typeName, fmt::format("{}", meshAnim.name)));
		newAnim->as<raco::user_types::Animation>()->setChannelAmount(samplerSize);
		moveScenegraphChildren({newAnim}, sceneRootNode);
		LOG_INFO(log_system::CONTEXT, "Assigning animation samplers to animation '{}'...", meshAnim.name);
		for (auto samplerIndex = 0; samplerIndex < samplerSize; ++samplerIndex) {
			if (!sceneChannels[animationIndex][samplerIndex]) {
				continue;
			}

			set({newAnim, {"animationChannels", fmt::format("Channel {}", samplerIndex)}}, sceneChannels[animationIndex][samplerIndex]);
			LOG_DEBUG(log_system::CONTEXT, "Assigned sampler to anim channel {}", samplerIndex);
		}
		LOG_INFO(log_system::CONTEXT, "Samplers assigned.", meshAnim.name);

		LOG_INFO(log_system::CONTEXT, "Linking samplers of animation '{}' to imported nodes...", meshAnim.name);
		for (auto channelIndex = 0; channelIndex < meshAnim.channels.size(); ++channelIndex) {
			auto& channel = meshAnim.channels[channelIndex];
			auto& linkStartAnim = scenegraphAnims.back();
			if (!linkStartAnim || !meshScenegraphNodes[channel.nodeIndex] || !sceneChannels[animationIndex][channel.samplerIndex]) {
				LOG_DEBUG(log_system::CONTEXT, "Link impossible because at least one of the scene elements is missing (animation and/or sampler and/or node) - skipping link creation...");
				continue;
			}

			auto &linkEndNode = meshScenegraphNodes[channel.nodeIndex];
			ValueHandle linkEndProp;
			auto& animTargetProp = channel.targetPath;

			if (animTargetProp == "translation") {
				linkEndProp = {linkEndNode, {"translation"}};
			} else if (animTargetProp == "rotation") {
				linkEndProp = {linkEndNode, {"rotation"}};
			} else if (animTargetProp == "scale") {
				linkEndProp = {linkEndNode, {"scaling"}};
			} else if (animTargetProp == "weights") {
				LOG_WARNING(log_system::CONTEXT, "Animation sampler of animation '{}' at index {} has animation target 'weights' which is unsupported - skipping link...", meshAnim.name, channelIndex);
				continue;
			} else {
				LOG_ERROR(log_system::CONTEXT, "Animation sampler of animation '{}' at index {} has invalid animation target '{}' - skipping link...", meshAnim.name, channelIndex, animTargetProp);
				continue;
			}

			auto animChannelOutputName = linkStartAnim->as<user_types::Animation>()->createAnimChannelOutputName(channel.samplerIndex, sceneChannels[animationIndex][channel.samplerIndex]->objectName());
			auto linkStart = ValueHandle{linkStartAnim, {"outputs", animChannelOutputName}};
			if (Queries::linkWouldBeAllowed(*project(), linkStart.getDescriptor(), linkEndProp.getDescriptor(), false) &&
				Queries::linkWouldBeValid(*project(), linkStart.getDescriptor(), linkEndProp.getDescriptor())) {
				addLink(linkStart, linkEndProp, false);
			} else {
				LOG_WARNING(log_system::CONTEXT, "Output property '{}' of Node '{}' can't be linked with Animation '{}' - either the property is already linked or types are incompatible", animTargetProp, linkEndProp.rootObject()->objectName(), linkStartAnim->objectName());
			}
		}
		LOG_INFO(log_system::CONTEXT, "Samplers linked.");
	}
	LOG_INFO(log_system::CONTEXT, "Animations imported.");

	if (project_->featureLevel() >= user_types::Skin::typeDescription.featureLevel) {
		for (auto index = 0; index < scenegraph.skins.size(); index++) {
			const auto& sceneSkin = scenegraph.skins[index];
			if (!sceneSkin.has_value()) {
				LOG_DEBUG(log_system::CONTEXT, "Found disabled skin at index {}, ignoring...", index);
				continue;
			}

			std::vector<SEditorObject> targetMeshNodes;
			auto targetMeshNode = meshScenegraphNodes[sceneSkin->meshNodeIndex];
			if (targetMeshNode->isType<user_types::MeshNode>()) {
				targetMeshNodes.emplace_back(targetMeshNode);
			} else {
				auto submeshRootNode = targetMeshNode->children_->get(0)->asRef()->as<user_types::Node>();
				for (auto child : submeshRootNode->children_->asVector<SEditorObject>()) {
					if (child->isType<user_types::MeshNode>()) {
						targetMeshNodes.emplace_back(child);
					} else {
						LOG_ERROR(log_system::CONTEXT, "Target child node is not a MeshNode '{}'", child->objectName());
					}
				}
			}
			if (!targetMeshNodes.empty()) {
				auto skinObj = createObject(user_types::Skin::typeDescription.typeName, sceneSkin->name);
				set({skinObj, &user_types::Skin::uri_}, relativeFilePath.string());
				skinObj->as<user_types::Skin>()->setupTargetProperties(targetMeshNodes.size());
				moveScenegraphChildren({skinObj}, sceneRootNode);
				for (auto index = 0; index < targetMeshNodes.size(); index++) {
					set(ValueHandle(skinObj, &user_types::Skin::targets_)[index], targetMeshNodes[index]);
				}
				for (auto jointIndex = 0; jointIndex < sceneSkin->jointNodeIndices.size(); jointIndex++) {
					set(ValueHandle(skinObj, &user_types::Skin::joints_)[jointIndex], meshScenegraphNodes[sceneSkin->jointNodeIndices[jointIndex]]);
				}
			}
		}
	}
}

SLink BaseContext::addLink(const ValueHandle& start, const ValueHandle& end, bool isWeak) {
	// Remove existing links ending on the property or any nested child properties
	for (auto link : Queries::getLinksConnectedToPropertySubtree(*project_, end, false, true)) {
		removeLink(link->endProp());
	}

	auto link = std::make_shared<Link>(start.getDescriptor(), end.getDescriptor(), true, isWeak);

	project_->addLink(link);
	changeMultiplexer_.recordAddLink(link->descriptor());
	updateBrokenLinkErrors(*link->endObject_);
	return link;
}

void BaseContext::removeLink(const PropertyDescriptor& end) {
	if (auto link = Queries::getLink(*project_, end)) {
		project_->removeLink(link);
		changeMultiplexer_.recordRemoveLink(link->descriptor());
		if (ValueHandle vh{end}; vh) {
			// The end property might not exist anymore: we do not remove links when an end property vanishes
			// (e. g. because a shader is updated). So only call recordValueChanged if the property exists.
			changeMultiplexer_.recordValueChanged(vh);
		}
		updateBrokenLinkErrors(*link->endObject_);
	}
}

void BaseContext::updateExternalReferences(LoadContext& loadContext) {
	ExtrefOperations::updateExternalObjects(*this, project(), *externalProjectsStore(), loadContext);
	PrefabOperations::globalPrefabUpdate(*this, true);
}

std::vector<SEditorObject> BaseContext::getTopLevelObjectsFromDeserializedObjects(serialization::ObjectsDeserialization& deserialization, Project* project) {
	SEditorObjectSet childrenSet;

	restoreReferences(*project, deserialization.objects, deserialization);

	for (const auto& obj : deserialization.objects) {
		for (const auto& objChild : obj->children_->asVector<SEditorObject>()) {
			childrenSet.emplace(objChild);
		}
	}

	std::vector<SEditorObject> topLevelObjects;
	for (const auto& obj : deserialization.objects) {
		if (childrenSet.find(obj) == childrenSet.end()) {
			topLevelObjects.emplace_back(obj);
		}
	}

	return topLevelObjects;
}

}  // namespace raco::core
