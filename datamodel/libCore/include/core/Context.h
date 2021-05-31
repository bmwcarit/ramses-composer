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

#include <set>

#include "Handles.h"
#include "ChangeRecorder.h"
#include "Link.h"

namespace raco::serialization {
struct DeserializationFactory;
struct ObjectsDeserialization;
}  // namespace raco::serialization

namespace raco::core {
struct MeshDescriptor;

class Project;
class ExternalProjectsStoreInterface;
class FileChangeMonitor;
class MeshCache;
class UndoStack;
class Errors;
class UserObjectFactoryInterface;
class EngineInterface;

// Contexts
// - use context for every operation modifying the data model
// - keeps track of dirty/modified objects (both for gui/engine and internally)
// - ensures consistency of data model by invoking handlers etc

class BaseContext {
public:
	BaseContext(Project* project, EngineInterface* engineInterface, UserObjectFactoryInterface* objectFactory, DataChangeRecorder* changeRecorder, Errors* errors);

	Project* project();

	ExternalProjectsStoreInterface* externalProjectsStore();
	void setExternalProjectsStore(ExternalProjectsStoreInterface* store);

	MeshCache* meshCache();
	void setMeshCache(MeshCache* cache);
	
	FileChangeMonitor* fileChangeMonitor();
	void setFileChangeMonitor(FileChangeMonitor* monitor);


	MultiplexedDataChangeRecorder& changeMultiplexer();
	DataChangeRecorder& modelChanges();
	DataChangeRecorder& uiChanges();
	Errors& errors();

	UserObjectFactoryInterface* objectFactory();
	EngineInterface& engineInterface();
	
	// Basic property changes
	void set(ValueHandle const& handle, bool const& value);
	void set(ValueHandle const& handle, int const& value);
	void set(ValueHandle const& handle, double const& value);
	void set(ValueHandle const& handle, std::string const& value);
	void set(ValueHandle const& handle, std::vector<std::string> const& value);
	void set(ValueHandle const& handle, SEditorObject const& value);

	template <typename AnnoType, typename T>
	void set(AnnotationValueHandle<AnnoType> const& handle, T const& value);

	// Add property to Table
	ValueBase* addProperty(const ValueHandle& handle, std::string name, std::unique_ptr<ValueBase>&& newProperty);

	// Remove property from Table
	void removeProperty(const ValueHandle& handle, size_t index);
	void removeProperty(const ValueHandle& handle, const std::string& name);
	
	// Remove all properties from Table
	void removeAllProperties(const ValueHandle &handle);

	// Object creation/deletion
	SEditorObject createObject(std::string type, std::string name = std::string(), std::string id = std::string());

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
	 * @exception ExtrefError
	 */
	std::vector<SEditorObject> pasteObjects(const std::string& val, SEditorObject const& target = {}, bool pasteAsExtref = false);

	// Delete set of objects
	// Returns number of actually deleted objects which may be larger than the passed in vector
	// since dependent objects may need to be included.
	size_t deleteObjects(std::vector<SEditorObject> const& objects, bool gcExternalProjectMap = true);


	// Move scenegraph node to new parent at before the specified index.
	// - If ValueHandle is invalid/empty the scenegraph parent is removed.
	// - If insertionBeforeIndex = -1 the node will be appended at the end of the new parent children.
	void moveScenegraphChild(SEditorObject const& object, SEditorObject const& newParent, int insertBeforeIndex = -1);

	// Import scenegraph from a mesh file and move that scenegraph root noder under parent.
	// This includes generating Mesh resources, Nodes and MeshNodes as well as searching for already created Materials.
	// If parent is invalid, the mesh scenegraph root node will be the project's scenegraph root node.
	// Returns true when the import has succeeded, false otherwise.
	bool importAssetScenegraph(const std::string& absPath, SEditorObject const& parent);

	// Link operations
	SLink addLink(const ValueHandle& start, const ValueHandle& end);
	void removeLink(const PropertyDescriptor& end);

	void performExternalFileReload(const std::vector<SEditorObject>& objects);

	// @exception ExtrefError
	void updateExternalReferences(std::vector<std::string>& pathStack);

private:
	friend class UndoStack;
	friend class FileChangeCallback;
	friend class PrefabOperations;
	friend class ExtrefOperations;

	void rerootRelativePaths(const SEditorObject& editorObject, const std::string& originFolder);
	bool extrefPasteDiscardObject(SEditorObject editorObject, raco::serialization::ObjectsDeserialization& deserialization);
	void adjustExtrefAnnotationsForPaste(std::vector<SEditorObject>& newObjects, raco::serialization::ObjectsDeserialization& deserialization, bool pasteAsExtref);
	
	static void restoreReferences(const Project& project, std::vector<SEditorObject>& newObjects, raco::serialization::ObjectsDeserialization& deserialization);


	// Should only be used from the Undo system
	static bool deleteWithVolatileSideEffects(Project* project, const std::set<SEditorObject>& objects, Errors& errors, bool gcExternalProjectMap = true);

	void callReferencedObjectChangedHandlers(SEditorObject const& changedObject);
	void removeReferencesTo(std::set<SEditorObject> const& objects);

	ValueTreeIterator erase(const ValueTreeIterator& it);

	void updateLinkValidity(SLink link);
	
	template <typename T>
	void setT(ValueHandle const& handle, T const& value);

	Project* project_ = nullptr;
	EngineInterface* engineInterface_ = nullptr;
	ExternalProjectsStoreInterface* externalProjectsStore_ = nullptr;

	MeshCache* meshCache_ = nullptr;
	FileChangeMonitor* fileChangeMonitor_ = nullptr;
	UserObjectFactoryInterface* objectFactory_ = nullptr;
	Errors* errors_;
	DataChangeRecorder* uiChanges_ = nullptr;

	MultiplexedDataChangeRecorder changeMultiplexer_;
	DataChangeRecorder modelChanges_;
};

}

