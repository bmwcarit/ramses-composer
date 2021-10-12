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

#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderPass.h"

#include <algorithm>
#include <cassert>

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

bool Queries::objectsReferencedByExtrefs(Project const& project, std::vector<SEditorObject> const& objects) {
	for (auto instance : project.instances()) {
		if (instance->query<ExternalReferenceAnnotation>() && 
			std::find(objects.begin(), objects.end(), instance) == objects.end()) {
			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
				if (prop.type() == PrimitiveType::Ref) {
					auto refValue = prop.asTypedRef<EditorObject>();
					if (refValue && (std::find(objects.begin(), objects.end(), refValue) != objects.end())) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

std::vector<ValueHandle> Queries::findAllReferencesFrom(std::set<SEditorObject> const& objects) {
	std::vector<ValueHandle> refs;
	for (auto instance : objects) {
		for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(instance))) {
			if (prop.type() == PrimitiveType::Ref) {
				auto refValue = prop.asTypedRef<EditorObject>();
				if (refValue && std::find(objects.begin(), objects.end(), refValue) == objects.end()) {
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
	std::set<SEditorObject> referenced;
	std::set<std::string> referencedRenderableTags;
	std::set<std::string> referencedMaterialTags;

	// This is deliberately naive right now: only objects which are not referenced (directly or via tag) by anything
	// else count as "unreferenced". Render passes cannot be referenced by anything and are therefore never "unreferenced".
	// A less naive alternative would be to use the render passes as seed objects, and then gather all objects which
	// are referenced by them, but given that this function is mainly used to delete "unused" resources it
	// seems to be better to return fewer rather than more objects.
	for (auto instance : project.instances()) {
		if (&instance->getTypeDescription() == &user_types::RenderPass::typeDescription) {
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

std::vector<SEditorObject> Queries::findAllValidReferenceTargets(Project const& project, const ValueHandle& handle) {
	std::vector<SEditorObject> valid;
	for (auto obj : project.instances()) {
		if (handle.constValueRef()->canSetRef(obj)) {
			valid.push_back(obj);
		}
	}

	// Special case: settings the "template" property of a PrefabInstance
	// -> filter out prefabs which would create a prefab instantiation loop
	auto inst = handle.rootObject()->as<user_types::PrefabInstance>();
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

bool Queries::canMoveScenegraphChild(Project const& project, SEditorObject const& object, SEditorObject const& newParent) {
	// This query is disallowed for objects not in the project.
	assert(project.getInstanceByID(object->objectID()) != nullptr);

	using namespace user_types;

	if (object->query<ExternalReferenceAnnotation>() || newParent && newParent->query<ExternalReferenceAnnotation>()) {
		return false;
	}

	// An object can't be moved below itself or below one of its children in the scenegraph hierarchy
	for (auto parent = newParent; parent; parent = parent->getParent()) {
		if (parent == object) {
			return false;
		}
	}

	// Prefab instance children can't be moved
	if (PrefabOperations::findContainingPrefabInstance(object->getParent())) {
		return false;
	}

	// Prefab instance subtree is locked: can't move anything into it
	if (PrefabOperations::findContainingPrefabInstance(newParent)) {
		return false;
	}

	// Prefab instance loop prevention
	if (auto newParentPrefab = PrefabOperations::findContainingPrefab(newParent)) {
		if (wouldObjectInPrefabCauseLoop(object, newParentPrefab)) {
			return false;
		}
	}

	return (object->as<Node>() || object->as<LuaScript>()) &&
		(newParent == nullptr || newParent->as<Node>() || newParent->as<Prefab>());
}


bool Queries::canDeleteObjects(Project const& project, const std::vector<SEditorObject>& objects) {
	for (const auto object : objects) {
		//  Objects nested inside PrefabInstances can't be deleted
		if (auto parent = object->getParent()) {
			if (parent && PrefabOperations::findContainingPrefabInstance(parent)) {
				return false;
			}
		}
	}

	if (objectsReferencedByExtrefs(project, objects)) {
		return false;
	}

	return true;
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

	if (object->as<user_types::LuaScript>()) {
		return false;
	}

	// Can't modify PrefabInstance contents
	if (PrefabOperations::findContainingPrefabInstance(object)) {
		return false;
	}

	return true;
}

bool Queries::canPasteObjectAsExternalReference(SEditorObject editorObject, bool isTopLevelObject) {
	return (editorObject->getTypeDescription().isResource  || editorObject->as<user_types::Prefab>()
		|| (editorObject->as<user_types::LuaScript>() && isTopLevelObject));
}

bool Queries::isProjectSettings(const SEditorObject& obj) {
	return obj->getTypeDescription().typeName == ProjectSettings::typeDescription.typeName;
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

bool Queries::isReadOnly(SEditorObject editorObj) {
	if (editorObj->query<ExternalReferenceAnnotation>()) {
		return true;
	}

	// Prefab instance subtree is read-only with one exception:
	// Lua scripts which are direct children of the prefab instance serve as the interface for the
	// prefab instance: their lua input properties are modifyable. see exception below.
	// This exception doesn't apply to prefab instance which are nested inside other prefab instances
	auto inst = PrefabOperations::findContainingPrefabInstance(editorObj);
	if (inst && inst != editorObj) {
		// Exception: LuaScript objects which are direct children of a PrefabInstance which is not nested inside another PrefabInstance
		// are not read-only.
		if (!(editorObj->as<user_types::LuaScript>() &&
				editorObj->getParent() == inst &&
				!PrefabOperations::findContainingPrefabInstance(inst->getParent()))) {
			return true;
		}
	}

	// PrefabInstances nested inside other PrefabInstances are also readonly
	if (editorObj->as<user_types::PrefabInstance>()) {
		auto parent = editorObj->getParent();
		if (parent && PrefabOperations::findContainingPrefabInstance(parent)) {
			return true;
		}
	}

	return false;
}


bool Queries::isReadOnly(const Project& project, const ValueHandle& handle, bool linkState) {
	if (Queries::isReadOnly(handle.rootObject())) {
		return true;
	}

	// Prefab instance subtree is read-only with one exception:
	// Lua scripts which are direct children of the prefab instance serve as the interface for the 
	// prefab instance: their lua input properties are modifyable. see exception below.
	// This exception doesn't apply to prefab instance which are nested inside other prefab instances
	auto inst = PrefabOperations::findContainingPrefabInstance(handle.rootObject());
	if (inst && inst != handle.rootObject()) {
		// All properties other than the lua inputs in a LuaScript inside a PrefabInstance are read-only.
		if (handle.rootObject()->as<user_types::LuaScript>()) {
			if (!(handle.depth() > 0 && handle.getPropertyNamesVector()[0] == "luaInputs")) {
				return true;
			}
		}
	}

	auto meshnode = handle.rootObject()->as<user_types::MeshNode>();
	if (meshnode) {
		for (size_t matIndex = 0; matIndex < meshnode->numMaterialSlots(); matIndex ++) {
			if ((meshnode->getMaterialOptionsHandle(matIndex).contains(handle) || meshnode->getUniformContainerHandle(matIndex).contains(handle)) &
				!meshnode->materialPrivate(matIndex)) {
				return true;
			}
		}
	}

	// Detect luascript output variables:
	if (handle.query<LinkStartAnnotation>()) {
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


bool Queries::isHidden(const Project& project, const ValueHandle& handle) {
	if (handle.query<HiddenProperty>()) {
		return true;
	}

	auto meshnode = handle.rootObject()->as<user_types::MeshNode>();
	if (meshnode) {
		for (size_t matIndex = 0; matIndex < meshnode->numMaterialSlots(); matIndex++) {
			if ((meshnode->getMaterialOptionsHandle(matIndex).contains(handle) || meshnode->getUniformContainerHandle(matIndex).contains(handle)) &
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

std::map<std::string, std::set<SLink>> Queries::getLinksConnectedToObjects(const Project& project, const std::set<SEditorObject>& objects, bool includeStarting, bool includeEnding) {
	// use a set to avoid duplicate link entries
	std::map<std::string, std::set<SLink>> result;
	const auto& linkStartPoints = project.linkStartPoints();
	const auto& linkEndPoints = project.linkEndPoints();

	for (const auto& object : objects) {
		const auto& propertyObjID = object->objectID();

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

	return result;
}

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
	if ((start.type() == PrimitiveType::Table || start.type() == PrimitiveType::Struct) && 
		(end.type() == PrimitiveType::Table || end.type() == PrimitiveType::Struct)) {
		return sameStructure(&start.constValueRef()->getSubstructure(), &end.constValueRef()->getSubstructure());
	}
	return start.type() == end.type();
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
	PropertyDescriptor startDesc{start.getDescriptor()};
	PropertyDescriptor endDesc{end.getDescriptor()};
	return linkSatisfiesConstraints(startDesc, endDesc) && !project.createsLoop(startDesc, endDesc);
}

bool Queries::linkWouldBeAllowed(const Project& project, const PropertyDescriptor& start, const PropertyDescriptor& end) {
	return linkSatisfiesConstraints(start, end) && !project.createsLoop(start, end);
}

bool Queries::linkWouldBeValid(const Project& project, const PropertyDescriptor& start, const PropertyDescriptor& end) {
	return isValidLinkStart(start) && isValidLinkEnd(end) && checkLinkCompatibleTypes(start, end) && linkWouldBeAllowed(project, start, end);
}

std::vector<SEditorObject> Queries::filterForVisibleObjects(const std::vector<SEditorObject>& objects) {
	std::vector<SEditorObject> result;
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(result), [](const auto& obj) { return Queries::isResource(obj) || Queries::isNotResource(obj); });
	return result;
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

}  // namespace raco::core
