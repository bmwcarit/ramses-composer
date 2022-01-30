/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/PrefabInstance.h"

namespace raco::user_types {

std::string PrefabInstance::mapObjectIDToInstance(SEditorObject obj, SPrefab prefab, SPrefabInstance instance) {
	if (obj == prefab) {
		return instance->objectID();
	}
	return EditorObject::XorObjectIDs(obj->objectID(), instance->objectID());
}

std::string PrefabInstance::mapObjectIDFromInstance(SEditorObject obj, SPrefab prefab, SPrefabInstance instance) {
	if (obj == instance) {
		return prefab->objectID();
	}
	return EditorObject::XorObjectIDs(obj->objectID(), instance->objectID());
}

}  // namespace raco::user_types
