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

#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"
#include "data_storage/Table.h"
#include "core/BasicAnnotations.h"
#include "core/CoreAnnotations.h"
#include "core/FileChangeMonitor.h"

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <iterator>

namespace raco::core {

using namespace raco::data_storage;

class BaseContext;
class ValueHandle;
class Errors;

class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;
using WEditorObject = std::weak_ptr<EditorObject>;
using SCEditorObject = std::shared_ptr<const EditorObject>;


// This is the base class for complex objects useable as reference types in the Value class, 
// i.e. for PrimitiveType::Ref Values
//
// Types of data/member variables in EditorObjects
// - persistent data: Value<T> and Property<T,Annos...> member variables
//   These will be serialized.
//   Must not be declared mutable.
// - volatile data: all other members variables
//   These will not be serialized and need to be recreated after load/paste and similar operations.
//   May be declared mutable.
//
class EditorObject : public ClassWithReflectedMembers, public std::enable_shared_from_this<EditorObject> {
public:
	virtual bool serializationRequired() const override {
		return true;
	}
	EditorObject(EditorObject const& other) :
		ClassWithReflectedMembers(std::vector<std::pair<std::string, ValueBase*>>{}),
		objectID_(other.objectID_), objectName_(other.objectName_), children_(other.children_) {
		fillPropertyDescription();
	}

	EditorObject(std::string name = std::string(), std::string id = std::string());

	virtual ~EditorObject() = default;

	template<class C>
	std::shared_ptr<C> as() {
		return std::dynamic_pointer_cast<C>(shared_from_this());
	}

	// Check the dynamic type of an EditorObject against a statically known class.
	// This is more efficient than using EditorObject::as<C>() since it doesn't perform dynamic_cast.
	template<class C>
	bool isType() const {
		return &getTypeDescription() == &C::typeDescription;
	}

	std::string const& objectID() const;
	void setObjectID(std::string const& id);

	std::string const& objectName() const;
	void setObjectName(std::string const& name);

	
	struct ChildIterator {
		ChildIterator(SEditorObject const& object, size_t index);

		SEditorObject operator*();
		bool operator!=(ChildIterator const& other) const;
		ChildIterator& operator++();

	private:
		SEditorObject object_ = nullptr;
		size_t index_ = 0;
	};

	ChildIterator begin();
	ChildIterator end();

	// Find index of object in data object children of current object.
	// Returns -1 if not found among children.
	int findChildIndex(const EditorObject* object);
	
	// Get data model parent; root objects have nullptr as parent.
	SEditorObject getParent();


	//
	// Const handlers
	// - are declared 'const'
	// - purpose: initialize and maintain volatile data
	// - they are not allowed to change persistent data.
	//

	virtual void onAfterDeserialization() const;

	// Called when
	//   sourceReferenceProperty contains a reference to *this
	// before either
	// - sourceReferenceProperty is set different value(for primType_Ref Values)
	//   -> example: prefab instance template parent reference
	// - sourceReferenceProperty is removed from a Table
	//   -> example: scenegraph children table/array
	// - used to maintain backpointers to the referencing object
	virtual void onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const;

	// Called after either
	// - sourceReferenceProperty was set to *this (for primType_Ref Values)
	//   -> example: prefab instance template parent reference
	// - sourceReferenceProperty containing *this was added to a Table
	//   -> example: scenegraph children table/array
	// - used to maintain backpointers to the referencing object
	virtual void onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const;

	// Called before objects are deleted via the Context.
	// - Should not perform any data model modifications.
	// - Can release volatile resources associated to the object like file watchers.
	// - Needed because cleanup via destructor will run too late since UI will in general hold
	//   shared pointers for longer than the desired lifetime of the resources, i.e. file watchers.
	virtual void onBeforeDeleteObject(BaseContext& context) const;


	// 
	// Side-effect handlers for persistent data
	// - are not declared 'const' and take BaseContext parameter
	// - purpose: maintain consistency of the persistent data.
	// - are allowed to change persistent data.
	// - need to use contexts for modifying persistent data.
	//

	// Called
	// - after a context was created for the Project containg the EditorObject
	// - after undo/prefab update/external reference update has changed an EditorObject
	virtual void onAfterContextActivated(BaseContext& context);

	// Called after changing a value inside this object, possibly nested multiple levels.
	virtual void onAfterValueChanged(BaseContext& context, ValueHandle const& value);

	// Called after any property in the changedObject changed and a property inside the current
	// object contains a reference property to changedObject.
	// - example: MeshNode update after either the Mesh or Material have changed.
	virtual void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {}

	// Called
	// - after creating the object
	// - when external files have changed
	// 
	// must perform sync with external files
	// - this needs to invalidate any cached data related to external files, .e.g
	//   script or shader content
	virtual void updateFromExternalFile(BaseContext& context) {}


	// Data model children (not the same as scenegraph children):
	// - model ownership, i.e. lifetime of child depends on parent
	// - have unique parent, i.e. can appear only once in tree
	// - used to model both scenegraph children of nodes and prefab/prefab instance resource 
	//   contents (offscreen buffers etc)
	Property<Table, ArraySemanticAnnotation, HiddenProperty> children_{{}, {}, {}};

	// Returns the object ID without braces or hyphens in a pair of separated hexadecimal numbers {id[0,15], id[16-31]}
	std::pair<uint64_t, uint64_t> objectIDAsRamsesLogicID() const;

	static std::string normalizedObjectID(std::string const& id);

	// Object IDs are calculated in the following cases
	// - PrefabInstance children:
	//   PrefabInstance child ID = Xor(PrefabInstance, Prefab child ID
	// - Prefab/PrefabInstance LuaInterface objects generated during migration
	//   calculated from the corresponding LuaScript object by xoring with '1'
	//   LuaInterface ID = Xor(LuaScript ID, 0x01)
	static std::string XorObjectIDs(std::string const& id1, std::string const& id2);

	void fillPropertyDescription() {
		properties_.emplace_back("objectID", &objectID_);
		properties_.emplace_back("objectName", &objectName_);
		properties_.emplace_back("userTags", &userTags_);
		properties_.emplace_back("children", &children_);
	}


	Property<std::string, HiddenProperty> objectID_{ std::string(), HiddenProperty() };
	Property<std::string, DisplayNameAnnotation> objectName_;

	Property<Table, ArraySemanticAnnotation, HiddenProperty, UserTagContainerAnnotation, DisplayNameAnnotation> userTags_{{}, {}, {}, {}, {"User Tags"}};

	// Used to check back pointers in the unit tests.
	const std::set<WEditorObject, std::owner_less<WEditorObject>>& referencesToThis() const;

protected:
	// Create file watchers for paths and associate them with the specified property.
	void recreatePropertyFileWatchers(BaseContext& context, const std::string& propertyName, const std::set<std::string>& paths);

private:
	friend class BaseContext;

	mutable WEditorObject parent_;
	
	// volatile
	mutable std::set<WEditorObject, std::owner_less<WEditorObject>> referencesToThis_;
	
	mutable std::map<std::string, std::set<FileChangeMonitor::UniqueListener>> uriListeners_;
};

class CompareEditorObjectByID {
public:
	bool operator()(const SEditorObject& lhs, const SEditorObject& rhs) const {
		return lhs != rhs &&  (lhs == nullptr || (rhs != nullptr && lhs->objectID() < rhs->objectID()));
	} 
};

// Make order of objects in std::set<SEditorObject> predictable in debug mode.
// We need this to reliably test for bugs which are order-dependent.
// The solution is to use a custom comparison function for the set which uses
// the EditorObject::objectID(). Tests can then force a particular order by 
// creating objects with known IDs.
// Since this might have a performance impact due to the necessary string comparisons
// this is only enabled in debug mode.
#ifdef NDEBUG
using SEditorObjectSet = std::set<SEditorObject>;
#else
using SEditorObjectSet = std::set<SEditorObject, CompareEditorObjectByID>;
#endif

class TreeIterator {
public:
	TreeIterator() = default;
	TreeIterator(SEditorObject object);

	SEditorObject operator*();
	TreeIterator& operator++();
	bool operator!=(TreeIterator const& other);

private:
	operator bool();

	struct Item {
		Item& operator++() {
			++current;
			return *this;
		}
		operator bool() const {
			return current != end;
		}

		EditorObject::ChildIterator current;
		EditorObject::ChildIterator end;
	};

	SEditorObject top_;
	std::stack<Item> stack_;
};


class TreeIteratorAdaptor {
public:
	TreeIteratorAdaptor(SEditorObject root) : object_(root) {}

	TreeIterator begin() {
		return TreeIterator(object_);
	}
	TreeIterator end() {
		return TreeIterator();
	}

private:
	SEditorObject object_;
};

}

template<>
struct std::iterator_traits<raco::core::EditorObject::ChildIterator> {
	using value_type = raco::core::SEditorObject;
	using reference = raco::core::SEditorObject&;
	using iterator_category = std::forward_iterator_tag;
};

template<>
struct std::iterator_traits<raco::core::TreeIterator> {
	using value_type = raco::core::SEditorObject;
	using reference = raco::core::SEditorObject&;
	using iterator_category = std::forward_iterator_tag;
};
