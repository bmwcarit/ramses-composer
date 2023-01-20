/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/utilities.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"
#include "ramses_adaptor/NodeAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"

namespace raco::ramses_adaptor {

raco::ramses_base::RamsesNodeBinding lookupNodeBinding(const SceneAdaptor* sceneAdaptor, core::SEditorObject node) {
	if (auto nodeAdaptor = sceneAdaptor->lookup<NodeAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<MeshNodeAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<PerspectiveCameraAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<OrthographicCameraAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	}
	return {};
}

}  // namespace raco::ramses_adaptor