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

#include <limits>
#include <ramses/framework/RamsesFrameworkTypes.h>

#ifndef RACO_BACKEND_INTERNAL_SCENE_ID
#define RACO_BACKEND_INTERNAL_SCENE_ID std::numeric_limits<uint64_t>::max()
#endif

#ifndef RACO_BACKEND_ABSTRACT_SCENE_ID
#define RACO_BACKEND_ABSTRACT_SCENE_ID (std::numeric_limits<uint64_t>::max() - 1)
#endif

struct BuildOptions {
	constexpr static ramses::sceneId_t internalSceneId = ramses::sceneId_t { RACO_BACKEND_INTERNAL_SCENE_ID };
	constexpr static ramses::sceneId_t abstractSceneId = ramses::sceneId_t{RACO_BACKEND_ABSTRACT_SCENE_ID};
};
