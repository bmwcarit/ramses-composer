/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/LuaInterfaceAdaptor.h"

#include "utils/FileUtils.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "user_types/PrefabInstance.h"

namespace raco::ramses_adaptor {

LuaInterfaceAdaptor::LuaInterfaceAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaInterface> editorObject)
	: UserTypeObjectAdaptor{sceneAdaptor, editorObject},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::LuaInterface::objectName_}, [this]() {
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  inputSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaInterface::inputs_}, [this](auto) {
		  // Only normal tag dirty here; don't set recreateStatus_
		  tagDirty();
	  })},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  childrenSubscription_(sceneAdaptor_->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		  if (parent_ != editorObject_->getParent()) {
			  setupParentSubscription();
			  tagDirty();
			  recreateStatus_ = true;
		  }
	  })),
	  linksLifecycleSubscription_{sceneAdaptor_->dispatcher()->registerOnLinksLifeCycle(
		  [this](const core::LinkDescriptor& link) {
			  if (sceneAdaptor_->optimizeForExport() && (link.start.object() == editorObject_ || link.end.object() == editorObject_)) {
				  tagDirty();
				  recreateStatus_ = true; 
			  }
		  }, 
		  [this](const core::LinkDescriptor& link) {
			  if (sceneAdaptor_->optimizeForExport() && (link.start.object() == editorObject_ || link.end.object() == editorObject_)) {
				  tagDirty();
				  recreateStatus_ = true;
			  }
		  })},
	  linkValidityChangeSubscription_{sceneAdaptor_->dispatcher()->registerOnLinkValidityChange([this](const core::LinkDescriptor& link) {
		  if (sceneAdaptor_->optimizeForExport() && (link.start.object() == editorObject_ || link.end.object() == editorObject_)) {
			  tagDirty();
			  recreateStatus_ = true;
		  }
	  })} {
	setupParentSubscription();
}

void LuaInterfaceAdaptor::setupParentSubscription() {
	parent_ = editorObject_->getParent();

	if (parent_ && parent_->as<user_types::PrefabInstance>()) {
		parentNameSubscription_ = sceneAdaptor_->dispatcher()->registerOn({parent_, &user_types::LuaScript::objectName_}, [this]() {
			tagDirty();
			recreateStatus_ = true;
		});
	} else {
		parentNameSubscription_ = components::Subscription{};
	}
}

void LuaInterfaceAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	if (ramsesInterface_) {
		logicNodes.push_back(ramsesInterface_.get());
	}
}

const rlogic::Property* LuaInterfaceAdaptor::getProperty(const std::vector<std::string>& names) {
	if (ramsesInterface_ && names.size() >= 1 && names[0] == "inputs") {
		return ILogicPropertyProvider::getPropertyRecursive(ramsesInterface_->getInputs(), names, 1);
	}
	return nullptr;
}

void LuaInterfaceAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR, level, valueHandle, message);
}


std::string LuaInterfaceAdaptor::generateRamsesObjectName() const {
	auto prefabInstOuter = raco::core::PrefabOperations::findOuterContainingPrefabInstance(editorObject_);
	if (prefabInstOuter) {
		if (prefabInstOuter == parent_) {
			return parent_->objectName() + "." + editorObject_->objectName();
		} else {
			return editorObject_->objectName() + "-" + editorObject_->objectID();
		}
	}
	return editorObject_->objectName();
}

bool LuaInterfaceAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	if (recreateStatus_) {
		auto interfaceText = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject_, &user_types::LuaInterface::uri_}));
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "{}: {}", generateRamsesObjectName(), interfaceText);
		ramsesInterface_.reset();

		if (!interfaceText.empty() &&
			(!sceneAdaptor_->optimizeForExport() || 
				!raco::core::Queries::getLinksConnectedToObject(sceneAdaptor_->project(), editorObject_, true, false).empty() &&
				raco::core::Queries::getLinksConnectedToObject(sceneAdaptor_->project(), editorObject_, false, true).empty())) {
			ramsesInterface_ = raco::ramses_base::ramsesLuaInterface(&sceneAdaptor_->logicEngine(), interfaceText, generateRamsesObjectName(), editorObject_->objectIDAsRamsesLogicID());
		}
	}

	if (ramsesInterface_) {
		core::ValueHandle luaInputs{editorObject_, &user_types::LuaInterface::inputs_};
		auto success = setLuaInputInEngine(ramsesInterface_->getInputs(), luaInputs);
		LOG_WARNING_IF(log_system::RAMSES_ADAPTOR, !success, "Script set properties failed: {}", LogicEngineErrors{sceneAdaptor_->logicEngine()});
	}

	tagDirty(false);
	recreateStatus_ = false;
	return true;
}


}  // namespace raco::ramses_adaptor
