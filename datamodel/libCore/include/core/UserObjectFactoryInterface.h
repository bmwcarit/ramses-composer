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

#include <string>
#include <map>
#include <functional>

namespace raco::core {

class UserObjectFactoryInterface {
public:
	using CreationFunction = std::function<SEditorObject(const std::string& name, const std::string& id)>;
	using ValueCreationFunction = std::function<data_storage::ValueBase*()>;

	struct TypeDescriptor {
		EditorObject::TypeDescriptor description;
		CreationFunction createFunc;
		ValueCreationFunction createValueFunc;
	};

	virtual SEditorObject createObject(const std::string& type, const std::string& name = std::string(), const std::string& id = std::string()) const = 0;
	virtual data_storage::ValueBase* createValue(const std::string& type) const = 0;

	virtual std::shared_ptr<AnnotationBase> createAnnotation(const std::string& type) const = 0;

	virtual const std::map<std::string, TypeDescriptor>& getTypes() const = 0;
	/**
	 * Indicates if a specific type should be creatable by the user. (E.g. appeare in the right-click context menu of the object tree, or via the python api).
	 * Currently only used for [ProjectSettings] which needs some special consideration in the UI and python api.
	 */
	virtual bool isUserCreatable(const std::string& type, int featureLevel) const = 0;
};

}