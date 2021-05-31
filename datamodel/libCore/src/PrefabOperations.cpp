/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/PrefabOperations.h"

#include "core/EditorObject.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/ExternalReferenceAnnotation.h"

#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

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
		if (auto inst = current->as<PrefabInstance>()) {
			return inst;
		}
		current = current->getParent();
	}
	return nullptr;
}

bool compareLinks(const raco::core::Link& left, const raco::core::Link& right, translateRefFunc translateRef) {
	return translateRef(*left.startObject_) == *right.startObject_ &&
		   translateRef(*left.endObject_) == *right.endObject_ &&
		   *left.startProp_ == *right.startProp_ &&
		   *left.endProp_ == *right.endProp_;
}


// Update operation
// - detect create & move into as creations
// - detect delete & move out as deletions
// - selective update of single properties according to the changerecorder entries for the prefab subtree
// - change recorder will be used as input for the dirty parts of the prefab and output for the changes in
//   the prefab instance and its children
void PrefabOperations::updatePrefabInstance(BaseContext& context, const SPrefab& prefab, SPrefabInstance instance, bool instanceDirty) {
	using namespace raco::core;
	DataChangeRecorder localChanges;

	 if (instance->query<ExternalReferenceAnnotation>()) {
		return;
	 }

	std::set<SEditorObject> prefabChildren;
	std::copy(++TreeIteratorAdaptor(prefab).begin(), TreeIteratorAdaptor(prefab).end(), std::inserter(prefabChildren, prefabChildren.end()));

	std::set<SEditorObject> instanceChildren;
	std::copy(++TreeIteratorAdaptor(instance).begin(), TreeIteratorAdaptor(instance).end(), std::inserter(instanceChildren, instanceChildren.end()));

	std::map<SEditorObject, SEditorObject> mapToInstance;
	mapToInstance[prefab] = instance;
	for (size_t index = 0; index < instance->mapToInstance_->size(); index++) {
		const Table& item = instance->mapToInstance_->get(index)->asTable();
		auto prefabChild = item.get(0)->asRef();
		auto instChild = item.get(1)->asRef();
		mapToInstance[prefabChild] = instChild;
	}

	auto translateRefFunc = [prefab, instance, &mapToInstance](SEditorObject obj) -> SEditorObject {
		auto it = mapToInstance.find(obj);
		if (it != mapToInstance.end()) {
			return it->second;
		}
		return obj;
	};

	// Delete prefab instance children who don't have corresponding prefab children
	{
		std::vector<SEditorObject> toRemove;
		auto it = instanceChildren.begin();
		while (it != instanceChildren.end()) {
			auto instChild = *it;
			auto prefabChild = PrefabInstance::mapFromInstance(instChild, instance);
			if (!prefabChild ||
				std::find(prefabChildren.begin(), prefabChildren.end(), prefabChild) == prefabChildren.end()) {
				toRemove.emplace_back(instChild);
				instance->removePrefabInstanceChild(context, prefabChild);
				mapToInstance.erase(prefabChild);
				it = instanceChildren.erase(it);
			} else {
				++it;
			}
		}
		context.deleteObjects(toRemove);
	}

	std::set<ValueHandle> allChangedValues;

	// Remove links
	// TODO: Replace vectors with sets and don't use std::find_if
	std::vector<SLink> prefabLinks = Queries::getLinksConnectedToObjects(*context.project(), prefabChildren, false, true);
	std::vector<SLink> instLinks = Queries::getLinksConnectedToObjects(*context.project(), instanceChildren, false, true);
	for (auto instLink : instLinks) {
		if (std::find_if(prefabLinks.begin(), prefabLinks.end(), [instLink, translateRefFunc](const SLink& prefabLink) {
				return compareLinks(*prefabLink, *instLink, translateRefFunc);
			}) == prefabLinks.end()) {

			// Don't remove links ending on top-level lua scripts:
			// These are the only changeably properties in the prefab instance children.
			auto instEndObject = *instLink->endObject_;
			if (instEndObject->as<user_types::LuaScript>() && instEndObject->getParent() == instance) {
				continue;
			}

			context.project()->removeLink(instLink);
			localChanges.recordRemoveLink(instLink->descriptor());

			auto prefabEndObject = PrefabInstance::mapFromInstance(instEndObject, instance);
			ValueHandle prefabEndHandle = ValueHandle::translatedHandle(ValueHandle(instLink->endProp()), prefabEndObject);
			if (prefabEndHandle) {
				allChangedValues.insert(prefabEndHandle);
			}
		}
	}

	// Create new prefab instance child objects for children of prefab
	std::vector<std::pair<SEditorObject, SEditorObject>> createdObjects;
	for (auto prefabChild : prefabChildren) {
		auto it = mapToInstance.find(prefabChild);
		if (it == mapToInstance.end()) {
			auto newInstChild = context.objectFactory()->createObject(prefabChild->getTypeDescription().typeName, prefabChild->objectName());
			instance->addChildMapping(context, prefabChild, newInstChild);
			mapToInstance[prefabChild] = newInstChild;
			context.project()->addInstance(newInstChild);
			localChanges.recordCreateObject(newInstChild);
			createdObjects.emplace_back(prefabChild, newInstChild);
		}
	}

	// Complete update of the the newly created objects
	for (auto [prefabChild, instChild]: createdObjects) {
		// Object IDs are never updated and the object name for newly created objects is already correct.
		updateEditorObject(prefabChild.get(), instChild, translateRefFunc, 
			[](const std::string& propName) {
				return propName == "objectID" || propName == "mapToInstance";
			},
			*context.objectFactory(),
			&localChanges, true, false);
	}

	// Single property updates from model changes
	auto const & modelChanges = context.modelChanges().getChangedValues();
	if (instanceDirty || modelChanges.find(ValueHandle(prefab, {"children"})) != modelChanges.end()) {
		updateSingleValue(&prefab->children_, &instance->children_, ValueHandle(instance, {"children"}), translateRefFunc, & localChanges, true);
	}

	std::copy(modelChanges.begin(), modelChanges.end(), std::inserter(allChangedValues, allChangedValues.end()));
	for (const auto& prop : allChangedValues) {
		if (prop.rootObject() != prefab && prefab == findContainingPrefab(prop.rootObject())) {
			if (std::find_if(createdObjects.begin(), createdObjects.end(), [prop](auto item) {
					return prop.rootObject() == item.first;
				}) == createdObjects.end()) {
				if (prop.rootObject()->getParent() == prefab &&
					prop.rootObject()->as<LuaScript>() && prop.depth() >= 1 && prop.getPropertyNamesVector()[0] == "luaInputs") {
					continue;
				}

				auto it = mapToInstance.find(prop.rootObject());
				assert(it != mapToInstance.end());
				auto inst = it->second;
				ValueHandle instProp = ValueHandle::translatedHandle(prop, inst);
				updateSingleValue(prop.valueRef(), instProp.valueRef(), instProp, translateRefFunc, &localChanges, true);
			}
		}
	}

	for (const auto& prefabLink : prefabLinks) {
		auto instLinksIt = std::find_if(instLinks.begin(), instLinks.end(), [prefabLink, translateRefFunc](const SLink& instLink) {
			return compareLinks(*prefabLink, *instLink, translateRefFunc);
		});
		if (instLinksIt == instLinks.end()) {
			// Create links

			// Special case for links ending on top-level lua scripts: only add links if the lua script is among the created objects.
			auto prefabEndObject = *prefabLink->endObject_;
			if (prefabEndObject->as<user_types::LuaScript>() && prefabEndObject->getParent() == prefab &&
				std::find_if(createdObjects.begin(), createdObjects.end(), [prefabEndObject](auto item) {
					return item.first == prefabEndObject;
				}) == createdObjects.end()) {
				continue;
			}

			auto destLink = Link::cloneLinkWithTranslation(prefabLink, translateRefFunc);
			context.project()->addLink(destLink);
			localChanges.recordAddLink(destLink->descriptor());
		} else if ((*instLinksIt)->isValid() != prefabLink->isValid()) {
			// update link validity
			(*instLinksIt)->isValid_ = prefabLink->isValid();
			localChanges.recordChangeValidityOfLink((*instLinksIt)->descriptor());
		}
	}

	// Update volatile data for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterDeserialization();
	}

	context.modelChanges().mergeChanges(localChanges);
	context.uiChanges().mergeChanges(localChanges);

	// Sync from external files for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterContextActivated(context);
	}
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
	ValueHandle templateHandle(instance, {"template"});
	auto changedValues = changes.getChangedValues();
	return (changedValues.find(templateHandle) != changedValues.end());
}


void PrefabOperations::globalPrefabUpdate(BaseContext& context, DataChangeRecorder& changes) {
	// Build prefab update order from dependency graph
	auto order = prefabUpdateOrder(*context.project());

	// Remove children from prefab instances which set the template property to nullptr:
	std::vector<SPrefabInstance> prefabInstances;
	for (auto obj : context.project()->instances()) {
		if (auto inst = obj->as<PrefabInstance>()) {
			if (prefabInstanceDirty(changes, inst) && *inst->template_ == nullptr && !findContainingPrefabInstance(inst->getParent())) {
				prefabInstances.emplace_back(inst);
			}
		}
	}
	for (auto inst : prefabInstances) {
		if (std::find(context.project()->instances().begin(), context.project()->instances().end(), inst) != context.project()->instances().end()) {
			auto children = inst->children_->asVector<SEditorObject>();
			context.deleteObjects(children);
			context.removeAllProperties({inst, {"mapToInstance"}});
		}
	}

	for (auto it = order.rbegin(); it != order.rend(); ++it) {
		auto prefab = (*it)->as<Prefab>();
		bool prefab_dirty = prefabDirty(changes, prefab);
		for (auto weak_inst : prefab->instances_) {
			if (auto inst = weak_inst.lock()->as<PrefabInstance>()) {
				if (!findContainingPrefabInstance(inst->getParent())) {
					bool inst_dirty = prefabInstanceDirty(changes, inst);
					if (inst_dirty || prefab_dirty) {
						updatePrefabInstance(context, prefab, inst, inst_dirty);
					}
				}
			}
		}
	}
}

}  // namespace raco::core