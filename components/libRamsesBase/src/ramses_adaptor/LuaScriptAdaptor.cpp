/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/LuaScriptAdaptor.h"

#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/LogicEngineFormatter.h"
#include "user_types/PrefabInstance.h"
#include "utils/FileUtils.h"

namespace raco::ramses_adaptor {

LuaScriptAdaptor::LuaScriptAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaScript> editorObject)
	: ObjectAdaptor{sceneAdaptor},
	  editorObject_{editorObject},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::LuaScript::objectName_}, [this]() {
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  setupInputValuesSubscription();
		  tagDirty();
		  recreateStatus_ = true;
	  })},
	  childrenSubscription_(sceneAdaptor_->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) {
		  if (parent_ != editorObject_->getParent()) {
			  setupParentSubscription();
			  tagDirty();
			  recreateStatus_ = true;
		  }
	  })) {
	setupParentSubscription();
	setupInputValuesSubscription();
}

raco::user_types::SEditorObject LuaScriptAdaptor::baseEditorObject() noexcept {
	return editorObject_;
}
const raco::user_types::SEditorObject LuaScriptAdaptor::baseEditorObject() const noexcept {
	return editorObject_;
}

void LuaScriptAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	return logicNodes.push_back(rlogicLuaScript());
}

void LuaScriptAdaptor::setupParentSubscription() {
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

void LuaScriptAdaptor::setupInputValuesSubscription() {
	inputSubscription_ = sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaScript::luaInputs_}, [this](auto) {
		// Only normal tag dirty here; don't set recreateStatus_
		tagDirty();
	});
}

std::string LuaScriptAdaptor::generateRamsesObjectName() const {
	std::string ramsesObjectName;

	if (parent_ && parent_->as<user_types::PrefabInstance>()) {
		ramsesObjectName = parent_->objectName() + ".";
	}

	ramsesObjectName += editorObject_->objectName();
	return ramsesObjectName;
}

bool LuaScriptAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	if (recreateStatus_) {
		auto scriptContent = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject_, &user_types::LuaScript::uri_}));
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "{}: {}", generateRamsesObjectName(), scriptContent);
		luaScript_.reset();
		if (!scriptContent.empty()) {
			auto ptr = sceneAdaptor_->logicEngine().createLuaScriptFromSource(scriptContent, generateRamsesObjectName());
			LOG_TRACE(log_system::RAMSES_ADAPTOR, "create: {}", fmt::ptr(ptr));
			if (ptr) {
				luaScript_ = {
					ptr,
					[this](auto* ptr) {
						auto success = sceneAdaptor_->logicEngine().destroy(*ptr);
						LOG_TRACE(log_system::RAMSES_ADAPTOR, "destroy: {} ({})", fmt::ptr(ptr), success);
						assert(success);
					}};
			} else {
				LOG_WARNING_IF(log_system::RAMSES_ADAPTOR, "Script creation failed: {}", LogicEngineErrors{sceneAdaptor_->logicEngine()});
			}
		}
	}

	if (luaScript_) {
		core::ValueHandle luaInputs{editorObject_, &user_types::LuaScript::luaInputs_};
		auto success = setLuaInputInEngine(luaScript_->getInputs(), luaInputs);
		LOG_WARNING_IF(log_system::RAMSES_ADAPTOR, !success, "Script set properties failed: {}", LogicEngineErrors{sceneAdaptor_->logicEngine()});
	}

	tagDirty(false);
	recreateStatus_ = false;
	return true;
}

void LuaScriptAdaptor::readDataFromEngine(core::DataChangeRecorder &recorder) {
	if (luaScript_) {
		core::ValueHandle luaOutputs{editorObject_, &user_types::LuaScript::luaOutputs_};
		getLuaOutputFromEngine(*luaScript_->getOutputs(), luaOutputs, recorder);
	}
}

const rlogic::Property* LuaScriptAdaptor::getProperty(const std::vector<std::string>& names) {
	const rlogic::Property* prop{names.at(0) == "luaInputs" ? luaScript_->getInputs() : luaScript_->getOutputs()};
	for (size_t i{1}; i < names.size(); i++) {
		if ( prop->getType()==rlogic::EPropertyType::Array) {
			// convert 1-bases Editor index back to 0-based index
			prop = prop->getChild(std::stoi(names.at(i))-1);
		} else {
			prop = prop->getChild(names.at(i));
		}
	}
	return prop;
}

void LuaScriptAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{ baseEditorObject() };
	if(errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR, level, valueHandle, message);
}

}  // namespace raco::ramses_adaptor
