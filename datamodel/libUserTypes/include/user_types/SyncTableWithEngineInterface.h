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
#include "core/EngineInterface.h"
#include "core/Handles.h"
#include "data_storage/BasicTypes.h"
#include "data_storage/Table.h"

namespace raco::user_types {

using raco::core::PropertyInterface;
using raco::core::PropertyInterfaceList;

// To cache properties nested inside Tables we use a /-separated property path as string.
using OutdatedPropertiesStore = std::map<std::pair<std::string, raco::core::EnginePrimitive>,
    std::unique_ptr<raco::data_storage::ValueBase>>;

void syncTableWithEngineInterface(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const raco::core::ValueHandle& handle, OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd);

template <class... Args>
raco::data_storage::ValueBase* createDynamicProperty(raco::core::EnginePrimitive type);

}  // namespace raco::user_types
