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

#include <set>

#include "ChangeRecorder.h"
#include "ExtrefOperations.h"
#include "Handles.h"
#include "Link.h"

namespace raco::serialization {
struct ObjectsDeserialization;
}  // namespace raco::serialization

namespace raco::components {
class MeshCacheImpl;
}

namespace raco::core {
struct MeshDescriptor;
struct MeshScenegraph;

class Project;
class ExternalProjectsStoreInterface;
class MeshCache;
class UndoStack;
class Errors;
class UserObjectFactoryInterface;
class EngineInterface;

/**
 * @brief The BaseContext is used internally to perform data model changes while ensuring low-level consistency.
 * 
 * In detail
 * - takes care of low-level side effects: these are the ones implemented by the EditorObject handler methods.
 * - changes are recorded for undo stack operations and UI/engine updates
 * - does not perform prefab update or undo stack push
 * - does not trigger UI/engine updates directly
*/
class BaseContext {
public:
	BaseContext(Project* project, EngineInterface* engineInterface, UserObjectFactoryInterface* objectFactory, DataChangeRecorder* changeRecorder, Errors* errors);

	Project* project();

	ExternalProjectsStoreInterface* externalProjectsStore();
	void setExternalProjectsStore(ExternalProjectsStoreInterface* store);

	MeshCache* meshCache();
	void setMeshCache(MeshCache* cache);
	
	MultiplexedDataChangeRecorder& changeMultiplexer();
	DataChangeRecorder& modelChanges();
	DataChangeRecorder& uiChanges();
	Errors& errors();

	UserObjectFactoryInterface* objectFactory();
	EngineInterface& engineInterface();

	void setUriValidationCaseSensitive(bool value);
	bool isUriValidationCaseSensitive() const;
	
	// Basic property changes
	void set(ValueHandle const& handle, bool const& value);
	void set(ValueHandle const& handle, int const& value);
	void set(ValueHandle const& handle, int64_t const& value);
	void set(ValueHandle const& handle, double const& value);
	void set(ValueHandle const& handle, std::string const& value);
	void set(ValueHandle const& handle, char const* value); // avoid cast of char const* to bool
	void set(ValueHandle const& handle, std::vector<std::string> const& value);
	void set(ValueHandle const& handle, SEditorObject const& value);
	void set(ValueHandle const& handle, Table const& value);
	void set(ValueHandle const& handle, std::array<double, 2> const& value);
	void set(ValueHandle const& handle, std::array<double, 3> const& value);
	void set(ValueHandle const& handle, std::array<double, 4> const& value);
	void set(ValueHandle const& handle, std::array<int, 2> const& value);
	void set(ValueHandle const& handle, std::array<int, 3> const& value);
	void set(ValueHandle const& handle, std::array<int, 4> const& value);
	// Set struct property to struct value.
	// Identical types are dynamically enforced at runtime.
	void set(ValueHandle const& handle, StructBase const& value);

	// Set Array property to array value.
	// Type safety of the value array is dynamically enforced at runtime.
	void set(ValueHandle const& handle, ArrayBase const& value);

	template <typename AnnoType, typename T>
	void set(AnnotationValueHandle<AnnoType> const& handle, T const& value);

	// Add property to Table or Array property
	// For an Array property the name is ignored and the type of the newProperty must be the element type of the array.
	// For a Table property the name may be empty only if the property carries a ArraySemanticAnnotation.
	ValueBase* addProperty(const ValueHandle& handle, std::string name, std::unique_ptr<ValueBase>&& newProperty, int indexBefore = -1);

	// Remove property from a Table or Array property
	void removeProperty(const ValueHandle& handle, size_t index);
	void removeProperty(const ValueHandle& handle, const std::string& name);
	
	// Remove all properties from a Table or Array property.
	void removeAllProperties(const ValueHandle &handle);

	void swapProperties(const ValueHandle& handle, size_t index_1, size_t index_2);

	/**
	 * @brief Resize Array property and initialize new entries to empty references.
	 * 
	 * @param handle Handle for a property of PrimitiveType::Array
	 * @param newSize New size for array.
	*/
	void resizeArray(const ValueHandle& handle, size_t newSize);

	
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
	// @param gcExternalProjectMap If true the external project map in the Project will be updated and external projects
	// 	   which are now unused are removed.
	// @param includeChildren Do not delete data model children object of the deleted object if set to false. If 
	// 	   deletion of the children is disabled using this flag the caller _must_ make sure that the data model stays
	// 	   consistent. This is only used in the Prefab update code and should normally never be used elsewhere.
	// @return number of actually deleted objects which may be larger than the passed in vector since dependent 
	//   objects may need to be included.
	size_t deleteObjects(std::vector<SEditorObject> const& objects, bool gcExternalProjectMap = true, bool includeChildren = true);


	// Move scenegraph nodes to new parent at before the specified index.
	// - If ValueHandle is invalid/empty the scenegraph parent is removed.
	// - If insertionBeforeIndex = -1 the node will be appended at the end of the new parent children.
	void moveScenegraphChildren(std::vector<SEditorObject> const& objects, SEditorObject const& newParent, int insertBeforeIndex = -1);

	// Import scenegraph as a hierarchy of EditorObjects and move that scenegraph root noder under parent.
	// This includes generating Mesh resources, Nodes and MeshNodes as well as searching for already created Materials.
	// If parent is invalid, the mesh scenegraph root node will be the project's scenegraph root node.
	void insertAssetScenegraph(const core::MeshScenegraph& scenegraph, const std::string& absPath, SEditorObject const& parent);

	// Link operations
	SLink addLink(const ValueHandle& start, const ValueHandle& end, bool isWeak = false);
	void removeLink(const PropertyDescriptor& end);

	void performExternalFileReload(const std::vector<SEditorObject>& objects);

	// @exception ExtrefError
	void updateExternalReferences(LoadContext& loadContext, int fileVersion = -1);

	void initBrokenLinkErrors();

	//Update link errors for links ending on 'object'
	void updateBrokenLinkErrors(SEditorObject endObject);
	
	// Update link errors for all links either starting or ending on 'object'
	void updateBrokenLinkErrorsAttachedTo(SEditorObject object);

	// Find and return the objects without parents within the serialized object set.
	// Note: the objects may still have parents in the origin project they were copied from.
	static std::vector<SEditorObject> getTopLevelObjectsFromDeserializedObjects(serialization::ObjectsDeserialization& deserialization, Project* project);

	// Initialize link validity flag during load. This does not update broken link errors and does not
	// generated change recorder entries. Use only during load to fix corrupt files.
	void initLinkValidity();


	static void generateNewObjectIDs(std::vector<core::SEditorObject>& newObjects);

private:
	friend class UndoStack;
	friend class UndoHelpers;
	friend class components::MeshCacheImpl;
	friend class PrefabOperations;
	friend class ExtrefOperations;

	void rerootRelativePaths(std::vector<SEditorObject>& newObjects, serialization::ObjectsDeserialization& deserialization);
	bool extrefPasteDiscardObject(SEditorObject editorObject, serialization::ObjectsDeserialization& deserialization);
	void adjustExtrefAnnotationsForPaste(std::vector<SEditorObject>& newObjects, serialization::ObjectsDeserialization& deserialization, bool pasteAsExtref);
	
	static void restoreReferences(const Project& project, std::vector<SEditorObject>& newObjects, serialization::ObjectsDeserialization& deserialization);

	// Should only be used from the Undo system
	bool deleteWithVolatileSideEffects(Project* project, const SEditorObjectSet& objects, Errors& errors, bool gcExternalProjectMap = true);

	void callReferencedObjectChangedHandlers(SEditorObject const& changedObject);

	void removeReferencesTo_If(SEditorObjectSet const& objects, std::function<bool(const ValueHandle& handle, SEditorObject object)> pred);
	void removeReferencesTo(SEditorObjectSet const& objects);
	void removeReferencesFrom_If(SEditorObjectSet const& objects, std::function<bool(const ValueHandle& handle, SEditorObject object)> pred);

	template <void (EditorObject::*Handler)(ValueHandle const&) const>
	static void callReferenceToThisHandler(ValueHandle const& vh);
	
	ValueTreeIterator erase(const ValueTreeIterator& it);

	void updateLinkValidity(SLink link);

	template <typename T>
	void setT(ValueHandle const& handle, T const& value);

	Project* project_ = nullptr;
	EngineInterface* engineInterface_ = nullptr;
	ExternalProjectsStoreInterface* externalProjectsStore_ = nullptr;

	MeshCache* meshCache_ = nullptr;
	UserObjectFactoryInterface* objectFactory_ = nullptr;
	Errors* errors_;
	DataChangeRecorder* uiChanges_ = nullptr;

	MultiplexedDataChangeRecorder changeMultiplexer_;
	DataChangeRecorder modelChanges_;

	bool isUriValidationCaseSensitive_ = false;
};

}

