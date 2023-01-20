/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "EditorObject.h"
#include "Handles.h"
#include "Link.h"

#include <string>
#include <vector>

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

/**
 * @brief The CommandInterface is the user-level API for modifying the data model in a safe way.
 * 
 * main characteristics
 * - all side-effects including Prefab update and undo stack push are taken care of.
 * - the consistency of the data model is ensured internally
 * - all operations are checked and an exception is thrown if an invalid operation is attempted.
 * - changes are recorded to allow notifying the UI and engine update code
 * - no UI/engine updates are triggered directly
 * - the CommandInterface is used directly by the UI and the Python API to modify the data model.
 * - the CommandInterface uses the BaseContext internally to perform the operations and adds
 *   - Prefab update
 *   - undo stack push
 *   - checking of operations for validity
*/
class CommandInterface {
public:
	CommandInterface(BaseContext* context, UndoStack* undostack);

	Project* project() const;
	UserObjectFactoryInterface* objectFactory();
	MeshCache* meshCache();
	Errors& errors();
	EngineInterface& engineInterface() const;
	UndoStack& undoStack();

	bool canSet(ValueHandle const& handle, int const& value) const;

	// Basic property changes
	void set(ValueHandle const& handle, bool const& value);
	void set(ValueHandle const& handle, int const& value);
	void set(ValueHandle const& handle, int64_t const& value);
	void set(ValueHandle const& handle, double const& value);
	void set(ValueHandle const& handle, std::string const& value);
	void set(ValueHandle const& handle, SEditorObject const& value);

	void set(ValueHandle const& handle, std::array<double, 2> const& value);
	void set(ValueHandle const& handle, std::array<double, 3> const& value);
	void set(ValueHandle const& handle, std::array<double, 4> const& value);
	void set(ValueHandle const& handle, std::array<int, 2> const& value);
	void set(ValueHandle const& handle, std::array<int, 3> const& value);
	void set(ValueHandle const& handle, std::array<int, 4> const& value);

	
	bool canSetTags(ValueHandle const& handle, std::vector<std::string> const& value) const;

	// Set a tag set property
	// The handle must be a Table property with a TagContainer annotation.
	// All existing tags will be completed replaced by the new tag set.
	void setTags(ValueHandle const& handle, std::vector<std::string> const& tags);

	bool canSetRenderableTags(ValueHandle const& handle, std::vector<std::pair<std::string, int>> const& renderableTags) const;

	// Set a renderable tag set property
	// @param handle must be a Table property with a RenderableTagContainer annotation.
	// @param renderableTags tag name -> order index map
	void setRenderableTags(ValueHandle const& handle, std::vector<std::pair<std::string, int>> const& renderableTags);

	// Object creation/deletion
	SEditorObject createObject(std::string type, std::string name = std::string(), SEditorObject parent = nullptr);

	// Delete set of objects
	// Returns number of actually deleted objects which may be larger than the passed in vector
	// since dependent objects may need to be included.
	size_t deleteObjects(std::vector<SEditorObject> const& objects);

	// Move scenegraph nodes to new parent at a position before the specified index.
	// - If ValueHandle is invalid/empty the scenegraph parent is removed.
	// - If insertionBeforeIndex = -1 the node will be appended at the end of the new parent children.
	// - Only objects that are allowed to be moved to newParent will be actually moved there. 
	//   Attempting to move objects not allowed to be moved is not considered an error but just leads to 
	//   the remaining objects being moved.
	// @return Number of actually moved children.
	size_t moveScenegraphChildren(std::vector<SEditorObject> const& objects, SEditorObject const& newParent, int insertBeforeIndex = -1);

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
	 * @param val serialized string of [EditorObjects]'s created by either #copyObjects or #cutObjects.
	 * @param target target for the paste operation. Will move all appropriate top level objects into the target.
	 * @return std::vector of all top level [EditorObject]'s which where created by the paste operation.
	 */
	std::vector<SEditorObject> pasteObjects(const std::string& val, SEditorObject const& target = {}, bool pasteAsExtref = false);

	/**
	 * Creates a duplicate of the #objects in the scene.
	 * The duplicated objects will essentially copy & paste the #objects on the same scenegraph hierarchy level.
	 * @param objects vector of objects to be duplicated.
	 * @return std::vector of all successfully duplicated objects
	 */
	std::vector<SEditorObject> duplicateObjects(const std::vector<SEditorObject>& objects);

	// Link operations
	SLink addLink(const ValueHandle& start, const ValueHandle& end, bool isWeak = false);
	void removeLink(const PropertyDescriptor& end);

	// @return number of actually deleted objects.
	size_t deleteUnreferencedResources();

private:
	bool canSetHandle(ValueHandle const& handle) const;
	bool canSetHandle(ValueHandle const& handle, PrimitiveType type) const;

	bool checkHandleForSet(ValueHandle const& handle);
	bool checkScalarHandleForSet(ValueHandle const& handle, PrimitiveType type);

	BaseContext* context_;
	UndoStack* undoStack_;
};

}  // namespace raco::core