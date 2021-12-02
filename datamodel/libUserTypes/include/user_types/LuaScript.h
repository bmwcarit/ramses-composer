/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "user_types/BaseObject.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include "core/FileChangeMonitor.h"
#include <map>

namespace raco::user_types {

class LuaScript : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "LuaScript", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	LuaScript(LuaScript const& other) : BaseObject(other), uri_(other.uri_), luaInputs_(other.luaInputs_), luaOutputs_(other.luaOutputs_) {
		fillPropertyDescription();
	}

	LuaScript(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("luaInputs", &luaInputs_);
		properties_.emplace_back("luaOutputs", &luaOutputs_);
	}

	void onBeforeDeleteObject(Errors& errors) const override;

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Lua script files(*.lua)"}, DisplayNameAnnotation("URI")};

	Property<Table, DisplayNameAnnotation> luaInputs_ {{}, DisplayNameAnnotation("Inputs")};
	Property<Table, DisplayNameAnnotation> luaOutputs_{{}, DisplayNameAnnotation("Outputs")};

private:

	void syncLuaInterface(BaseContext& context);

	mutable FileChangeMonitor::UniqueListener uriListener_;
	OutdatedPropertiesStore cachedLuaInputValues_;

	std::string currentScriptContents_;
	bool currentScriptValid_ = false;
};

using SLuaScript = std::shared_ptr<LuaScript>;

}
