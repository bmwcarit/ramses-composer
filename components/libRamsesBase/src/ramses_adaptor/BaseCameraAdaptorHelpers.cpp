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

#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

std::array<components::Subscription, 4> BaseCameraAdaptorHelpers::viewportSubscriptions(SceneAdaptor* sceneAdaptor, ObjectAdaptor* cameraAdaptor) {
	return std::array<components::Subscription, 4> {
		sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{ cameraAdaptor->baseEditorObject() }.get("viewPortOffsetX"), [cameraAdaptor]() {
				cameraAdaptor->tagDirty();
			}),
			sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{ cameraAdaptor->baseEditorObject() }.get("viewPortOffsetY"), [cameraAdaptor]() {
				cameraAdaptor->tagDirty();
			}),
			sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{ cameraAdaptor->baseEditorObject() }.get("viewPortWidth"), [cameraAdaptor]() {
				cameraAdaptor->tagDirty();
			}),
			sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{ cameraAdaptor->baseEditorObject() }.get("viewPortHeight"), [cameraAdaptor]() {
				cameraAdaptor->tagDirty();
			})
	};
}

void BaseCameraAdaptorHelpers::sync(std::shared_ptr<user_types::BaseCamera> editorObject, ramses::Camera* ramsesCamera, rlogic::RamsesCameraBinding* cameraBinding) {
	ramsesCamera->setViewport(*editorObject->viewportOffsetX_, *editorObject->viewportOffsetY_, *editorObject->viewportWidth_, *editorObject->viewportHeight_);
	cameraBinding->setName(std::string(editorObject->objectName() + "_CameraBinding").c_str());
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetX")->set(static_cast<int>(*editorObject->viewportOffsetX_));
	cameraBinding->getInputs()->getChild("viewport")->getChild("offsetY")->set(static_cast<int>(*editorObject->viewportOffsetY_));
	// Ramses asserts if the viewport width/height <=0. Unfortunately we cannot prevent a link from setting the value to <=0?
	cameraBinding->getInputs()->getChild("viewport")->getChild("width")->set(std::max(1, static_cast<int>(*editorObject->viewportWidth_)));
	cameraBinding->getInputs()->getChild("viewport")->getChild("height")->set(std::max(1, static_cast<int>(*editorObject->viewportHeight_)));
}

const rlogic::Property* BaseCameraAdaptorHelpers::getProperty(rlogic::RamsesCameraBinding* cameraBinding, const std::vector<std::string>& propertyNamesVector) {
	static std::map<std::string_view, std::string_view> propertyNameToViewportPropertyName{
		{ "viewPortOffsetX", "offsetX" },
		{ "viewPortOffsetY", "offsetY" },
		{ "viewPortWidth", "width" },
		{ "viewPortHeight", "height" }
	};
	assert(propertyNamesVector.size() == 1);
	std::string const propName = propertyNamesVector[0];
	if (propertyNameToViewportPropertyName.find(propName) != propertyNameToViewportPropertyName.end()) {
		auto ramsesViewportProperties = cameraBinding->getInputs()->getChild("viewport");
		assert(ramsesViewportProperties != nullptr);
		auto ramsesViewportProperty = ramsesViewportProperties->getChild(propertyNameToViewportPropertyName.at(propName));
		assert(ramsesViewportProperty != nullptr);
		return ramsesViewportProperty;
	}
	return nullptr;
}

}  // namespace raco::ramses_adaptor
