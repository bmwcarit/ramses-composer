/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Undo.h"

#include "core/ChangeRecorder.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/Link.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"

#include <cassert>

namespace raco::core {

raco::core::ChangeBase::ChangeBase() {

}


}  // namespace raco::core
