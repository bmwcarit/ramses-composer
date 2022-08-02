/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

void BaseCameraAdaptorHelpers::sync(std::shared_ptr<user_types::BaseCamera> editorObject, ramses::Camera* ramsesCamera, rlogic::RamsesCameraBinding* cameraBinding) {
	ramsesCamera->setViewport(*editorObject->viewport_->offsetX_, *editorObject->viewport_->offsetY_, *editorObject->viewport_->width_, *editorObject->viewport_->height_);
	cameraBinding->setName(std::string(editorObject->objectName() + "_CameraBinding").c_str());
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetX")->set(static_cast<int>(*editorObject->viewport_->offsetX_));
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetY")->set(static_cast<int>(*editorObject->viewport_->offsetY_));
	// Ramses asserts if the viewport width/height <=0. Unfortunately we cannot prevent a link from setting the value to <=0?
	cameraBinding->getInputs()->getChild("viewport")->getChild("width")->set(std::max(1, static_cast<int>(*editorObject->viewport_->width_)));
	cameraBinding->getInputs()->getChild("viewport")->getChild("height")->set(std::max(1, static_cast<int>(*editorObject->viewport_->height_)));
}

const rlogic::Property* BaseCameraAdaptorHelpers::getProperty(rlogic::RamsesCameraBinding* cameraBinding, const std::vector<std::string>& propertyNamesVector) {
	if (cameraBinding && propertyNamesVector.size() >= 1 && propertyNamesVector[0] == "viewport") {
		return ILogicPropertyProvider::getPropertyRecursive(cameraBinding->getInputs(), propertyNamesVector);
	}
	return nullptr;
}

}  // namespace raco::ramses_adaptor
