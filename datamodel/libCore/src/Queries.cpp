/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Queries.h"
#include "core/Queries_Tags.h"

#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"

#include "user_types/Animation.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaInterface.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderPass.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace raco::core {

std::vector<ValueHandle> Queries::findAllReferencesTo(Project const& project, std::vector<SEditorObject> const& objects) {
	std::vector<ValueHandle> refs;
	for (auto instance : project.instances()) {
		if (std::find(objects.begin(), objects.end(), instance) == objects.end()) {
			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
				if (prop.type() == PrimitiveType::Ref) {
					auto refValue = prop.asTypedRef<EditorObject>();
					if (refValue && (std::find(objects.begin(), objects.end(), refValue) != objects.end())) {
						refs.emplace_back(prop);
					}
				}
			}
		}
	}
	return refs;
}

std::vector<ValueHandle> Queries::findAllReferencesFrom(SEditorObjectSet const& objects) {
	std::vector<ValueHandle> refs;
	for (auto instance : objects) {
		for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
			if (prop.type() == PrimitiveType::Ref) {
				auto refValue = prop.asTypedRef<EditorObject>();
				if (refValue && objects.find(refValue) == objects.end()) {
					refs.emplace_back(prop);
				}
			}
		}
	}
	return refs;
}

std::vector<ValueHandle> Queries::findAllReferences(Project const& project) {
	std::vector<ValueHandle> refs;
	for (auto instance : project.instances()) {
		for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
			if (prop.type() == PrimitiveType::Ref) {
				auto refValue = prop.asTypedRef<EditorObject>();
				if (refValue) {
					refs.emplace_back(prop);
				}
			}
		}
	}
	return refs;
}

std::vector<ValueHandle> Queries::findAllReferences(const SEditorObject& object) {
	std::vector<ValueHandle> refs;
	for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(object))) {
		if (prop.type() == PrimitiveType::Ref) {
			auto refValue = prop.asTypedRef<EditorObject>();
			if (refValue) {
				refs.emplace_back(prop);
			}
		}
	}
	return refs;
}

std::vector<SEditorObject> Queries::findAllUnreferencedObjects(Project const& project, std::function<bool(SEditorObject)> predicate) {
	SEditorObjectSet referenced;
	std::set<std::string> referencedRenderableTags;
	std::set<std::string> referencedMaterialTags;

	// This is deliberately naive right now: only objects which are not referenced (directly or via tag) by anything
	// else count as "unreferenced". Render passes cannot be referenced by anything and are therefore never "unreferenced".
	// A less naive alternative would be to use the render passes as seed objects, and then gather all objects which
	// are referenced by them, but given that this function is mainly used to delete "unused" resources it
	// seems to be better to return fewer rather than more objects.
	for (auto instance : project.instances()) {
		if (instance->isType<user_types::RenderPass>()) {
			// Render passes cannot be referenced by anything - don't count them as unreferenced.
			referenced.insert(instance);
		}		
		if (auto renderLayer = instance->as<user_types::RenderLayer>()) {
			auto const& renderableTags = renderLayer->renderableTags();
			auto materialTags = renderLayer->materialFilterTags();
			referencedRenderableTags.insert(std::begin(renderableTags), std::end(renderableTags));
			referencedMaterialTags.merge(materialTags);
		}
		for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
			if (prop.type() == PrimitiveType::Ref) {
				auto refValue = prop.asTypedRef<EditorObject>();
				if (refValue) {
					referenced.insert(refValue);
				}
			}
		}
	}

	std::vector<SEditorObject> unreferenced;
	for (auto instance : project.instances()) {
		if (referenced.find(instance) != referenced.end()) {
			continue;
		}
		if (predicate && !predicate(instance)) {
			continue;
		}
		if (Queries::hasObjectAnyTag(instance->as<user_types::Node>(), referencedRenderableTags)) {
			continue;
		}
		if (Queries::hasObjectAnyTag(instance->as<user_types::RenderLayer>(), referencedRenderableTags)) {
			continue;
		}
		if (Queries::hasObjectAnyTag(instance->as<user_types::Material>(), referencedMaterialTags)) {
			continue;
		}
		unreferenced.emplace_back(instance);
	}
	return unreferenced;
}

bool Queries::canDeleteUnreferencedResources(const Project& project) {
	auto toRemove = Queries::findAllUnreferencedObjects(project, Queries::isResource);
	return !toRemove.empty();
}
bool Queries::canDuplicateObjects(const std::vector<SEditorObject>& objects, const Project& project) {
	if (objects.empty()) {
		return false;
	}

	auto parentForAllObjs = objects.front()->getParent();

	for (const auto& obj : objects) {
		if (obj->query<ExternalReferenceAnnotation>() || !Queries::canPasteIntoObject(project, obj->getParent()) || obj->getParent() != parentForAllObjs) {
			return false;
		}
	}

	return true;
}

bool Queries::isValidReferenceTarget(Project const& project, const ValueHandle& handle, SEditorObject object) {
	if (!handle.constValueRef()->canSetRef(object)) {
		return false;
	}
	auto handleRoot = handle.rootObject();

	if (object) {
		// Don't allow referencing any objects inside prefabs from objects not in the same prefab
		// References from a Prefab to its children are of course allowed.
		auto objPrefabRoot = PrefabOperations::findContainingPrefab(object->getParent());
		if (objPrefabRoot && objPrefabRoot != PrefabOperations::findContainingPrefab(handleRoot)) {
			return false;
		}
	}

	if (handle.isRefToProp(&user_types::PrefabInstance::template_)) {
		auto inst = handleRoot->as<user_types::PrefabInstance>();
		// We can only create loops if the PrefabInstance is nested inside a Prefab:
		if (auto prefab = PrefabOperations::findContainingPrefab(inst)) {
			// Now the actual loop check:
			// if the candidate prefab object depends via instantiations on the containing prefab
			// of the instance then we have found a loop and will discard the object from the
			// valid set.
			std::vector<user_types::SPrefab> downstreamPrefabs;
			PrefabOperations::prefabUpdateOrderDepthFirstSearch(prefab, downstreamPrefabs);

			if (std::find(downstreamPrefabs.begin(), downstreamPrefabs.end(), object) != downstreamPrefabs.end()) {
				return false;
			}
		}
	}

	return true;
}

std::vector<SEditorObject> Queries::findAllValidReferenceTargets(Project const& project, const ValueHandle& handle) {
	auto handleRoot = handle.rootObject();
	auto handlePrefabRoot = PrefabOperations::findContainingPrefab(handleRoot);

	std::vector<SEditorObject> valid;
	for (auto obj : project.instances()) {
		if (handle.constValueRef()->canSetRef(obj)) {
			// Don't allow referencing any objects inside prefabs from objects not in the same prefab
			// References from a Prefab to its children are of course allowed.
			auto objPrefabRoot = PrefabOperations::findContainingPrefab(obj->getParent());
			if (!objPrefabRoot || handlePrefabRoot == objPrefabRoot) {
				valid.push_back(obj);
			}
		}
	}

	// Special case: settings the "template" property of a PrefabInstance
	// -> filter out prefabs which would create a prefab instantiation loop
	auto inst = handleRoot->as<user_types::PrefabInstance>();
	if (inst && handle.getPropName() == "template" && handle.depth() == 1) {
		// We can only create loops if the PrefabInstance is nested inside a Prefab:
		if (auto prefab = PrefabOperations::findContainingPrefab(inst)) {
			// Now the actual loop check:
			// if the candidate prefab object depends via instantiations on the containing prefab 
			// of the instance then we have found a loop and will discard the object from the
			// valid set.
			std::vector<user_types::SPrefab> downstreamPrefabs;
			PrefabOperations::prefabUpdateOrderDepthFirstSearch(prefab, downstreamPrefabs);

			auto it = valid.begin();
			while (it != valid.end()) {
				auto targetPrefab = (*it)->as<user_types::Prefab>();
				assert(targetPrefab != nullptr);
				if (std::find(downstreamPrefabs.begin(), downstreamPrefabs.end(), targetPrefab) != downstreamPrefabs.end()) {
					it = valid.erase(it);
				} else {
					++it;
				}
			}
		}
	}

	return valid;
}

SEditorObject Queries::findById(const std::vector<SEditorObject>& objects, const std::string& id) {
	for (const auto obj : objects) {
		if (obj->objectID() == id) {
			return obj;
		}
	}
	return nullptr;
}

SEditorObject Queries::findByName(const std::vector<SEditorObject>& objects, const std::string& name) {
	for (const auto obj : objects) {
		if (obj->objectName() == name) {
			return obj;
		}
	}
	return nullptr;
}

SEditorObject Queries::findById(const Project& project, const std::string& id) {
	return Queries::findById(project.instances(), id);
}

ValueHandle Queries::findByIdAndPath(const Project& project, const std::string& object_id, const std::string& path) {
	if (auto editorObject{ findById(project, object_id) }) {
		size_t pos{ 0 };
		std::string_view remainder{ path };
		remainder = remainder.substr(editorObject->objectName().size() + 1);
		ValueHandle h{ editorObject };
		do {
			pos = remainder.find(".");
			auto part{ remainder.substr(0, pos) };
			remainder = remainder.substr(pos + 1, remainder.size() - 1 - pos);
			if (!(h = h.get(std::string{ part })))
				return {};
		} while (pos != std::string_view::npos);
		return h;
	}
	return {};
}

// Check if moving an object inside a prefab would cause a prefab instantiation loop.
// A loop would be caused by prefab instances within the subtree of the object which
// depend on the prefab by a prefab instantiation chain.
bool wouldObjectInPrefabCauseLoop(SEditorObject object, user_types::SPrefab prefab) {
	std::vector<user_types::SPrefab> downstreamPrefabs;
	PrefabOperations::prefabUpdateOrderDepthFirstSearch(prefab, downstreamPrefabs);
	for (auto child : TreeIteratorAdaptor(object)) {
		if (auto inst = child->as<user_types::PrefabInstance>()) {
			if (auto childPrefab = *inst->template_) {
				if (std::find(downstreamPrefabs.begin(), downstreamPrefabs.end(), childPrefab) != downstreamPrefabs.end()) {
					return true;
				}
			}
		}
	}

	return false;
}


bool Queries::canPasteIntoObject(Project const& project, SEditorObject const& object) { 
	// Can always paste into the toplevel:
	if (!object) {
		return true;
	}

	if (object->query<ExternalReferenceAnnotation>()) {
		return false;
    }

	if (isResource(object)) {
		return false;
	}

	if (object->as<user_types::LuaScript>() || object->as<user_types::LuaInterface>() || object->as<user_types::Animation>()) {
		return false;
	}

	// Can't modify PrefabInstance contents
	if (PrefabOperations::findContainingPrefabInstance(object)) {
		return false;
	}

	return true;
}

bool Queries::canPasteObjectAsExternalReference(const SEditorObject& editorObject, bool wasTopLevelObjectInSourceProject) {
	return (editorObject->getTypeDescription().isResource && !editorObject->as<user_types::RenderPass>()) || 
		editorObject->as<user_types::Prefab>() || 
		((editorObject->as<user_types::LuaScript>() || editorObject->as<user_types::LuaInterface>()) && wasTopLevelObjectInSourceProject);
}

bool Queries::isProjectSettings(const SEditorObject& obj) {
	return obj->isType<ProjectSettings>();
}

bool Queries::isResource(const SEditorObject& object) {
	return object->getTypeDescription().isResource && !isProjectSettings(object);
}

bool Queries::isNotResource(const SEditorObject& object) {
	return !object->getTypeDescription().isResource && !isProjectSettings(object);
}

bool Queries::isChildHandle(const ValueHandle& handle) {
	return handle.type() == PrimitiveType::Ref && handle.depth() == 2 && handle.parent().getPropName() == "children";
}

bool Queries::isChildObject(const SEditorObject& child, const SEditorObject& parent) {
	for (auto current = child->getParent(); current; current = current->getParent()) {
		if (current == parent) {
			return true;
		}
	}

	return false;
}

std::string Queries::getFullObjectHierarchyPath(SEditorObject obj) {
	std::vector<std::string> fullHierarchyPath;

	while (obj) {
		fullHierarchyPath.emplace_back(obj->objectName());
		obj = obj->getParent();
	}

	return fmt::format("{}", fmt::join(fullHierarchyPath.rbegin(), fullHierarchyPath.rend(), "/"));
}

bool Queries::isReadOnly(SEditorObject editorObj) {
	if (editorObj->query<ExternalReferenceAnnotation>()) {
		return true;
	}

	// Prefab instance subtree is read-only with one exception:
	// Lua interfaces which are direct children of the prefab instance serve as the interface for the
	// prefab instance: their lua input properties are modifyable. 
	if (PrefabOperations::findContainingPrefabInstance(editorObj->getParent()) && !PrefabOperations::isInterfaceObject(editorObj)) {
		return true;
	}

	return false;
}


bool Queries::isReadOnly(const Project& project, const ValueHandle& handle, bool linkState) {
	if (Queries::isReadOnly(handle.rootObject())) {
		return true;
	}

	// Prefab instance subtree is read-only with one exception:
	// Lua interfaces which are direct children of the prefab instance serve as the interface for the 
	// prefab instance: their lua input properties are modifyable. see exception below.
	// This exception doesn't apply to prefab instance which are nested inside other prefab instances
	if (PrefabOperations::findContainingPrefabInstance(handle.rootObject()->getParent()) && !PrefabOperations::isInterfaceProperty(handle)) {
		return true;
	}

	auto meshnode = handle.rootObject()->as<user_types::MeshNode>();
	if (meshnode) {
		for (size_t matIndex = 0; matIndex < meshnode->numMaterialSlots(); matIndex ++) {
			if ((meshnode->getMaterialOptionsHandle(matIndex).contains(handle) || meshnode->getUniformContainerHandle(matIndex).contains(handle)) &&
				!meshnode->materialPrivate(matIndex)) {
				return true;
			}
		}
	}

	if (handle.query<LinkStartAnnotation>() && !handle.query<LinkEndAnnotation>()) {
		return true;
	}
	if (!linkState) {
		if (currentLinkState(project, handle) != CurrentLinkState::NOT_LINKED) {
			return true;
		}
	}

	// Check parents read-only status
	auto parent = handle.parent();
	if (parent && parent.depth() > 0) {
		return isReadOnly(project, parent);
	}
	return false;
}


bool Queries::isHiddenInPropertyBrowser(const Project& project, const ValueHandle& handle) {
	if (handle.query<TagContainerAnnotation>() || handle.query<RenderableTagContainerAnnotation>()) {
		return false;
	}
	if (handle.query<HiddenProperty>()) {
		return true;
	}

	auto meshnode = handle.rootObject()->as<user_types::MeshNode>();
	if (meshnode) {
		for (size_t matIndex = 0; matIndex < meshnode->numMaterialSlots(); matIndex++) {
			if ((meshnode->getMaterialOptionsHandle(matIndex).contains(handle) || meshnode->getUniformContainerHandle(matIndex).contains(handle)) &&
				!meshnode->materialPrivate(matIndex)) {
				return true;
			}
		}
	}

	// Hide the tag property for the objects for which we currently do not use it
	if (handle.isRefToProp(&user_types::Node::tags_) && handle.rootObject()->as<user_types::BaseCamera>() != nullptr) {
		return true;
	}

	return false;
}


SLink Queries::getLink(const Project& project, const PropertyDescriptor& property) {
	const auto& links = project.linkEndPoints();

	auto it = links.find(property.object()->objectID());
	if (it != links.end()) {
		for (const auto& link : it->second) {
			if (link->compareEndPropertyNames(property.propertyNames())) {
				return link;
			}
		}
	}

	return nullptr;
}

Queries::CurrentLinkState Queries::currentLinkState(const Project& project, const ValueHandle& property) {
	if (auto link = Queries::getLink(project, property.getDescriptor())) {
		return (link->isValid()) ? CurrentLinkState::LINKED : CurrentLinkState::BROKEN;
	}

	auto current = property.parent();
	while (current && current.depth() > 0) {
		if (Queries::getLink(project, current.getDescriptor())) {
			return CurrentLinkState::PARENT_LINKED;
		}
		current = current.parent();
	}
	return CurrentLinkState::NOT_LINKED;
}


Queries::LinkState Queries::linkState(const Project& project, ValueHandle const& handle) {
	if (handle) {
		return {Queries::currentLinkState(project, handle), Queries::isReadOnly(project, handle, true), Queries::isValidLinkEnd(handle)};
	}
	return {CurrentLinkState::NOT_LINKED, true, false};
}


std::vector<SLink> Queries::getLinksConnectedToPropertySubtree(const Project& project, const ValueHandle& property, bool includeStarting, bool includeEnding) {
	std::vector<SLink> result;
	PropertyDescriptor desc{property.getDescriptor()};
	const auto& propertyRootObjID = property.rootObject()->objectID();

	if (includeStarting) {
		const auto& linkStartPoints = project.linkStartPoints();
		auto linkIt = linkStartPoints.find(propertyRootObjID);
		if (linkIt != linkStartPoints.end()) {
			for (const auto& link : linkIt->second) {
				auto startProp = link->startProp();
				if (desc == startProp || desc.contains(startProp)) {
					result.emplace_back(link);
				}
			}
		}
	}

	if (includeEnding) {
		const auto& linkEndPoints = project.linkEndPoints();
		auto linkIt = linkEndPoints.find(propertyRootObjID);
		if (linkIt != linkEndPoints.end()) {
			for (const auto& link : linkIt->second) {
				auto endProp = link->endProp();
				if (desc == endProp || desc.contains(endProp)) {
					result.emplace_back(link);
				}
			}
		}
	}

	return result;
}

std::vector<SLink> Queries::getLinksConnectedToProperty(const Project& project, const ValueHandle& property, bool includeStarting, bool includeEnding) {
	std::vector<SLink> result;
	PropertyDescriptor desc{property.getDescriptor()};
	const auto& propertyObjID = property.rootObject()->objectID();
	const auto& linkStartPoints = project.linkStartPoints();
	const auto& linkEndPoints = project.linkEndPoints();

	if (includeStarting) {
		auto linkIt = linkStartPoints.find(propertyObjID);
		if (linkIt != linkStartPoints.end()) {
			for (const auto& link : linkIt->second) {
				if (!link->isValid()) {
					continue;
				}

				auto startProp = link->startProp();
				if (startProp == desc) {
					result.emplace_back(link);
				}
			}
		}
	}

	if (includeEnding) {
		auto linkIt = linkEndPoints.find(propertyObjID);
		if (linkIt != linkEndPoints.end()) {
			for (const auto& link : linkIt->second) {
				if (!link->isValid()) {
					continue;
				}

				auto endProp = link->endProp();
				if (endProp == desc) {
					result.emplace_back(link);
				}
			}
		}
	}

	return result;
}

std::vector<SLink> Queries::getLinksConnectedToPropertyParents(const Project& project, const ValueHandle& property, bool includeSelf) {
	std::vector<SLink> result;
	PropertyDescriptor desc{property.getDescriptor()};
	const auto& propertyObjID = property.rootObject()->objectID();
	const auto& linkStartPoints = project.linkStartPoints();
	const auto& linkEndPoints = project.linkEndPoints();

	auto linkIt = linkStartPoints.find(propertyObjID);
	if (linkIt != linkStartPoints.end()) {
		for (const auto& link : linkIt->second) {
			auto startProp = link->startProp();
			if ((includeSelf && startProp == desc) || startProp.contains(desc)) {
				result.emplace_back(link);
			}
		}
	}

	linkIt = linkEndPoints.find(propertyObjID);
	if (linkIt != linkEndPoints.end()) {
		for (const auto& link : linkIt->second) {
			auto endProp = link->endProp();
			if ((includeSelf && endProp == desc) || endProp.contains(desc)) {
				result.emplace_back(link);
			}
		}
	}
	return result;
}

std::vector<SLink> Queries::getLinksConnectedToObject(const Project& project, const SEditorObject& object, bool includeStarting, bool includeEnding) {
	std::vector<SLink> result;
	const auto& propertyObjID = object->objectID();

	if (includeStarting) {
		const auto& linkStartPoints = project.linkStartPoints();
		auto linkIt = linkStartPoints.find(propertyObjID);
		if (linkIt != linkStartPoints.end()) {
			result.insert(result.end(), linkIt->second.begin(), linkIt->second.end());
		}
	}

	if (includeEnding) {
		const auto& linkEndPoints = project.linkEndPoints();
		auto linkIt = linkEndPoints.find(propertyObjID);
		if (linkIt != linkEndPoints.end()) {
			result.insert(result.end(), linkIt->second.begin(), linkIt->second.end());
		}
	}
	return result;
}


namespace {
void getLinksConnectedToObjectsHelper(
	const std::map<std::string, std::set<raco::core::SLink>>& linkStartPoints, 
	const std::map<std::string, std::set<raco::core::SLink>>& linkEndPoints,
	const std::string& propertyObjID, 
	bool includeStarting, 
	bool includeEnding, 
	std::map<std::string, std::set<raco::core::SLink>>& result) {

	if (includeStarting) {
		auto linkIt = linkStartPoints.find(propertyObjID);
		if (linkIt != linkStartPoints.end()) {
			for (const auto& link : linkIt->second) {
				result[(*link->endObject_)->objectID()].insert(link);
			}
		}
	}

	if (includeEnding) {
		auto linkIt = linkEndPoints.find(propertyObjID);
		if (linkIt != linkEndPoints.end()) {
			result[propertyObjID].insert(linkIt->second.begin(), linkIt->second.end());
		}
	}
}
}  // namespace

template <typename Container>
inline std::map<std::string, std::set<SLink>> Queries::getLinksConnectedToObjects(const Project& project, const Container& objects, bool includeStarting, bool includeEnding) {
	// use a set to avoid duplicate link entries
	std::map<std::string, std::set<SLink>> result;
	const auto& linkStartPoints = project.linkStartPoints();
	const auto& linkEndPoints = project.linkEndPoints();

	for (const auto& object : objects) {
		const auto& propertyObjID = object->objectID();
	
		getLinksConnectedToObjectsHelper(linkStartPoints, linkEndPoints, propertyObjID, includeStarting, includeEnding, result);
	}

	return result;
}

template<>
std::map<std::string, std::set<SLink>> Queries::getLinksConnectedToObjects(const Project& project, const std::map<std::string, SEditorObject>& objects, bool includeStarting, bool includeEnding) {
	// use a set to avoid duplicate link entries
	std::map<std::string, std::set<SLink>> result;
	const auto& linkStartPoints = project.linkStartPoints();
	const auto& linkEndPoints = project.linkEndPoints();

	for (const auto& [id, object] : objects) {
		const auto& propertyObjID = object->objectID();

		getLinksConnectedToObjectsHelper(linkStartPoints, linkEndPoints, propertyObjID, includeStarting, includeEnding, result);
	}

	return result;
}

template std::map<std::string, std::set<SLink>> Queries::getLinksConnectedToObjects<SEditorObjectSet>(const Project& project, const SEditorObjectSet& objects, bool includeStarting, bool includeEnding);

template std::map<std::string, std::set<SLink>> Queries::getLinksConnectedToObjects<std::unordered_set<SEditorObject>>(const Project& project, const std::unordered_set<SEditorObject>& objects, bool includeStarting, bool includeEnding);


std::string Queries::getBrokenLinksErrorMessage(const Project& project, SEditorObject obj) {
	std::vector<std::string> brokenLinks;
	for (auto link : Queries::getLinksConnectedToObject(project, obj, false, true)) {
		core::ValueHandle endHandle(link->endProp());
		if (endHandle && !link->isValid()) {
			brokenLinks.emplace_back(fmt::format("{} -> {}", link->startProp().getPropertyPath(), link->endProp().getPropertyPath()));
		}
	}
	if (brokenLinks.size() > 0) {
		std::sort(brokenLinks.begin(), brokenLinks.end());
		return fmt::format("{} Invalid Link(s):\n{}", brokenLinks.size(), fmt::join(brokenLinks, "\n"));
	}
	return {};
}

bool sameStructure(const ReflectionInterface* left, const ReflectionInterface* right) {
	if (left->size() != right->size()) {
		return false;
	}

	for (int i = 0; i < left->size(); i++) {
		if (left->name(i).empty()) {
			return false;
		}
	}
	for (int i = 0; i < right->size(); i++) {
		if (right->name(i).empty()) {
			return false;
		}
	}

	for (int i = 0; i < left->size(); i++) {
		auto name = left->name(i);
		const ValueBase* lval = left->get(name);
		const ValueBase* rval = right->get(name);
		if (!rval) {
			return false;
		}
		if (lval->type() != rval->type()) {
			return false;
		}
		if (lval->type() == PrimitiveType::Table && !sameStructure(&lval->asTable(), &rval->asTable())) {
			return false;
		}
	}
	return true;
}

bool checkLinkCompatibleTypes(const ValueHandle& start, const ValueHandle& end) {
	// Node rotations can be linked as euler or quaternion values
	if (end.rootObject()->as<user_types::Node>() && end.isRefToProp(&user_types::Node::rotation_) &&
		start.isVec4f() && end.isVec3f()) {
		return true;
	}

	auto startType = start.type();
	auto endType = end.type();
	if ((startType == PrimitiveType::Table || startType == PrimitiveType::Struct) &&
		(endType == PrimitiveType::Table || endType == PrimitiveType::Struct)) {
		return sameStructure(&start.constValueRef()->getSubstructure(), &end.constValueRef()->getSubstructure());
	}

	return startType == endType;
}

bool Queries::linkSatisfiesConstraints(const PropertyDescriptor& start, const PropertyDescriptor& end) {
	auto startPrefab = PrefabOperations::findContainingPrefab(start.object());
	auto endPrefab = PrefabOperations::findContainingPrefab(end.object());
	auto startPrefabInstance = PrefabOperations::findContainingPrefabInstance(start.object());
	auto endPrefabInstance = PrefabOperations::findContainingPrefabInstance(end.object());

	if (startPrefab && !endPrefab) {
		return false;
	}
	// This constraint ensures that we don't run into problems with prefabs which are external references:
	if (!startPrefab && endPrefab) {
		return false;
	}
	if (startPrefab && endPrefab && (startPrefab != endPrefab)) {
		return false;
	}

	if (startPrefabInstance && !endPrefabInstance) {
		return false;
	}
	// Ban inter-PrefabInstance links but allow exception for nested Prefabs:
	// We allow links starting inside a Prefab and ending inside a PrefabInstance contained in that Prefab.
	// We also need to allow the corresponding link in the corresponding PrefabInstance which means
	// that the start PrefabInstance is allowed to be one nesting level up from the end PrefabInstance.
	if (startPrefabInstance && endPrefabInstance && (startPrefabInstance != endPrefabInstance)) {
		auto endParent = endPrefabInstance->getParent();
		auto endParentPrefabInst = PrefabOperations::findContainingPrefabInstance(endParent);
		if (endParentPrefabInst != startPrefabInstance) {
			return false;
		}
	}

	// Ensure that links ending on materials or global LuaScripts work with external references:
	// LuaScripts which are children of Nodes outside of Prefabs are not allowed as external references
	// so we can't use them as link starting points if the link ends on a top-level LuaScript object or Material
	// both of which are allowed as external references.
	if (!end.object()->getParent() && start.object()->getParent()) {
		return false;
	}

    // Don't allow links ending on an interface script inside a Prefab. These links would not be propagated to the 
	// PrefabInstance anyway.
	if (PrefabOperations::isPrefabInterfaceProperty(end)) {
		return false;
	}

	return true;
}


std::set<ValueHandle> Queries::allowedLinkStartProperties(const Project& project, const ValueHandle& end) {
	PropertyDescriptor endDesc{end.getDescriptor()};
	std::set<ValueHandle> result;
	for (auto instance : project.instances()) {
		if (instance != end.rootObject()) {
			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
				PropertyDescriptor propDesc{prop.getDescriptor()};
				if (prop.query<LinkStartAnnotation>() && checkLinkCompatibleTypes(prop, end) && linkSatisfiesConstraints(propDesc,endDesc) && !project.createsLoop(propDesc, endDesc)) {
					result.insert(prop);
				}
			}
		}
	}
	return result;
}

std::set<std::pair<ValueHandle, bool>> Queries::allLinkStartProperties(const Project& project, const ValueHandle& end) {
	PropertyDescriptor endDesc{end.getDescriptor()};
	std::set<std::pair<ValueHandle, bool>> result;
	for (auto instance : project.instances()) {
		if (instance != end.rootObject()) {
			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
				PropertyDescriptor propDesc{prop.getDescriptor()};
				if (prop.query<LinkStartAnnotation>() && checkLinkCompatibleTypes(prop, end) && linkSatisfiesConstraints(propDesc, endDesc)) {
					result.insert({ prop, project.createsLoop(propDesc, endDesc) });
				}
			}
		}
	}
	return result;
}

bool Queries::isValidLinkEnd(const ValueHandle& endProperty) {
	return endProperty.isProperty() && endProperty.query<LinkEndAnnotation>();
}

bool Queries::isValidLinkStart(const ValueHandle& startProperty) {
	return startProperty.isProperty() && startProperty.query<LinkStartAnnotation>();
}

bool Queries::userCanCreateLink(const Project& project, const ValueHandle& start, const ValueHandle& end) {
	if (!(start && end && isValidLinkEnd(end) && isValidLinkStart(start) && checkLinkCompatibleTypes(start, end))) {
		return false;
	}
	if (Queries::isReadOnly(project, end, true)) {
		return false;
	}
	PropertyDescriptor startDesc{start.getDescriptor()};
	PropertyDescriptor endDesc{end.getDescriptor()};
	return linkSatisfiesConstraints(startDesc, endDesc) && !project.createsLoop(startDesc, endDesc);
}

bool Queries::userCanRemoveLink(const Project& project, const PropertyDescriptor& end) {
	if (Queries::isReadOnly(end.object())) {
		return false;
	}
	return true;
}

bool Queries::linkWouldBeAllowed(const Project& project, const PropertyDescriptor& start, const PropertyDescriptor& end) {
	return linkSatisfiesConstraints(start, end) && !project.createsLoop(start, end);
}

bool Queries::linkWouldBeValid(const Project& project, const PropertyDescriptor& start, const PropertyDescriptor& end) {
	ValueHandle startHandle(start);
	ValueHandle endHandle(end);
	return startHandle && endHandle && isValidLinkStart(startHandle) && isValidLinkEnd(endHandle) && checkLinkCompatibleTypes(startHandle, endHandle) && linkWouldBeAllowed(project, start, end);
}

std::vector<SEditorObject> Queries::filterForNotResource(const std::vector<SEditorObject>& objects) {
	std::vector<SEditorObject> result{};
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result), Queries::isNotResource);
	return result;
}

std::vector<SEditorObject> Queries::filterByTypeName(const std::vector<SEditorObject>& objects, const std::vector<std::string>& typeNames) {
	std::vector<SEditorObject> result{};
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result),
		[&typeNames](const SEditorObject& object) {
			return std::find(typeNames.begin(), typeNames.end(), object->getTypeDescription().typeName) != typeNames.end();
		});
	return result;
}

std::vector<SEditorObject> Queries::filterForTopLevelObjectsByTypeName(const std::vector<SEditorObject>& objects, const std::vector<std::string>& typeNames) {
	std::vector<SEditorObject> result{};
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result),
		[&typeNames](const SEditorObject& object) {
			return object->getParent() == nullptr && std::find(typeNames.begin(), typeNames.end(), object->getTypeDescription().typeName) != typeNames.end();
		});
	return result;
}

std::vector<SEditorObject> Queries::filterForVisibleTopLevelObjects(const std::vector<SEditorObject>& objects) {
	std::vector<SEditorObject> result;
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result), [](const auto& obj) { return obj->getParent() == nullptr && (Queries::isResource(obj) || Queries::isNotResource(obj)); });
	return result;
}


SEditorObjectSet Queries::collectAllChildren(std::vector<SEditorObject> baseObjects) {
	SEditorObjectSet children;
	for (auto obj : baseObjects) {
		std::copy(TreeIteratorAdaptor(obj).begin(), TreeIteratorAdaptor(obj).end(), std::inserter(children, children.end()));
	}
	return children;
}


std::vector<SEditorObject> Queries::filterForDeleteableObjects(Project const& project, const std::vector<SEditorObject>& objects) {

	// Since there is no case where a child is deleteable but its parent is not, we can just collect all children first before determining what is deleteable.
	auto objectsWithChildren = Queries::collectAllChildren(objects);
	std::vector<SEditorObject> partionedObjects{objectsWithChildren.begin(), objectsWithChildren.end()};

	auto begin = partionedObjects.begin();
	auto end = partionedObjects.end();

	auto partitionPoint = begin;
	auto previousPartitionPoint = end;

	// Iteratively partion the list, so that deleteable items are moved to the front and non deleteable items to the back.
	while (partitionPoint != previousPartitionPoint && partitionPoint != end) {

		previousPartitionPoint = partitionPoint;
		partitionPoint = std::partition(
			partitionPoint, end,
			[project, partitionPoint, begin, end](const SEditorObject& object) {

				if (object->isType<ProjectSettings>()) {
					return false;
				}

				// Block deletion if this object is a child of a prefab instance
				if (auto parent = object->getParent(); parent) {
					auto instance = PrefabOperations::findContainingPrefabInstance(parent);
					if (instance && std::find(begin, partitionPoint, instance) == partitionPoint) {
						return false;
					}
				}

				// Block deletion if this object is referenced by external reference objects that are not yet in the deleteable partion.
				for (const auto& instanceWeak : object->referencesToThis()) {
					const auto instance = instanceWeak.lock();
					if (instance->query<ExternalReferenceAnnotation>() && std::find(begin, partitionPoint, instance) == partitionPoint) {
						return false;
					}
				}

				// Block deletion if this object has output properties linked to by external reference objects that are not yet in the deleteable partion.
				auto links = getLinksConnectedToObject(project, object, true, false);
				for (const auto& link : links) {
					auto instance = link->endObject_.asRef();
					if (instance->query<ExternalReferenceAnnotation>() && std::find(begin, partitionPoint, instance) == partitionPoint) {
						return false;
					}
				}

				return true;
			});
	}

	std::vector<SEditorObject> deletableObjects{begin, partitionPoint};
	assert(deletableObjects.size() == Queries::collectAllChildren(deletableObjects).size());
	return deletableObjects;
}

std::vector<SEditorObject> Queries::filterForMoveableScenegraphChildren(Project const& project, const std::vector<SEditorObject>& objects, SEditorObject const& newParent) {	
	
	if (newParent && !canPasteIntoObject(project, newParent)) {
		return {};
	}

	std::vector<SEditorObject> result;
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result),
		[project, objects, newParent](const SEditorObject& object) {
			using namespace user_types;

			// This query is disallowed for objects not in the project.
			assert(project.getInstanceByID(object->objectID()) != nullptr);

			if (object->query<ExternalReferenceAnnotation>()) {
				return false;
			}

			// An object can't be moved below itself or below one of its children in the scenegraph hierarchy
			if (object == newParent || (newParent && isChildObject(newParent, object))) {
				return false;
			}

			// Prefab instance children can't be moved
			if (PrefabOperations::findContainingPrefabInstance(object->getParent())) {
				return false;
			}

			if (auto newParentPrefab = PrefabOperations::findContainingPrefab(newParent)) {
				// Prefab instance loop prevention
				if (wouldObjectInPrefabCauseLoop(object, newParentPrefab)) {
					return false;
				}

				// LuaInterfaces must be top-level if inside a Prefab.
				if (object->isType<LuaInterface>() && newParent != newParentPrefab) {
					return false;
				}
			}

			return (object->as<Node>() || object->as<LuaScript>() || object->as<LuaInterface>() || object->as<Animation>());
		});

	return result;
}

}  // namespace raco::core
