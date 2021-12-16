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
	: ObjectAdaptor{sceneAdaptor},
	  editorObject_{editorObject},
	  nameSubscription_{sceneAdaptor_->dispatcher()->registerOn({editorObject_, &user_types::LuaScriptModule::objectName_}, [this]() {
		  tagDirty();
	  })},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject_, [this]() {
		  tagDirty();
	  })} {
}

raco::user_types::SEditorObject LuaScriptModuleAdaptor::baseEditorObject() noexcept {
	return editorObject_;
}
const raco::user_types::SEditorObject LuaScriptModuleAdaptor::baseEditorObject() const noexcept {
	return editorObject_;
}

bool LuaScriptModuleAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	const auto& scriptContents = editorObject_->currentScriptContents_;
	if (scriptContents.empty()) {
		module_.reset();
	} else {
		module_ = raco::ramses_base::ramsesLuaModule(scriptContents, &sceneAdaptor_->logicEngine(), editorObject_->objectName());
	}

	tagDirty(false);
	return true;
}

}  // namespace raco::ramses_adaptor
