/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Handles.h"
#include "Link.h"

#include <string>
#include <vector>

namespace raco::serialization {
struct DeserializationFactory;
}

namespace raco::core {
struct MeshDescriptor;
struct MeshScenegraph;

class Project;
class ExternalProjectsStoreInterface;
class MeshCache;
class UndoStack;
class UserObjectFactoryInterface;
class BaseContext;
class Errors;
class ValueHandle;
class EngineInterface;

class CommandInterface {
public:
	CommandInterface(BaseContext* context, UndoStack* undostack);

	Project* project();
	UserObjectFactoryInterface* objectFactory();
	MeshCache* meshCache();
	Errors& errors();
	EngineInterface& engineInterface();
	UndoStack& undoStack();

	// Basic property changes
	void set(ValueHandle const& handle, bool const& value);
	void set(ValueHandle const& handle, int const& value);
	void set(ValueHandle const& handle, double const& value);
	void set(ValueHandle const& handle, std::string const& value);
	void set(ValueHandle const& handle, std::vector<std::string> const& value);
	void set(ValueHandle const& handle, Table const& value);
	void set(ValueHandle const& handle, SEditorObject const& value);

	// Object creation/deletion
	SEditorObject createObject(std::string type, std::string name = std::string(), std::string id = std::string(), SEditorObject parent = nullptr);

	// Delete set of objects
	// Returns number of actually deleted objects which may be larger than the passed in vector
	// since dependent objects may need to be included.
	size_t deleteObjects(std::vector<SEditorObject> const& objects);

	// Move scenegraph node to new parent at before the specified index.
	// - If ValueHandle is invalid/empty the scenegraph parent is removed.
	// - If insertionBeforeIndex = -1 the node will be appended at the end of the new parent children.
	void moveScenegraphChild(SEditorObject const& object, SEditorObject const& newParent, int insertBeforeIndex = -1);

	// Calls Context::insertAssetScenegraph and generates a composite undo command.
	void insertAssetScenegraph(const raco::core::MeshScenegraph& scenegraph, const std::string& absPath, SEditorObject const& parent);

	/**
	 * Creates a serialized representation of all given [EditorObject]'s and their appropriate dependencies.
	 * Used in conjunction with #pasteObjects.
	 * @param deepCopy if true will copy ALL references, if false will copy only the necesscary ones (e.g. children)
	 * @return string containing the serialization of the passed [EditorObject]'s.
	 */
	std::string copyObjects(const std::vector<SEditorObject>& objects, bool deepCopy = false);

	/**
	 * Similar behaviour to #copyObjects, additonally will delete the given [EditorObject]'s.
	 * @return string containing the serialization of the passed [EditorObject]'s.
	 */
	std::string cutObjects(const std::vector<SEditorObject>& objects, bool deepCut = false);

	/**
	 * Paste the serialization created with #copyObjects or #cutObjects into the project associated
	 * with this context.
	 * @param val serializated string of [EditorObjects]'s created by either #copyObjects or #cutObjects.
	 * @param target target for the paste operation. Will move all appropiate top level object into the target.
	 * @return std::vector of all top level [EditorObject]'s which where created by the paste operation.
	 */
	std::vector<SEditorObject> pasteObjects(const std::string& val, SEditorObject const& target = {}, bool pasteAsExtref = false, bool* outSuccess = nullptr, std::string* outError = nullptr);

	// Link operations
	SLink addLink(const ValueHandle& start, const ValueHandle& end);
	void removeLink(const PropertyDescriptor& end);

	void deleteUnreferencedResources();

private:
	BaseContext* context_;
	UndoStack* undoStack_;
};

}