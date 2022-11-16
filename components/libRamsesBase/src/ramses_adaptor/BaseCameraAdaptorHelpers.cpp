/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/BaseCameraAdaptorHelpers.h"

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"

#include "user_types/BaseCamera.h"

#include <ramses-logic/RamsesCameraBinding.h>

#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

void BaseCameraAdaptorHelpers::sync(std::shared_ptr<user_types::BaseCamera> editorObject, ramses::Camera* ramsesCamera, rlogic::RamsesCameraBinding* cameraBinding, core::Errors* errors) {
	bool allValid = true;
	int clippedWidth = raco::ramses_base::clipAndCheckIntProperty({editorObject, {"viewport", "width"}}, errors, &allValid);
	int clippedHeight = raco::ramses_base::clipAndCheckIntProperty({editorObject, {"viewport", "height"}}, errors, &allValid);

	if (allValid) {
		ramsesCamera->setViewport(*editorObject->viewport_->offsetX_, *editorObject->viewport_->offsetY_, clippedWidth, clippedHeight);
	}

	cameraBinding->setName(std::string(editorObject->objectName() + "_CameraBinding").c_str());
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetX")->set<int>(*editorObject->viewport_->offsetX_);
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetY")->set<int>(*editorObject->viewport_->offsetY_);
	if (allValid) {
		cameraBinding->getInputs()->getChild("viewport")->getChild("width")->set<int>(clippedWidth);
		cameraBinding->getInputs()->getChild("viewport")->getChild("height")->set<int>(clippedHeight);
	}
}

const rlogic::Property* BaseCameraAdaptorHelpers::getProperty(rlogic::RamsesCameraBinding* cameraBinding, const std::vector<std::string>& propertyNamesVector) {
	if (cameraBinding && propertyNamesVector.size() >= 1 && propertyNamesVector[0] == "viewport") {
		return ILogicPropertyProvider::getPropertyRecursive(cameraBinding->getInputs(), propertyNamesVector);
	}
	return nullptr;
}

}  // namespace raco::ramses_adaptor
