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

#include <map>
#include <set>

namespace raco::user_types {

class LuaScriptModule : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "LuaScriptModule", true };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	static inline const std::set<std::string> LUA_STANDARD_MODULES = {
		"string",
		"table",
		"math",
		"debug"};

	LuaScriptModule(LuaScriptModule const& other) : BaseObject(other), uri_(other.uri_) {
		fillPropertyDescription();
	}

	LuaScriptModule(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
	}

	void updateFromExternalFile(BaseContext& context) override;


	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Lua script files(*.lua)"}, DisplayNameAnnotation("URI")};

	std::string currentScriptContents_;
private:

	void syncLuaInterface(BaseContext& context);
};

using SLuaScriptModule = std::shared_ptr<LuaScriptModule>;

}
