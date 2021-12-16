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
#include "core/MeshCacheInterface.h"
#include "core/PathManager.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"

#include <spdlog/fmt/bundled/ranges.h>

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


void CommandInterface::set(ValueHandle const& handle, bool const& value) {
	if (handle && handle.asBool() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, int const& value) {
	if (handle && handle.asInt() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, double const& value) {
	if (handle && handle.asDouble() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, std::string const& value) {
	if (handle) {
		auto newValue = handle.query<URIAnnotation>() ? PathManager::sanitizePath(value) : value;

		if (handle.asString() != newValue) {
			context_->set(handle, newValue);
			PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
			undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), newValue),
				fmt::format("{}", handle.getPropertyPath(true)));
		}
	}
}

void CommandInterface::set(ValueHandle const& handle, SEditorObject const& value) {
	if (handle && handle.asTypedRef<EditorObject>() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(),
			value ? value->objectName() : "<None>"));
	}
}

void CommandInterface::set(ValueHandle const& handle, std::vector<std::string> const& value) {
	if (handle && handle.constValueRef()->asTable().asVector<std::string>() != value) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}' to {}", handle.getPropertyPath(), value),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

void CommandInterface::set(ValueHandle const& handle, Table const& value) {
	if (handle && !(handle.constValueRef()->asTable() == value)) {
		context_->set(handle, value);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Set property '{}'", handle.getPropertyPath()),
			fmt::format("{}", handle.getPropertyPath(true)));
	}
}

SEditorObject CommandInterface::createObject(std::string type, std::string name, std::string id, SEditorObject parent) {
	auto types = context_->objectFactory()->getTypes();
	if (types.find(type) != types.end()) {
		auto newObject = context_->createObject(type, name, id);
		if (parent) {
			context_->moveScenegraphChildren(Queries::filterForMoveableScenegraphChildren(*project(), {newObject}, parent), parent);
		}
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Create '{}' object '{}'", type, name));
		return newObject;
	}
	return nullptr;
}

size_t CommandInterface::deleteObjects(std::vector<SEditorObject> const& objects) {
	auto deletableObjects = Queries::filterForDeleteableObjects(*project(), objects);
	if (!deletableObjects.empty()) {
		auto numDeleted = context_->deleteObjects(deletableObjects);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Delete {} objects", numDeleted));
		return numDeleted;
	}
	return 0;
}

void CommandInterface::moveScenegraphChildren(std::vector<SEditorObject> const& objects, SEditorObject const& newParent, int insertBeforeIndex) {
	auto moveableChildren = Queries::filterForMoveableScenegraphChildren(*project(), objects, newParent);

	if (moveableChildren.size() > 0) {
		context_->moveScenegraphChildren(moveableChildren, newParent, insertBeforeIndex);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
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
}

void CommandInterface::insertAssetScenegraph(const raco::core::MeshScenegraph& scenegraph, const std::string& absPath, SEditorObject const& parent) {
	context_->insertAssetScenegraph(scenegraph, absPath, parent);
	PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
	undoStack_->push(fmt::format("Inserted assets from {}", absPath));
	PathManager::setCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, std::filesystem::path(absPath).parent_path().generic_string());
}

std::string CommandInterface::copyObjects(const std::vector<SEditorObject>& objects, bool deepCopy) {
	return context_->copyObjects(objects, deepCopy);
}

std::string CommandInterface::cutObjects(const std::vector<SEditorObject>& objects, bool deepCut) {
	auto deletableObjects = Queries::filterForDeleteableObjects(*project(), objects);
	if (!deletableObjects.empty()) {
		auto result = context_->cutObjects(deletableObjects, deepCut);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Cut {} objects with deep = {}", deletableObjects.size(), deepCut));
		return result;
	}
	return std::string();
}

std::vector<SEditorObject> CommandInterface::pasteObjects(const std::string& val, SEditorObject const& target, bool pasteAsExtref, bool* outSuccess, std::string* outError) {
	bool success = true;
	std::vector<SEditorObject> result;
	if (Queries::canPasteIntoObject(*project(), target)) {
		try {
			result = context_->pasteObjects(val, target, pasteAsExtref);

			PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
			undoStack_->push(fmt::format("Paste {} into '{}'",
				pasteAsExtref ? std::string("as external reference") : std::string(),
				target ? target->objectName() : "<root>"));
		} catch (ExtrefError& e) {
			success = false;
			if (outError) {
				*outError = e.what();
			}
			// Force restoring project from last undo stack state.
			// Necessary to get consistent state when "paste as external reference" fails only during the external reference update.
			undoStack_->setIndex(undoStack_->getIndex(), true);
		}
	}
	if (outSuccess) {
		*outSuccess = success;
	}
	return result;
}

SLink CommandInterface::addLink(const ValueHandle& start, const ValueHandle& end) {
	if (Queries::userCanCreateLink(*context_->project(), start, end)) {
		auto link = context_->addLink(start, end);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Create link from '{}' to '{}'",
			start.getPropertyPath(), end.getPropertyPath()));
		return link;
	}
	return nullptr;
}

void CommandInterface::removeLink(const PropertyDescriptor& end) {
	if (ValueHandle(end)) {
		context_->removeLink(end);
		PrefabOperations::globalPrefabUpdate(*context_, context_->modelChanges());
		undoStack_->push(fmt::format("Remove link ending on '{}'", end.getPropertyPath()));
	}
}

void CommandInterface::deleteUnreferencedResources() {
	auto toRemove = Queries::findAllUnreferencedObjects(*project(), Queries::isResource);
	if (!toRemove.empty()) {
		deleteObjects(toRemove);
	}
}


}  // namespace raco::core
