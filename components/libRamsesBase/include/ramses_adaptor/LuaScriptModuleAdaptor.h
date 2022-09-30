/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/Handles.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/LuaScriptModule.h"

#include <ramses-logic/LuaScript.h>

#include <memory>
#include <vector>

namespace raco::ramses_adaptor {

class LuaScriptModuleAdaptor : public UserTypeObjectAdaptor<user_types::LuaScriptModule> {
public:
	explicit LuaScriptModuleAdaptor(SceneAdaptor* sceneAdaptor, raco::user_types::SLuaScriptModule editorObject);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;

	ramses_base::RamsesLuaModule module() const;

private:
	ramses_base::RamsesLuaModule module_;
	
	components::Subscription subscription_;
	components::Subscription nameSubscription_;
	components::Subscription stdModuleSubscription_;
};

};	// namespace raco::ramses_adaptor
