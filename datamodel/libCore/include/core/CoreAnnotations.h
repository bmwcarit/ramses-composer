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

#include "data_storage/AnnotationBase.h"
#include "data_storage/Value.h"

#include "core/EngineInterface.h"
#include "core/PathManager.h"

#include <map>


namespace raco::core {

class URIAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "URIAnnotation", false };

	// Filter for relative directory URIs that stay the same relative to the project (and do not get re-rooted), when the project file path changes.
	static inline const std::string projectSubdirectoryFilter {"projectSubDir"};

	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	URIAnnotation(const URIAnnotation& other)
		: AnnotationBase({{"filter", &filter_}, {"folderTypeKey", &folderTypeKey_}}),
		  filter_(other.filter_),
		  folderTypeKey_(other.folderTypeKey_) {
	}

	URIAnnotation(const std::string& filter = "", PathManager::FolderTypeKeys folderTypeKey = PathManager::FolderTypeKeys::Invalid)
		: AnnotationBase({{"filter", &filter_}, {"folderTypeKey", &folderTypeKey_}}),
		  filter_(filter),
		  folderTypeKey_(static_cast<int>(folderTypeKey)) {
	}

	URIAnnotation& operator=(const URIAnnotation& other) {
		filter_ = other.filter_;
		folderTypeKey_ = other.folderTypeKey_;
		return *this;
	}

    const std::string& getFilter() const {
		return *filter_;
	}

	core::PathManager::FolderTypeKeys getFolderTypeKey() const {
		return static_cast<core::PathManager::FolderTypeKeys>(*folderTypeKey_);
	}

	bool isProjectSubdirectoryURI() const {
		return filter_.asString() == projectSubdirectoryFilter;
	}

    data_storage::Value<std::string> filter_;

	// This is really a PathManager::FolerTypeKey
	data_storage::Value<int> folderTypeKey_;
};

class HiddenProperty : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "HiddenProperty", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	HiddenProperty(const HiddenProperty& other) : AnnotationBase({}) {}
	HiddenProperty() : AnnotationBase({}){}

	HiddenProperty& operator=(const HiddenProperty& other) {
		return *this;
	}
};

class ReadOnlyAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"ReadOnlyAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	ReadOnlyAnnotation(const ReadOnlyAnnotation& other) : AnnotationBase({}) {}
	ReadOnlyAnnotation() : AnnotationBase({}) {}
	ReadOnlyAnnotation& operator=(const ReadOnlyAnnotation& other) {
		return *this;
	}
};

// Tag a Table with array semantic. This implies
// - the properties in the Table have empty property names
// - PrimitiveType::Ref properties in the Table must not contain nullptrs.
//   Instead of setting a reference property to nullptr the property needs to be removed.
class ArraySemanticAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "ArraySemanticAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	ArraySemanticAnnotation(const ArraySemanticAnnotation& other) : AnnotationBase({}) {}
	ArraySemanticAnnotation() : AnnotationBase({}) {}
	ArraySemanticAnnotation& operator=(const ArraySemanticAnnotation& other) {
		return *this;
	}
};

/**
 * @brief Marks a Table containing tags used by the rendering system.
 * 
 * Not to be mixed up with UserTagContainerAnnotation
*/
class TagContainerAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"TagContainerAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	TagContainerAnnotation(const TagContainerAnnotation& other) : AnnotationBase({}) {}
	TagContainerAnnotation() : AnnotationBase({}) {}
	TagContainerAnnotation& operator=(const TagContainerAnnotation& other) {
		return *this;
	}
};

/**
 * @brief Marks a Table containing the tags to be rendered by a RenderLayer
*/
class RenderableTagContainerAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"RenderableTagContainerAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	RenderableTagContainerAnnotation(const RenderableTagContainerAnnotation& other) : AnnotationBase({}) {}
	RenderableTagContainerAnnotation() : AnnotationBase({}) {}
	RenderableTagContainerAnnotation& operator=(const RenderableTagContainerAnnotation& other) {
		return *this;
	}
};

/**
 * @brief Marks a Table property containing user-defined tags.
 * 
 * The user-defined Tags have no special semantics within RamsesComposer
*/
class UserTagContainerAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"UserTagContainerAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	UserTagContainerAnnotation(const UserTagContainerAnnotation& other) : AnnotationBase({}) {}
	UserTagContainerAnnotation() : AnnotationBase({}) {}
	UserTagContainerAnnotation& operator=(const UserTagContainerAnnotation& other) {
		return *this;
	}
};

class EnumerationAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "EnumerationAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	EnumerationAnnotation(EnumerationAnnotation const& other) : AnnotationBase({{"type", &type_}}),
																type_(other.type_) {}

	EnumerationAnnotation(EUserTypeEnumerations type = EUserTypeEnumerations::Undefined) : AnnotationBase({{"type", &type_}}),
															  type_(static_cast<int>(type)) {
	}

	EnumerationAnnotation& operator=(const EnumerationAnnotation& other) {
		type_ = other.type_;
		return *this;
	}

	// This is really a core::EUserTypeEnumerations
	data_storage::Value<int> type_;
};

/**
 * @brief Customize the behaviour of the reference editor in the property browser.
 * 
 * References properties without this annotation will show a warning color in the editor field if the reference is empty.
 * Additionally the text to be shown in the editor field for empty references can be customized.
*/
class ExpectEmptyReference : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"EmptyReferenceAllowable", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}

	ExpectEmptyReference(ExpectEmptyReference const& other) : AnnotationBase({{"label", &emptyRefLabel_}}),
															  emptyRefLabel_(other.emptyRefLabel_) {}

	ExpectEmptyReference(std::string emptyRefLabel = std::string()) : AnnotationBase({{"label", &emptyRefLabel_}}),
															 emptyRefLabel_(emptyRefLabel) {
	}

	ExpectEmptyReference& operator=(const ExpectEmptyReference& other) {
		emptyRefLabel_ = other.emptyRefLabel_;
		return *this;
	}

	data_storage::Value<std::string> emptyRefLabel_;
};

/**
 * @brief Tag a property as enabled starting at a particular logic engine feature level.
 * 
 * Property with a feature level higher than the current project feature level must not be exposed to the user, are not readable or 
 * writable from the Python API, and must not be used in the engine backend / adaptor classes.
*/
class FeatureLevel : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"FeatureLevel", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	FeatureLevel(const FeatureLevel& other) : AnnotationBase({{"featureLevel", &featureLevel_}}), featureLevel_(other.featureLevel_) {}
	FeatureLevel(int featureLevel = 1) : AnnotationBase({{"featureLevel", &featureLevel_}}), featureLevel_(featureLevel) {}

	FeatureLevel& operator=(const FeatureLevel& other) {
		featureLevel_ = other.featureLevel_;
		return *this;
	}

	data_storage::Value<int> featureLevel_;
};

/**
 * @brief Mark a property as an editor-only property that is not part of the persistent data model.
 * 
 * Volatile properties
 * - are ignored by undo/redo, prefab update, and external reference update.
 * - are not serialized, i.e. they are not written to the project files and ignored by copy/paste.
 * - can be set using the CommandInterface as usual, although this will not trigger a Prefab update or generate an undo stack entry.
 * - can be changed using the CommandInterface even when the property is linked or read-only.
 * - changes will generate DataChangeRecorder entries as for other properties.
 * - if a Table/Struct/Array property is tagged as volatile all child properties _must_ be tagged as volatile too.
 *   this is not checked but undo/prefab/extref/serialization will rely on this.
 * 
 * Volatile properties are used for internal editor-only state needed only at runtime
*/
class VolatileProperty : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"VolatileProperty", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	VolatileProperty(const VolatileProperty& other) : AnnotationBase({}) {}
	VolatileProperty() : AnnotationBase({}) {}

	VolatileProperty& operator=(const VolatileProperty& other) {
		return *this;
	}
};


/**
 * @brief Mark a PrimitiveType::Array property as resizable by the user.
*/
class ResizableArray : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"ResizableArray", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	ResizableArray(const ResizableArray& other) : AnnotationBase({}) {}
	ResizableArray() : AnnotationBase({}) {}

	ResizableArray& operator=(const ResizableArray& other) {
		return *this;
	}
};


}  // namespace raco::data_storage