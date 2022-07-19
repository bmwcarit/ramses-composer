/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/CommandInterface.h"

#include "core/Context.h"
#include "core/CoreFormatter.h"
#include "core/MeshCacheInterface.h"
#include "core/PathManager.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "core/Queries_Tags.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"
#include "utils/u8path.h"

#include "user_types/RenderLayer.h"

#include <spdlog/fmt/bundled/ranges.h>
#include <algorithm>

namespace raco::core {

CommandInterface::CommandInterface(BaseContext* context, UndoStack* undoStack) : context_(context), undoStack_(undoStack) {
}

Project* CommandInterface::project() {
	return context_->project();
}

UserObjectFactoryInterface* CommandInterface::objectFactory() {
	return context_->objectFactory();
}

MeshCache* CommandInterface::meshCache() {
	return context_->meshCache();
}

Errors& CommandInterface::errors() {
	return context_->errors();
}

EngineInterface& CommandInterface::engineInterface() {
	return context_->engineInterface();
}

UndoStack& CommandInterface::undoStack() {
	return *undoStack_;
}


bool CommandInterface::checkHandleForSet(ValueHandle const& handle) {
	if (!handle) {
		throw std::runtime_error(fmt::format("Invalid property handle"));
	}
	if (!project()->isInstance(handle.rootObject())) {
		throw std::runtime_error(fmt::format("Object '{}' not in project", handle.rootObject()->objectName()));
	}
	if (Queries::isReadOnly(*project(), handle, false)) {
		throw std::runtime_error(fmt::format("Property '{}' is read-only", handle.getPropertyPath()));
	}
	if (Queries::currentLinkState(*project(), handle) != Queries::CurrentLinkState::NOT_LINKED) {
		throw std::runtime_error(fmt::format("Property '{}' is linked", handle.getPropertyPath()));
	}

	return true;
}

bool CommandInterface::checkScalarHandleForSet(ValueHandle const& handle, PrimitiveType type) {
	if (checkHandleForSet(handle)) {
		if (handle.type() != type) {
			throw std::runtime_error(fmt::format("Property '{}' is a '{}' and not a bool", handle.getPropertyPath(), handle.type()));
		}
	}
	return true;
}


void CommandInterface::set(ValueHandle const& handle, bool const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Bool) && handle.asBool() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, int const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Int) && handle.asInt() != value) {
		if (auto anno = handle.query<raco::core::EnumerationAnnotation>()) {
			auto description = engineInterface().enumerationDescription(static_cast<raco::core::EngineEnumeration>(anno->type_.asInt()));
			if (description.find(value) == description.end()) {
				throw std::runtime_error(fmt::format("Value '{}' not in enumeration type", value));
			}
		}
		
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, int64_t const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Int64) && handle.asInt64() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, double const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Double) && handle.asDouble() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, std::string const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::String)) {
		auto newValue = handle.query<URIAnnotation>() ? raco::utils::u8path::sanitizePathString(value) : value;

		if (handle.asString() != newValue) {
			context_->set(handle, newValue);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), newValue),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, SEditorObject const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Ref) && handle.asTypedRef<EditorObject>() != value) {
		if (!Queries::isValidReferenceTarget(*context_->project(), handle, value)) {
			throw std::runtime_error(fmt::format("Property '{}' can't be set to '{}': invalid reference value", handle.getPropertyPath(), value->objectID()));
		}
		
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(),
			value ? value->objectName() : "<None>"));
	}
}


void CommandInterface::set(ValueHandle const& handle, std::array<double, 2> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec2f()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec2f", handle.getPropertyPath()));
		}
		if (handle.asVec2f() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {})", handle.getPropertyPath(), value[0], value[1]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, std::array<double, 3> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec3f()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec3f", handle.getPropertyPath()));
		}
		if (handle.asVec3f() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {}, {})", handle.getPropertyPath(), value[0], value[1], value[2]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, std::array<double, 4> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec4f()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec4f", handle.getPropertyPath()));
		}
		if (handle.asVec4f() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {}, {}, {})", handle.getPropertyPath(), value[0], value[1], value[2], value[3]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, std::array<int, 2> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec2i()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec2i", handle.getPropertyPath()));
		}
		if (handle.asVec2i() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {})", handle.getPropertyPath(), value[0], value[1]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, std::array<int, 3> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec3i()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec3i", handle.getPropertyPath()));
		}
		if (handle.asVec3i() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {}, {})", handle.getPropertyPath(), value[0], value[1], value[2]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, std::array<int, 4> const& value) {
	if (checkHandleForSet(handle)) {
		if (!handle.isVec4i()) {
			throw std::runtime_error(fmt::format("Property '{}' is not a Vec4i", handle.getPropertyPath()));
		}
		if (handle.asVec4i() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set property '{}' to ({}, {}, {}, {})", handle.getPropertyPath(), value[0], value[1], value[2], value[3]),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::setTags(ValueHandle const& handle, std::vector<std::string> const& value) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Table)) {
		if (!handle.constValueRef()->query<TagContainerAnnotation>()) {
			throw std::runtime_error(fmt::format("Property is not a TagContainer property '{}'", handle.getPropertyPath()));
		}

		std::set<std::string> forbiddenTags;
		Queries::findForbiddenTags(*project(), handle.rootObject(), forbiddenTags);
		for (const auto& tag : value) {
			if (forbiddenTags.find(tag) != forbiddenTags.end()) {
				throw std::runtime_error(fmt::format("Tag '{}' in object '{}' not allowed: would create renderable loop", tag, handle.rootObject()->objectName()));
			}
		}

		if (handle.constValueRef()->asTable().asVector<std::string>() != value) {
			context_->set(handle, value);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set tag set property '{}' to {}", handle.getPropertyPath(), value),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::setRenderableTags(ValueHandle const& handle, std::vector<std::pair<std::string, int>> const& renderableTags) {
	if (checkScalarHandleForSet(handle, PrimitiveType::Table)) {
		if (!handle.constValueRef()->query<RenderableTagContainerAnnotation>()) {
			throw std::runtime_error(fmt::format("Property is not a RenderableTagContainer property '{}'", handle.getPropertyPath()));
		}

		// We can end up here for non-RenderLayer objects if called from the tests.
		if (auto renderLayer = handle.rootObject()->as<user_types::RenderLayer>()) {
			std::set<std::string> forbiddenTags;
			Queries::findRenderLayerForbiddenRenderableTags(*project(), renderLayer, forbiddenTags);
			for (const auto& [name, index] : renderableTags) {
				if (forbiddenTags.find(name) != forbiddenTags.end()) {
					throw std::runtime_error(fmt::format("Tag '{}' in object '{}' not allowed: would create renderable loop", name, handle.rootObject()->objectName()));
				}
			}
		}

		data_storage::Table table;
		for (auto const& p : renderableTags) {
			table.addProperty(p.first, new Value<int>(p.second));
		}

		if (!(handle.constValueRef()->asTable() == table)) {
			context_->set(handle, table);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Set renderable tag property '{}'", handle.getPropertyPath()),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}


SEditorObject CommandInterface::createObject(std::string type, std::string name, SEditorObject parent) {
	if (context_->objectFactory()->isUserCreatable(type)) {
		if (parent && !project()->isInstance(parent)) {
			throw std::runtime_error(fmt::format("Create object: parent object '{}' not in project", parent->objectName()));
		}
		auto newObject = context_->createObject(type, name);
		if (parent) {
			context_->moveScenegraphChildren(Queries::filterForMoveableScenegraphChildren(*project(), {newObject}, parent), parent);
		}
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Create '{}' object '{}'", type, name));
		return newObject;
	} else {
		throw std::runtime_error(fmt::format("Can't create object of type '{}'", type));
	}
	return nullptr;
}

size_t CommandInterface::deleteObjects(std::vector<SEditorObject> const& objects) {
	for (auto obj : objects) {
		if (!project()->isInstance(obj)) {
			throw std::runtime_error(fmt::format("Trying to delete object '{}': not in project", obj->objectName()));
		}
	}

	auto deletableObjects = Queries::filterForDeleteableObjects(*project(), objects);
	if (!deletableObjects.empty()) {
		auto numDeleted = context_->deleteObjects(deletableObjects);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Delete {} objects", numDeleted));
		return numDeleted;
	}
	return 0;
}

size_t CommandInterface::moveScenegraphChildren(std::vector<SEditorObject> const& objects, SEditorObject const& newParent, int insertBeforeIndex) {
	for (auto obj : objects) {
		if (!project()->isInstance(obj)) {
			throw std::runtime_error(fmt::format("Scenegraph move: moved object '{}' not in project", obj->objectName()));
		}
	}
	if (newParent && !project()->isInstance(newParent)) {
		throw std::runtime_error(fmt::format("Scenegraph move: new parent object '{}' not in project", newParent->objectName()));
	}
	if (newParent) {
		if (insertBeforeIndex < -1 || insertBeforeIndex > static_cast<int>(newParent->children_->size())) {
			throw std::runtime_error(fmt::format("Scenegraph move: insertion index '{}' for new parent '{}' is out of range", insertBeforeIndex, newParent->objectName()));
		}
	} else {
		if (insertBeforeIndex != -1) {
			throw std::runtime_error(fmt::format("Scenegraph move: insertion index '{}' for new parent <root> is out of range", insertBeforeIndex));
		}
	}

	auto moveableChildren = Queries::filterForMoveableScenegraphChildren(*project(), objects, newParent);

	if (moveableChildren.size() > 0) {
		context_->moveScenegraphChildren(moveableChildren, newParent, insertBeforeIndex);
		PrefabOperations::globalPrefabUpdate(*context_);
		if (moveableChildren.size() == 1) {
			undoStack_->push(fmt::format("Move object '{}' to new parent '{}' before index {}", moveableChildren.front()->objectName(),
				newParent ? newParent->objectName() : "<root>",
				insertBeforeIndex));
		} else {
			undoStack_->push(fmt::format("Move {} objects to new parent '{}' before index {}", moveableChildren.size(),
				newParent ? newParent->objectName() : "<root>",
				insertBeforeIndex));
		}
	}
	return moveableChildren.size();
}

void CommandInterface::insertAssetScenegraph(const raco::core::MeshScenegraph& scenegraph, const std::string& absPath, SEditorObject const& parent) {
	// TODO error checking: scenegraph is not checked
	if (parent && !project()->isInstance(parent)) {
		throw std::runtime_error(fmt::format("insertAssetScenegraph: parent object '{}' not in project", parent->objectName()));
	}

	context_->insertAssetScenegraph(scenegraph, absPath, parent);
	PrefabOperations::globalPrefabUpdate(*context_);
	undoStack_->push(fmt::format("Inserted assets from {}", absPath));
	PathManager::setCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, raco::utils::u8path(absPath).parent_path().string());
}

std::string CommandInterface::copyObjects(const std::vector<SEditorObject>& objects, bool deepCopy) {
	for (auto obj : objects) {
		if (!project()->isInstance(obj)) {
			throw std::runtime_error(fmt::format("Object '{}' not in project", obj->objectName()));
		}
	}
	return context_->copyObjects(objects, deepCopy);
}

std::string CommandInterface::cutObjects(const std::vector<SEditorObject>& objects, bool deepCut) {
	for (auto obj : objects) {
		if (!project()->isInstance(obj)) {
			throw std::runtime_error(fmt::format("Cut objects: object '{}' not in project", obj->objectName()));
		}
	}

	auto deletableObjects = Queries::filterForDeleteableObjects(*project(), objects);
	if (!deletableObjects.empty()) {
		auto result = context_->cutObjects(deletableObjects, deepCut);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Cut {} objects with deep = {}", deletableObjects.size(), deepCut));
		return result;
	}
	return std::string();
}

std::vector<SEditorObject> CommandInterface::pasteObjects(const std::string& val, SEditorObject const& target, bool pasteAsExtref) {
	if (target && !project()->isInstance(target)) {
		throw std::runtime_error(fmt::format("Paste objects: target object '{}' not in project", target->objectName()));
	}

	std::vector<SEditorObject> result;
	if (Queries::canPasteIntoObject(*project(), target)) {
		try {
			result = context_->pasteObjects(val, target, pasteAsExtref);

			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Paste {} into '{}'",
				pasteAsExtref ? std::string("as external reference") : std::string(),
				target ? target->objectName() : "<root>"));
		}
		catch (ExtrefError& e) {
			// Force restoring project from last undo stack state.
			// Necessary to get consistent state when "paste as external reference" fails only during the external reference update.
			undoStack_->setIndex(undoStack_->getIndex(), true);
			throw std::runtime_error(fmt::format("Paste as external reference: external reference update failed with error '{}'", e.what()));
		}
	}
	else {
		throw std::runtime_error(fmt::format("Paste objects: invalid paste target '{}'", target->objectName()));
	}
	return result;
}

std::vector<SEditorObject> CommandInterface::duplicateObjects(const std::vector<SEditorObject>& objects) {
	if (objects.empty()) {
		return {};
	}

	for (auto obj : objects) {
		if (!project()->isInstance(obj)) {
			throw std::runtime_error(fmt::format("Duplicate objects: object '{}' not in project", obj->objectName()));
		}
	}
	
	if (!Queries::canDuplicateObjects(objects, *project())) {
		throw std::runtime_error(fmt::format("Duplicate objects: can't duplicate objects"));
	}

	std::vector<SEditorObject> duplicatedObjs;
	for (const auto& obj : objects) {
		auto target = obj->getParent();
		auto serializedObj = context_->copyObjects({obj}, false);
		auto duplicatedObj = context_->pasteObjects(serializedObj, target).front();
		duplicatedObjs.emplace_back(duplicatedObj);
	}

	if (!duplicatedObjs.empty()) {
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Duplicate {} object{}", duplicatedObjs.size(), duplicatedObjs.size() > 1 ? "s" : ""));
	}

	return duplicatedObjs;
}

SLink CommandInterface::addLink(const ValueHandle& start, const ValueHandle& end) {
	if (start && !context_->project()->isInstance(start.rootObject())) {
		throw std::runtime_error(fmt::format("Link starting object '{}' not in project", start.rootObject()->objectName()));
	}
	if (end && !context_->project()->isInstance(end.rootObject())) {
		throw std::runtime_error(fmt::format("Link end object '{}' not in project", end.rootObject()->objectName()));
	}

	if (Queries::userCanCreateLink(*context_->project(), start, end)) {
		auto link = context_->addLink(start, end);
		PrefabOperations::globalPrefabUpdate(*context_);
		undoStack_->push(fmt::format("Create link from '{}' to '{}'",
			start.getPropertyPath(), end.getPropertyPath()));
		return link;
	} else {
		throw std::runtime_error(fmt::format("Create link: link creation not possible"));
	}
	return nullptr;
}

void CommandInterface::removeLink(const PropertyDescriptor& end) {
	if (!context_->project()->isInstance(end.object())) {
		throw std::runtime_error(fmt::format("Link end object '{}' not in project", end.object()->objectName()));
	}

	if (auto link = Queries::getLink(*context_->project(), end)) {
		if (Queries::userCanRemoveLink(*context_->project(), end)) {
			context_->removeLink(end);
			PrefabOperations::globalPrefabUpdate(*context_);
			undoStack_->push(fmt::format("Remove link ending on '{}'", end.getPropertyPath()));
		} else {
			throw std::runtime_error(fmt::format("Remove link {} failed: end object is read-only.", link));
		}
	}
}

size_t CommandInterface::deleteUnreferencedResources() {
	auto toRemove = Queries::findAllUnreferencedObjects(*project(), Queries::isResource);
	if (!toRemove.empty()) {
		return deleteObjects(toRemove);
	}
	return 0;
}


}  // namespace raco::core
