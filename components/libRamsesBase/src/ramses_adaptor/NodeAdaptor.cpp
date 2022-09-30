/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/NodeAdaptor.h"

namespace raco::ramses_adaptor {

NodeAdaptor::NodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SNode node)
	: SpatialAdaptor{sceneAdaptor, node, raco::ramses_base::ramsesNode(sceneAdaptor->scene())} {}

std::vector<ExportInformation> NodeAdaptor::getExportInformation() const {
	auto result = std::vector<ExportInformation>();
	if (getRamsesObjectPointer() != nullptr) {
		result.emplace_back(ramses::ERamsesObjectType_Node, ramsesObject().getName());
	}

	if (nodeBinding() != nullptr){
		result.emplace_back("NodeBinding", nodeBinding()->getName().data());
	}

	return result;
}

}

