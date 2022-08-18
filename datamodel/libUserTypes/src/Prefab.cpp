/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Prefab.h"

#include "core/Handles.h"

namespace raco::user_types {

void Prefab::onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const {
	BaseObject::onBeforeRemoveReferenceToThis(sourceReferenceProperty);
	auto srcRootObject = sourceReferenceProperty.rootObject();
	instances_.erase(srcRootObject);
}

void Prefab::onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const {
	BaseObject::onAfterAddReferenceToThis(sourceReferenceProperty);
	auto srcRootObject = sourceReferenceProperty.rootObject();
	instances_.insert(srcRootObject);
}

}  // namespace raco::user_types