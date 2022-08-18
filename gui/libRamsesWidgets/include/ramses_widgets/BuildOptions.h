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

#ifndef RACO_NATIVE_RAMSES_DISPLAY_WINDOW
#define RACO_NATIVE_RAMSES_DISPLAY_WINDOW false
#endif

#ifndef RACO_MINIMAL_PREVIEW_DISPLAY_AREA
#define RACO_MINIMAL_PREVIEW_DISPLAY_AREA false
#endif

#ifndef RACO_INTERNAL_SCENE_IDS_START
#define RACO_INTERNAL_SCENE_IDS_START 1337
#endif

#ifndef RACO_INTERNAL_SCENE_IDS_END
#define RACO_INTERNAL_SCENE_IDS_END 50000
#endif

struct BuildOptions {
	constexpr static bool nativeRamsesDisplayWindow = static_cast<bool>(RACO_NATIVE_RAMSES_DISPLAY_WINDOW);
	constexpr static bool minimalPreviewDisplayArea = static_cast<bool>(RACO_MINIMAL_PREVIEW_DISPLAY_AREA);
	constexpr static int ramsesComposerSceneIdStart = static_cast<int>(RACO_INTERNAL_SCENE_IDS_START);
	constexpr static int ramsesComposerSceneIdEnd = static_cast<int>(RACO_INTERNAL_SCENE_IDS_END);
};
