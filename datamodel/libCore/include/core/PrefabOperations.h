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

#include "core/Context.h"

namespace raco::user_types {

class Prefab;
using SPrefab = std::shared_ptr<Prefab>;

class PrefabInstance;
using SPrefabInstance = std::shared_ptr<PrefabInstance>;

}  // namespace raco::user_types

namespace raco::core {

class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;

class PrefabOperations {
public:
	static void globalPrefabUpdate(BaseContext& context, bool propagateMissingInterfaceProperties = false);

	static raco::user_types::SPrefabInstance findContainingPrefabInstance(SEditorObject object);
	static raco::user_types::SPrefabInstance findOuterContainingPrefabInstance(SEditorObject object);

	static raco::user_types::SPrefab findContainingPrefab(SEditorObject object);
	
	/**
	 * @brief Check if an objects serves as an interface object in a PrefabInstance.
	 * 
	 * Interface objects are only the LuaScripts which are direct children of non-nested PrefabInstances.
	 * 
	 * @param object object to check
	 * @return true if object is identified as interface object.
	*/
	static bool isInterfaceObject(SEditorObject object);

	/**
	 * @brief Check if a property is an interface property of a prefab instance
	 * 
	 * Interface properties are the luaInputs property subtree of interface objects.
	 * 
	 * @param prop Property to check.
	 * @return true if property is identified as interface property.
	*/
	static bool isInterfaceProperty(const ValueHandle& prop);

	static void prefabUpdateOrderDepthFirstSearch(raco::user_types::SPrefab current, std::vector<raco::user_types::SPrefab>& order);

private:
	static void updatePrefabInstance(BaseContext& context, const raco::user_types::SPrefab& prefab, raco::user_types::SPrefabInstance instance, bool instanceDirty, bool propagateMissingInterfaceProperties);
};

}  // namespace raco::core