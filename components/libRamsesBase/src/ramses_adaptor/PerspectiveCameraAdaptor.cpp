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
	: SpatialAdaptor(sceneAdaptor, editorObject, ramses_base::ramsesPerspectiveCamera(sceneAdaptor->scene(), editorObject->objectIDAsRamsesLogicID())),

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
	if (*editorObject_->frustumType_ == static_cast<int>(user_types::EFrustumType::Aspect_FieldOfView)) {
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
	cameraBinding_ = ramses_base::ramsesCameraBinding(getRamsesObjectPointer(), &sceneAdaptor_->logicEngine(), editorObject_->objectIDAsRamsesLogicID(), *editorObject_->frustumType_ == static_cast<int>(user_types::EFrustumType::Planes));

	BaseCameraAdaptorHelpers::sync(editorObject(), ramsesObject().get(), cameraBinding_.get(), errors);

	for (size_t index = 0; index < editorObject_->frustum_->size(); index++) {
		auto& propName = editorObject_->frustum_->name(index);
		cameraBinding_->getInputs()->getChild("frustum")->getChild(propName)->set(static_cast<float>(editorObject_->frustum_->get(propName)->asDouble()));
	}

	tagDirty(false);
	return true;
}

ramses::Property* PerspectiveCameraAdaptor::getProperty(const std::vector<std::string_view>& propertyNamesVector) {
	if (auto p = BaseCameraAdaptorHelpers::getProperty(cameraBinding_.get(), propertyNamesVector)) {
		return p;
	}
	if (cameraBinding_ && propertyNamesVector.size() >= 1 && propertyNamesVector[0] == "frustum") {
		return ILogicPropertyProvider::getPropertyRecursive(cameraBinding_->getInputs(), propertyNamesVector);
	}
	return SpatialAdaptor::getProperty(propertyNamesVector);
}

void PerspectiveCameraAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	SpatialAdaptor::getLogicNodes(logicNodes);
	logicNodes.push_back(cameraBinding_.get());
}

ramses_base::RamsesCameraBinding PerspectiveCameraAdaptor::cameraBinding() {
	return cameraBinding_;
}

std::vector<ExportInformation> PerspectiveCameraAdaptor::getExportInformation() const {
	auto result = std::vector<ExportInformation>();
	if (getRamsesObjectPointer() != nullptr) {
		result.emplace_back(ramses::ERamsesObjectType::PerspectiveCamera, ramsesObject().getName());
	}

	if (nodeBinding() != nullptr) {
		result.emplace_back("NodeBinding", nodeBinding()->getName());
	}

	if (cameraBinding_) {
		result.emplace_back("CameraBinding", cameraBinding_->getName());
	}

	return result;
}

}  // namespace raco::ramses_adaptor
