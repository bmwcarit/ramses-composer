/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/AnchorPointAdaptor.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"
#include "ramses_adaptor/NodeAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"

namespace raco::ramses_adaptor {

raco::ramses_adaptor::AnchorPointAdaptor::AnchorPointAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SAnchorPoint anchorPoint)
	: UserTypeObjectAdaptor{sceneAdaptor, anchorPoint},
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnchorPoint::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnchorPoint::node_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject_, &user_types::AnchorPoint::camera_}, [this]() { tagDirty(); })},
	  dirtySubscription_{sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() { tagDirty(); })} {
}

void AnchorPointAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	logicNodes.emplace_back(anchorPoint_.get());
}

const rlogic::Property* AnchorPointAdaptor::getProperty(const std::vector<std::string>& propertyNamesVector) {
	if (anchorPoint_ && propertyNamesVector.size() >= 2) {
		return anchorPoint_->getOutputs()->getChild(propertyNamesVector[1]);
	}
	return nullptr;
}

void AnchorPointAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

bool AnchorPointAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);
	anchorPoint_.reset();
	
	raco::ramses_base::RamsesNodeBinding nodeBinding = lookupNodeBinding(sceneAdaptor_, *editorObject_->node_);

	raco::ramses_base::RamsesCameraBinding cameraBinding = nullptr;
	auto camera = *editorObject_->camera_;
	if (auto cameraAdaptor = sceneAdaptor_->lookup<PerspectiveCameraAdaptor>(camera)) {
		cameraBinding = cameraAdaptor->cameraBinding();
	} else if (auto cameraAdaptor = sceneAdaptor_->lookup<OrthographicCameraAdaptor>(camera)) {
		cameraBinding = cameraAdaptor->cameraBinding();
	}

	if (nodeBinding && cameraBinding) {
		anchorPoint_ = raco::ramses_base::ramsesAnchorPoint(&sceneAdaptor_->logicEngine(), nodeBinding, cameraBinding, editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID());
	}
		
	tagDirty(false);
	return false;
}

void AnchorPointAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	if (anchorPoint_) {
		core::ValueHandle outputs(editorObject_, &user_types::AnchorPoint::outputs_);
		getOutputFromEngine(*anchorPoint_->getOutputs(), outputs, recorder);
	}
}

std::vector<ExportInformation> AnchorPointAdaptor::getExportInformation() const {
	if (anchorPoint_ == nullptr) {
		return {};
	}

	return {ExportInformation{"AnchorPoint", anchorPoint_->getName().data()}};
}

}  // namespace raco::ramses_adaptor