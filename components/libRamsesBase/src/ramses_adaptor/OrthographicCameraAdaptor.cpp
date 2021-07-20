/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/OrthographicCameraAdaptor.h"

#include "ramses_adaptor/BaseCameraAdaptorHelpers.h"
#include "ramses_adaptor/SceneAdaptor.h"

#include "ramses_base/RamsesHandles.h"



namespace raco::ramses_adaptor {
OrthographicCameraAdaptor::OrthographicCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::OrthographicCamera> editorObject)
	: SpatialAdaptor(sceneAdaptor, editorObject, raco::ramses_base::ramsesOrthographicCamera(sceneAdaptor->scene())),
	  cameraBinding_{raco::ramses_base::ramsesCameraBinding(&sceneAdaptor->logicEngine())},
	  viewportSubscriptions_{ std::move(BaseCameraAdaptorHelpers::viewportSubscriptions(sceneAdaptor, this)) },
	  frustrumSubscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("near"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("far"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("left"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("right"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("bottom"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("top"), [this]() {
			  tagDirty();
		  })} {
	cameraBinding_->setRamsesCamera(&ramsesObject());
}

OrthographicCameraAdaptor::~OrthographicCameraAdaptor() {
	sceneAdaptor_->setCamera(nullptr);
}

bool OrthographicCameraAdaptor::sync(core::Errors* errors) {
	SpatialAdaptor::sync(errors);
	BaseCameraAdaptorHelpers::sync(editorObject(), &ramsesObject(), cameraBinding_.get());
	ramsesObject().setFrustum(static_cast<float>(*editorObject()->left_), static_cast<float>(*editorObject()->right_), static_cast<float>(*editorObject()->bottom_), static_cast<float>(*editorObject()->top_), static_cast<float>(*editorObject()->near_), static_cast<float>(*editorObject()->far_));
	sceneAdaptor_->setCamera(&ramsesObject());
	// The logic engine will always set the entire struct even if there is a link for only one of the values, and use the default values in the binding
	// for the non-linked elements in the struct - so we need to also set the default values for the bindings.
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("nearPlane")->set(static_cast<float>(*editorObject()->near_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("farPlane")->set(static_cast<float>(*editorObject()->far_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("leftPlane")->set(static_cast<float>(*editorObject()->left_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("rightPlane")->set(static_cast<float>(*editorObject()->right_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("bottomPlane")->set(static_cast<float>(*editorObject()->bottom_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("topPlane")->set(static_cast<float>(*editorObject()->top_));
	tagDirty(false);
	return true;
}

void OrthographicCameraAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	logicNodes.push_back(cameraBinding_.get());
}

const rlogic::Property* OrthographicCameraAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector)
{
	using raco::user_types::Node;
	using raco::user_types::property_name;

	static std::map<std::string_view, std::string_view> propertyNameToFrustrumPropertyName{
		{ "near", "nearPlane" },
		{ "far", "farPlane" },
		{ "left", "leftPlane" },
		{ "right", "rightPlane" },
		{ "bottom", "bottomPlane" },
		{ "top", "topPlane" },
	};
	std::string propName = propertyNamesVector[0];
	assert(propertyNamesVector.size() == 1);
	if (propertyNameToFrustrumPropertyName.find(propName) != propertyNameToFrustrumPropertyName.end()) {
		auto const ramsesFrustrumProperties = cameraBinding_->getInputs()->getChild("frustumProperties");
		assert(ramsesFrustrumProperties != nullptr);
		auto const ramsesFrustrumProperty = ramsesFrustrumProperties->getChild(propertyNameToFrustrumPropertyName.at(propName));
		assert(ramsesFrustrumProperty != nullptr);
		return ramsesFrustrumProperty;
	}
	if (auto p = BaseCameraAdaptorHelpers::getProperty(cameraBinding_.get(), propertyNamesVector)) {
		return p;
	}
	return SpatialAdaptor::getProperty(propertyNamesVector);
}

}  // namespace raco::ramses_adaptor