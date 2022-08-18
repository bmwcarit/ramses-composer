/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/PrefabOperations.h"

#include "core/EditorObject.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"

#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/LuaInterface.h"

#include <unordered_set>

namespace raco::core {

using namespace user_types;

// Returns the containing prefab if the object is a direct or indirect child object of a prefab;
// otherwise nullptr is returned.
SPrefab PrefabOperations::findContainingPrefab(SEditorObject object) {
	SEditorObject current = object;
	while (current) {
		if (auto prefab = current->as<Prefab>()) {
			return prefab;
		}
		current = current->getParent();
	}
	return nullptr;
}

// Returns the containing prefab instance if the object is a direct or indirect child object of a prefab instance;
// otherwise nullptr is returned.
// - for nested instantiations: returns the first PrefabInstance when walking up the tree, not the uppermost one.
SPrefabInstance PrefabOperations::findContainingPrefabInstance(SEditorObject object) {
	SEditorObject current = object;
	while (current) {
		if (current->isType<PrefabInstance>()) {
			return current->as<PrefabInstance>();
		}
		current = current->getParent();
	}
	return nullptr;
}

SPrefabInstance PrefabOperations::findOuterContainingPrefabInstance(SEditorObject object) {
	auto currentInst = PrefabOperations::findContainingPrefabInstance(object);
	if (currentInst) {
		while (auto parentInst = PrefabOperations::findContainingPrefabInstance(currentInst->getParent())) {
			currentInst = parentInst;
		}
	}
	return currentInst;
}

bool PrefabOperations::isInterfaceObject(SEditorObject object) {
	if (!object->as<LuaInterface>()) {
		return false;
	}
	auto inst = PrefabOperations::findOuterContainingPrefabInstance(object);
	return inst && object->getParent() == inst;
}

bool PrefabOperations::isInterfaceProperty(const ValueHandle& prop) {
	return isInterfaceObject(prop.rootObject()) && prop.depth() >= 1 && prop.getPropertyNamesVector()[0] == "inputs";
}

bool PrefabOperations::isPrefabInterfaceObject(SEditorObject object) {
	if (!object->as<LuaInterface>()) {
		return false;
	}
	auto prefab = PrefabOperations::findContainingPrefab(object);
	return prefab && object->getParent() == prefab;
}

bool PrefabOperations::isPrefabInterfaceProperty(const PropertyDescriptor& prop) {
	return isPrefabInterfaceObject(prop.object()) && prop.propertyNames().size() >= 1 && prop.propertyNames()[0] == "inputs";
}


namespace {

SLink lookupLink(SLink srcLink, const std::map<std::string, std::set<SLink>>& destLinks, translateRefFunc translateRef) {
	auto transStartObj = translateRef(*srcLink->startObject_);
	auto transEndObj = translateRef(*srcLink->endObject_);

	auto it = destLinks.find(transEndObj->objectID());
	if (it != destLinks.end()) {
		for (const auto& destLink : it->second) {
			if (transStartObj == *destLink->startObject_ &&
				transEndObj == *destLink->endObject_ &&
				*srcLink->startProp_ == *destLink->startProp_ &&
				*srcLink->endProp_ == *destLink->endProp_) {
				return destLink;
			}
		}
	}
	return nullptr;
}

}  // namespace

// Update operation
// - detect create & move into as creations
// - detect delete & move out as deletions
// - selective update of single properties according to the changerecorder entries for the prefab subtree
// - change recorder will be used as input for the dirty parts of the prefab and output for the changes in
//   the prefab instance and its children
void PrefabOperations::updatePrefabInstance(BaseContext& context, const SPrefab& prefab, SPrefabInstance instance, bool instanceDirty, bool propagateMissingInterfaceProperties) {
	using namespace raco::core;
	DataChangeRecorder localChanges;

	if (instance->query<ExternalReferenceAnnotation>()) {
		return;
	}

	std::unordered_set<SEditorObject> prefabChildren;
	std::copy(++TreeIteratorAdaptor(prefab).begin(), TreeIteratorAdaptor(prefab).end(), std::inserter(prefabChildren, prefabChildren.end()));

	SEditorObjectSet instanceChildren;
	std::copy(++TreeIteratorAdaptor(instance).begin(), TreeIteratorAdaptor(instance).end(), std::inserter(instanceChildren, instanceChildren.end()));

	std::map<SEditorObject, SEditorObject> mapToInstance;
	std::map<SEditorObject, SEditorObject> mapToPrefab;
	mapToInstance[prefab] = instance;
	mapToPrefab[instance] = prefab;

	for (auto instChild : instanceChildren) {
		auto prefabChildID = PrefabInstance::mapObjectIDFromInstance(instChild, prefab, instance);
		auto prefabChild = context.project()->getInstanceByID(prefabChildID);
		mapToInstance[prefabChild] = instChild;
		mapToPrefab[instChild] = prefabChild;
	}

	auto translateRefFunc = [prefab, instance, &mapToInstance](SEditorObject obj) -> SEditorObject {
		auto it = mapToInstance.find(obj);
		if (it != mapToInstance.end()) {
			return it->second;
		}
		return obj;
	};

	auto translateRefToPrefabFunc = [prefab, instance, &mapToPrefab](SEditorObject obj) -> SEditorObject {
		auto it = mapToPrefab.find(obj);
		if (it != mapToPrefab.end()) {
			return it->second;
		}
		return obj;
	};

	auto isPrefabInstanceInterfaceObject = [instance](SEditorObject object) -> bool {
		return object->getParent() == instance && object->isType<LuaInterface>();
	};

	auto isPrefabInterfaceObject = [prefab](SEditorObject object) -> bool {
		return object->getParent() == prefab && object->isType<LuaInterface>();
	};

	// Check if a property of a prefab child object is an interface property.
	auto isPrefabInterfaceProperty = [isPrefabInterfaceObject, prefab](const ValueHandle& prop) {
		return isPrefabInterfaceObject(prop.rootObject()) && prop.depth() >= 1 && prop.getPropertyNamesVector()[0] == "inputs";
	};


	// Delete prefab instance children who don't have corresponding prefab children
	{
		std::vector<SEditorObject> toRemove;
		auto it = instanceChildren.begin();
		while (it != instanceChildren.end()) {
			auto instChild = *it;

			auto prefabIt = mapToPrefab.find(instChild);
			if (prefabIt == mapToPrefab.end() ||
				prefabChildren.find(prefabIt->second) == prefabChildren.end()) {
				toRemove.emplace_back(instChild);
				mapToInstance.erase(prefabIt->second);
				mapToPrefab.erase(instChild);
				it = instanceChildren.erase(it);
			} else {
				++it;
			}
		}
		context.deleteObjects(toRemove, true, false);
	}

	std::set<ValueHandle> allChangedValues;

	// Remove links
	std::map<std::string, std::set<SLink>> prefabLinks = Queries::getLinksConnectedToObjects(*context.project(), prefabChildren, false, true);
	std::map<std::string, std::set<SLink>> instLinks = Queries::getLinksConnectedToObjects(*context.project(), instanceChildren, false, true);

	for (const auto& instLinkCont : instLinks) {
		for (const auto& instLink : instLinkCont.second) {
			// Don't remove links ending on top-level lua interfaces:
			// These are the only changeably properties in the prefab instance children.
			if (!isPrefabInstanceInterfaceObject(*instLink->endObject_)) {
				if (!lookupLink(instLink, prefabLinks, translateRefToPrefabFunc)) {
					context.project()->removeLink(instLink);
					localChanges.recordRemoveLink(instLink->descriptor());

					// If we remove links we need to start propagating the property value again.
					// To ensure this we create a changed value entry to be processed below.
					auto instEndObject = *instLink->endObject_;
					auto prefabEndObject = mapToPrefab[instEndObject];

					ValueHandle prefabEndHandle = ValueHandle::translatedHandle(ValueHandle(instLink->endProp()), prefabEndObject);
					if (prefabEndHandle) {
						allChangedValues.insert(prefabEndHandle);
					}
				}
			}
		}
	}

	// Create new prefab instance child objects for children of prefab
	std::vector<std::pair<SEditorObject, SEditorObject>> createdObjects;
	for (auto prefabChild : prefabChildren) {
		auto it = mapToInstance.find(prefabChild);
		if (it == mapToInstance.end()) {
			auto instChildID = PrefabInstance::mapObjectIDToInstance(prefabChild, prefab, instance);
			auto newInstChild = context.objectFactory()->createObject(prefabChild->getTypeDescription().typeName, prefabChild->objectName(), instChildID);
			mapToInstance[prefabChild] = newInstChild;
			mapToPrefab[newInstChild] = prefabChild;
			context.project()->addInstance(newInstChild);
			localChanges.recordCreateObject(newInstChild);
			createdObjects.emplace_back(prefabChild, newInstChild);
			
			// If we add an object we must also update the children property of the scenegraph parent
			// Normally this property should already be present in the model changes, but
			// since the save file optimization may remove objects this is not guaranteed directly after loading.
			if (prefabChild->getParent()) {
				allChangedValues.insert(ValueHandle(prefabChild->getParent(), &EditorObject::children_));
			}
		}
	}

	// Complete update of the the newly created objects
	for (auto [prefabChild, instChild] : createdObjects) {
		// Object IDs are never updated and the object name for newly created objects is already correct.
		UndoHelpers::updateEditorObject(
			prefabChild.get(), instChild, translateRefFunc,
			[](const std::string& propName) {
				return propName == "objectID";
			},
			*context.objectFactory(),
			&localChanges, true, false);
	}


	// Update prefab children objects which have creation records in the context model changes but we already have a corresponding 
	// prefabinstance child object. These have not been created and updated above. They may contain properties without value changed
	// records which we still need to update.
	// - Since the prefab instance child object already exists we do not update interface properties here.
	// - This might lead to duplicate work if the properties we update here also have a value changed entry in the model changes.
	// Use case: the save file optimization will remove extref Prefab contents but not the local prefab instance interface scripts.
	for (auto object : context.modelChanges().getCreatedObjects()) {
		if (prefabChildren.find(object) != prefabChildren.end()) {
			auto instChild = mapToInstance.find(object)->second;
			UndoHelpers::updateEditorObject(
				object.get(), instChild, translateRefFunc,
				[object, &isPrefabInterfaceProperty](const std::string& propName) {
					return propName == "objectID" || isPrefabInterfaceProperty({object, {propName}});
				},
				*context.objectFactory(),
				&localChanges, true, false);
		}
	}

	// Single property updates from model changes
	auto const& modelChanges = context.modelChanges().getChangedValues();
	if (instanceDirty || context.modelChanges().hasValueChanged(ValueHandle(prefab, &EditorObject::children_))) {
		UndoHelpers::updateSingleValue(&prefab->children_, &instance->children_, ValueHandle(instance, &EditorObject::children_), translateRefFunc, &localChanges, true);
	}

	for (const auto& [id, cont] : modelChanges) {
		allChangedValues.insert(cont.begin(), cont.end());
	}
	for (const auto& prop : allChangedValues) {
		if (prop.rootObject() != prefab && prefabChildren.find(prop.rootObject()) != prefabChildren.end()) {
			if (std::find_if(createdObjects.begin(), createdObjects.end(), [prop](auto item) {
					return prop.rootObject() == item.first;
				}) == createdObjects.end()) {
				auto it = mapToInstance.find(prop.rootObject());
				assert(it != mapToInstance.end());
				auto inst = it->second;
				ValueHandle instProp = ValueHandle::translatedHandle(prop, inst);

				if (!isPrefabInterfaceProperty(prop)) {
					UndoHelpers::updateSingleValue(prop.valueRef(), instProp.valueRef(), instProp, translateRefFunc, &localChanges, true);
				}
			}
		}
	}

	for (const auto& prefabLinkCont : prefabLinks) {
		for (const auto& prefabLink : prefabLinkCont.second) {
			// Don't propagate links ending on interface scripts:
			if (!isPrefabInterfaceObject(*prefabLink->endObject_)) {
				auto instLink = lookupLink(prefabLink, instLinks, translateRefFunc);
				if (!instLink) {
					// Create link
					auto destLink = Link::cloneLinkWithTranslation(prefabLink, translateRefFunc);
					context.project()->addLink(destLink);
					localChanges.recordAddLink(destLink->descriptor());
				} else if (*instLink->isWeak_ != *prefabLink->isWeak_) {
					// strong <-> weak link transitions are handled as removal and creation operation
					// the Project::removeLink/addLink calls are needed to update the link graph map correctly.
					context.project()->removeLink(instLink);
					localChanges.recordRemoveLink(instLink->descriptor());
					instLink->isWeak_ = *prefabLink->isWeak_;
					instLink->isValid_ = prefabLink->isValid();
					context.project()->addLink(instLink);
					localChanges.recordAddLink(instLink->descriptor());
				} else if (instLink->isValid() != prefabLink->isValid()) {
					// update link validity
					instLink->isValid_ = prefabLink->isValid();
					localChanges.recordChangeValidityOfLink(instLink->descriptor());
				}
			}
		}
	}

	// Update volatile data for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterDeserialization();
	}

	context.modelChanges().mergeChanges(localChanges);
	context.uiChanges().mergeChanges(localChanges);

	// Sync from external files for new or changed objects
	auto changedObjects = localChanges.getAllChangedObjects();
	context.performExternalFileReload({changedObjects.begin(), changedObjects.end()});
}

void PrefabOperations::prefabUpdateOrderDepthFirstSearch(SPrefab current, std::vector<SPrefab>& order) {
	if (std::find(order.begin(), order.end(), current) == order.end()) {
		for (auto weak_inst : current->instances_) {
			if (auto inst = weak_inst.lock()) {
				if (!PrefabOperations::findContainingPrefabInstance(inst->getParent())) {
					if (auto inst_prefab = PrefabOperations::findContainingPrefab(inst)) {
						prefabUpdateOrderDepthFirstSearch(inst_prefab, order);
					}
				}
			}
		}
		order.emplace_back(current);
	}
}

// Prefab update order
// - prefab B needs to be updated after prefab A if B contains a prefab instance of A
// @return reverse update order
std::vector<SPrefab> prefabUpdateOrder(const Project& project) {
	std::vector<SPrefab> result;
	for (auto obj : project.instances()) {
		if (auto prefab = obj->as<Prefab>()) {
			PrefabOperations::prefabUpdateOrderDepthFirstSearch(prefab, result);
		}
	}
	return result;
}

bool prefabDirty(const DataChangeRecorder& changes, SPrefab prefab) {
	for (auto obj : changes.getAllChangedObjects(false, false, true)) {
		auto parentPrefab = PrefabOperations::findContainingPrefab(obj);
		if (parentPrefab && parentPrefab == prefab) {
			return true;
		}
	}
	return false;
}

// Prefab instance is dirty if the template property has changed or the instance was newly created
bool prefabInstanceDirty(const DataChangeRecorder& changes, SPrefabInstance instance) {
	auto& createdObjects = changes.getCreatedObjects();
	if (std::find_if(createdObjects.begin(), createdObjects.end(), [instance](const ValueHandle& handle) {
			return handle.rootObject() == instance;
		}) != createdObjects.end()) {
		return true;
	}
	ValueHandle templateHandle(instance, &PrefabInstance::template_);
	return changes.hasValueChanged(templateHandle);
}

void PrefabOperations::globalPrefabUpdate(BaseContext& context, bool propagateMissingInterfaceProperties) {
	// Build prefab update order from dependency graph
	auto order = prefabUpdateOrder(*context.project());

	// Remove children from prefab instances which set the template property to nullptr:
	std::vector<SPrefabInstance> prefabInstances;
	for (auto obj : context.project()->instances()) {
		if (auto inst = obj->as<PrefabInstance>()) {
			if (prefabInstanceDirty(context.modelChanges(), inst) && *inst->template_ == nullptr && !findContainingPrefabInstance(inst->getParent())) {
				prefabInstances.emplace_back(inst);
			}
		}
	}
	for (auto inst : prefabInstances) {
		if (std::find(context.project()->instances().begin(), context.project()->instances().end(), inst) != context.project()->instances().end()) {
			auto children = inst->children_->asVector<SEditorObject>();
			context.deleteObjects(children);
		}
	}

	for (auto it = order.rbegin(); it != order.rend(); ++it) {
		auto prefab = (*it)->as<Prefab>();
		bool prefab_dirty = prefabDirty(context.modelChanges(), prefab);
		for (auto weak_inst : prefab->instances_) {
			if (auto inst = weak_inst.lock()->as<PrefabInstance>()) {
				if (!findContainingPrefabInstance(inst->getParent())) {
					bool inst_dirty = prefabInstanceDirty(context.modelChanges(), inst);
					if (inst_dirty || prefab_dirty) {
						updatePrefabInstance(context, prefab, inst, inst_dirty, propagateMissingInterfaceProperties);
					}
				}
			}
		}
	}
}

}  // namespace raco::core