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

#include "core/BasicAnnotations.h"
#include "core/BasicTypes.h"
#include "core/CoreAnnotations.h"
#include "core/EditorObject.h"
#include "core/FileChangeCallback.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"

#include <memory>

namespace raco::user_types {
using namespace raco::core;

// These are the available metadata categories.
// They are used as property names of the containers in the BaseObject::metaData_ property.
struct MetaDataCategories {
	static inline const char* GLTF_EXTRAS{"gltfExtras"};
	static inline const char* MESH_INFO{"meshInfo"};
};

// scene object hierarchy as C++ classes:
// - Value<T> (without annotations) or Property<T, Annos...> (with annotations) data members allowed
// - needs to provide ReflectionInterface -> automatic serialization & property browser etc interface
// - normal data members allowed: volatile (i.e. not serialized)

class BaseObject : public EditorObject {
public:
	BaseObject(BaseObject const& other)
		: EditorObject(),
		  userTags_(other.userTags_),
		  metaData_(other.metaData_) {
		fillPropertyDescription();
	}

	BaseObject(std::string name = std::string(), std::string id = std::string())
		: EditorObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("userTags", &userTags_);
		properties_.emplace_back("metaData", &metaData_);
	}

	Property<Table, ArraySemanticAnnotation, HiddenProperty, UserTagContainerAnnotation, DisplayNameAnnotation> userTags_{{}, {}, {}, {}, {"User Tags"}};

	Property<Table, DisplayNameAnnotation> metaData_{{}, {"Meta Data"}};
};

}  // namespace raco::user_types