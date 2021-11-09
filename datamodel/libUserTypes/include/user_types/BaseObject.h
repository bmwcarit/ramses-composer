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

#include "data_storage/BasicAnnotations.h"
#include "data_storage/BasicTypes.h"
#include "core/CoreAnnotations.h"
#include "core/EditorObject.h"
#include "core/FileChangeCallback.h"
#include "core/FileChangeMonitor.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"

#include <memory>


namespace raco::user_types {
using namespace raco::core;

// scene object hierarchy as C++ classes:
// - Value<T> (without annotations) or Property<T, Annos...> (with annotations) data members allowed
// - needs to provide ReflectionInterface -> automatic serialization & property browser etc interface
// - normal data members allowed: volatile (i.e. not serialized)

class BaseObject : public EditorObject {
public:
	BaseObject(BaseObject const& other) : EditorObject() {
		fillPropertyDescription();
	}

	BaseObject(std::string name = std::string(), std::string id = std::string())
		: EditorObject(name, id)
	{
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
	}

	virtual FileChangeMonitor::UniqueListener registerFileChangedHandler(BaseContext& context, const ValueHandle& value, FileChangeCallback::Callback callback) {
		auto resourceAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), value);
		return context.fileChangeMonitor()->registerFileChangedHandler(resourceAbsPath, 
			{&context, shared_from_this(), std::move(callback)});
	}
};

}