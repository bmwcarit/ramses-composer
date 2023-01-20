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

#include "user_types/BaseObject.h"
#include "user_types/SyncTableWithEngineInterface.h"

namespace raco::user_types {

bool checkLuaModules(ValueHandle moduleTableHandle, Errors& errors);

bool syncLuaModules(BaseContext& context, ValueHandle moduleHandle, const std::string& fileContents, std::map<std::string, std::string>& cachedModuleRefs, std::string& outError);

}
