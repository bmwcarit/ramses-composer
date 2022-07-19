/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/LuaScriptModuleAdaptor.h"

#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/LogicEngineFormatter.h"
#include "user_types/PrefabInstance.h"
#include "utils/FileUtils.h"

namespace raco::ramses_adaptor {

LuaScriptModuleAdaptor::LuaScriptModuleAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SLuaScriptModule editorObject)
	: UserTypeObjectAdaptor{sceneAdaptor, editorObject},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::LuaScriptModule::objectName_}, [this]() {
		  tagDirty();
	  })},
	  stdModuleSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject_, &user_types::LuaScriptModule::stdModules_}, [this](auto) {
		  tagDirty();
	  })},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  tagDirty();
	  })} {
}

bool LuaScriptModuleAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	if (editorObject_->isValid()) {
		const auto& scriptContents = editorObject_->currentScriptContents();
		auto luaConfig = raco::ramses_base::createLuaConfig(editorObject_->stdModules_->activeModules());
		module_ = raco::ramses_base::ramsesLuaModule(scriptContents, &sceneAdaptor_->logicEngine(), luaConfig, editorObject_->objectName(), editorObject_->objectIDAsRamsesLogicID());
		assert(module_ != nullptr);
	} else {
		module_.reset();
	}

	tagDirty(false);
	return true;
}

ramses_base::RamsesLuaModule LuaScriptModuleAdaptor::module() const {
	return module_;
}

}  // namespace raco::ramses_adaptor
