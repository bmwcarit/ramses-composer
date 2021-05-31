/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"

#include "ramses_adaptor/BaseCameraAdaptorHelpers.h"
#include "ramses_adaptor/SceneAdaptor.h"

#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

PerspectiveCameraAdaptor::PerspectiveCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::PerspectiveCamera> editorObject)
	: SpatialAdaptor(sceneAdaptor, editorObject, raco::ramses_base::ramsesPerspectiveCamera(sceneAdaptor->scene())),
	  cameraBinding_{ sceneAdaptor->logicEngine().createRamsesCameraBinding(), [this](rlogic::RamsesCameraBinding* binding) { this->sceneAdaptor_->logicEngine().destroy(*binding); } },
	  viewportSubscriptions_{ std::move(BaseCameraAdaptorHelpers::viewportSubscriptions(sceneAdaptor, this)) },
      frustrumSubscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("near"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("far"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("fov"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("aspect"), [this]() {
			  tagDirty();
		  })} {
	cameraBinding_->setRamsesCamera(&ramsesObject());
}

PerspectiveCameraAdaptor::~PerspectiveCameraAdaptor() {
	sceneAdaptor_->setCamera(nullptr);
}

bool PerspectiveCameraAdaptor::sync(core::Errors* errors) {
	SpatialAdaptor::sync(errors);
	BaseCameraAdaptorHelpers::sync(editorObject(), &ramsesObject(), cameraBinding_.get());
	ramsesObject().setFrustum(static_cast<float>(*editorObject()->fov_), static_cast<float>(*editorObject()->aspect_), static_cast<float>(*editorObject()->near_), static_cast<float>(*editorObject()->far_));
	sceneAdaptor_->setCamera(&ramsesObject());
	// The logic engine will always set the entire struct even if there is a link for only one of the values, and use the default values in the binding
	// for the non-linked elements in the struct - so we need to also set the default values for the bindings.
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("nearPlane")->set(static_cast<float>(*editorObject()->near_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("farPlane")->set(static_cast<float>(*editorObject()->far_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("fieldOfView")->set(static_cast<float>(*editorObject()->fov_));
	cameraBinding_->getInputs()->getChild("frustumProperties")->getChild("aspectRatio")->set(static_cast<float>(*editorObject()->aspect_));
	tagDirty(false);
	return true;
}

const rlogic::Property& PerspectiveCameraAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector)
{
	using raco::user_types::Node;
	using raco::user_types::property_name;

	static std::map<std::string_view, std::string_view> propertyNameToFrustrumPropertyName{
		{ "near", "nearPlane" },
		{ "far", "farPlane" },
		{ "fov", "fieldOfView" },
		{ "aspect", "aspectRatio" }
	};
	std::string propName = propertyNamesVector[0];
	assert(propertyNamesVector.size() == 1);
	if(propertyNameToFrustrumPropertyName.find(propName) != propertyNameToFrustrumPropertyName.end()) {
		auto const ramsesFrustrumProperties = cameraBinding_->getInputs()->getChild("frustumProperties");
		assert(ramsesFrustrumProperties != nullptr);
		auto const ramsesFrustrumProperty = ramsesFrustrumProperties->getChild(propertyNameToFrustrumPropertyName.at(propName));
		assert(ramsesFrustrumProperty != nullptr);
		return *ramsesFrustrumProperty;
	}
	if (auto p = BaseCameraAdaptorHelpers::getProperty(cameraBinding_.get(), propertyNamesVector)) {
		return *p;
	}
	return SpatialAdaptor::getProperty(propertyNamesVector);
}

void PerspectiveCameraAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	logicNodes.push_back(cameraBinding_.get());
}

}  // namespace raco::ramses_adaptor
