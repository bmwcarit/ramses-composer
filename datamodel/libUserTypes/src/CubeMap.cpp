/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/CubeMap.h"

#include "user_types/SyncTableWithEngineInterface.h"
#include "Validation.h"
#include "core/Context.h"
#include "log_system/log.h"
#include "utils/FileUtils.h"
#include <algorithm>


namespace raco::user_types {

void CubeMap::updateFromExternalFile(BaseContext& context) {
	context.errors().removeError({shared_from_this()});
	validateURI(context, {shared_from_this(), &CubeMap::uriFront_});
	validateURI(context, {shared_from_this(), &CubeMap::uriBack_});
	validateURI(context, {shared_from_this(), &CubeMap::uriLeft_});
	validateURI(context, {shared_from_this(), &CubeMap::uriRight_});
	validateURI(context, {shared_from_this(), &CubeMap::uriTop_});
	validateURI(context, {shared_from_this(), &CubeMap::uriBottom_});

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
