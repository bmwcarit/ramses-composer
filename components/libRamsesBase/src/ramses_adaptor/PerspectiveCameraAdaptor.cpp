/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"

#include "ramses_adaptor/BaseCameraAdaptorHelpers.h"
#include "ramses_adaptor/SceneAdaptor.h"

#include "ramses_base/RamsesHandles.h"

#include "user_types/PerspectiveCamera.h"

namespace raco::ramses_adaptor {

PerspectiveCameraAdaptor::PerspectiveCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::PerspectiveCamera> editorObject)
	: SpatialAdaptor(sceneAdaptor, editorObject, raco::ramses_base::ramsesPerspectiveCamera(sceneAdaptor->scene())),

	  viewportSubscription_{sceneAdaptor->dispatcher()->registerOnChildren({editorObject, &user_types::PerspectiveCamera::viewport_}, [this](auto) {
		  tagDirty();
	  })},
	  frustrumSubscription_{sceneAdaptor->dispatcher()->registerOnChildren({editorObject, &user_types::PerspectiveCamera::frustum_}, [this](auto) {
		  tagDirty();
	  })} {
}

PerspectiveCameraAdaptor::~PerspectiveCameraAdaptor() {
}

bool PerspectiveCameraAdaptor::sync(core::Errors* errors) {
	SpatialAdaptor::sync(errors);
	if (*editorObject_->frustumType_ == static_cast<int>(raco::user_types::EFrustumType::Aspect_FieldOfView)) {
		(*ramsesObject()).setFrustum(
			static_cast<float>(editorObject()->frustum_->get("fieldOfView")->asDouble()), 
			static_cast<float>(editorObject()->frustum_->get("aspectRatio")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("nearPlane")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("farPlane")->asDouble()));
	} else {
		(*ramsesObject()).setFrustum(
			static_cast<float>(editorObject()->frustum_->get("leftPlane")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("rightPlane")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("bottomPlane")->asDouble()), 
			static_cast<float>(editorObject()->frustum_->get("topPlane")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("nearPlane")->asDouble()),
			static_cast<float>(editorObject()->frustum_->get("farPlane")->asDouble()));
	}
	cameraBinding_ = raco::ramses_base::ramsesCameraBinding(getRamsesObjectPointer(), &sceneAdaptor_->logicEngine(), editorObject_->objectIDAsRamsesLogicID(), *editorObject_->frustumType_ == static_cast<int>(raco::user_types::EFrustumType::Planes));

	BaseCameraAdaptorHelpers::sync(editorObject(), ramsesObject().get(), cameraBinding_.get());

	for (size_t index = 0; index < editorObject_->frustum_->size(); index++) {
		auto& propName = editorObject_->frustum_->name(index);
		cameraBinding_->getInputs()->getChild("frustum")->getChild(propName)->set(static_cast<float>(editorObject_->frustum_->get(propName)->asDouble()));
	}

	tagDirty(false);
	return true;
}

const rlogic::Property* PerspectiveCameraAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector) {
	if (auto p = BaseCameraAdaptorHelpers::getProperty(cameraBinding_.get(), propertyNamesVector)) {
		return p;
	}
	if (cameraBinding_ && propertyNamesVector.size() >= 1 && propertyNamesVector[0] == "frustum") {
		return ILogicPropertyProvider::getPropertyRecursive(cameraBinding_->getInputs(), propertyNamesVector);
	}
	return SpatialAdaptor::getProperty(propertyNamesVector);
}

void PerspectiveCameraAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	logicNodes.push_back(cameraBinding_.get());
}

raco::ramses_base::RamsesCameraBinding PerspectiveCameraAdaptor::cameraBinding() {
	return cameraBinding_;
}

}  // namespace raco::ramses_adaptor
