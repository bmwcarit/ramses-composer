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

#include "user_types/BaseObject.h"
#include "user_types/LuaStandardModuleSelection.h"

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

	LuaScriptModule(LuaScriptModule const& other) : BaseObject(other), uri_(other.uri_), stdModules_(other.stdModules_) {
		fillPropertyDescription();
	}

	LuaScriptModule(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("stdModules", &stdModules_);
	}

	void updateFromExternalFile(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void onBeforeDeleteObject(BaseContext& context) const override;

	bool isValid() const;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Lua script files(*.lua);; All files (*.*)", core::PathManager::FolderTypeKeys::Script}, DisplayNameAnnotation("URI")};

	Property<LuaStandardModuleSelection, DisplayNameAnnotation> stdModules_{{}, {"Standard Modules"}};

	const std::string& currentScriptContents() const;

private:
	void sync(BaseContext& context);

	std::string currentScriptContents_;
	bool isValid_ = false;
};

using SLuaScriptModule = std::shared_ptr<LuaScriptModule>;

}
