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

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/AbstractObjectAdaptor.h"

namespace raco::ramses_adaptor {

class SceneAdaptor;

class AbstractSceneAdaptor;

struct Factories {
	static UniqueObjectAdaptor createAdaptor(SceneAdaptor* buildContext, core::SEditorObject obj);

	static UniqueAbstractObjectAdaptor createAbstractAdaptor(AbstractSceneAdaptor* buildContext, core::SEditorObject obj);

private:
};
}  // namespace raco::ramses_adaptor
