/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/ObjectAdaptor.h"

#include "ramses_adaptor/SceneAdaptor.h"


namespace raco::ramses_adaptor {

ObjectAdaptor::~ObjectAdaptor() {
}

bool ObjectAdaptor::sync(core::Errors* errors) {
	return false;
}

bool ObjectAdaptor::isDirty() const {
	return dirtyStatus_;
}

void ObjectAdaptor::tagDirty(bool newStatus) {
	dirtyStatus_ = newStatus;
}

}  // namespace raco::ramses_adaptor