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
#include "user_types/SyncTableWithEngineInterface.h"
#include "user_types/LuaStandardModuleSelection.h"

#include <map>

namespace raco::user_types {

class LuaScript : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"LuaScript", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	LuaScript(LuaScript const& other) : BaseObject(other), uri_(other.uri_), stdModules_(other.stdModules_), luaModules_(other.luaModules_), inputs_(other.inputs_) {
		fillPropertyDescription();
	}

	LuaScript(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("stdModules", &stdModules_);
		properties_.emplace_back("luaModules", &luaModules_);
		properties_.emplace_back("inputs", &inputs_);
		properties_.emplace_back("outputs", &outputs_);
	}

	void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	void updateFromExternalFile(BaseContext& context) override;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Lua script files(*.lua);; All files (*.*)", core::PathManager::FolderTypeKeys::Script}, DisplayNameAnnotation("URI")};

	Property<LuaStandardModuleSelection, DisplayNameAnnotation> stdModules_{{}, {"Standard Modules"}};

	Property<Table, DisplayNameAnnotation> luaModules_{{}, DisplayNameAnnotation("Modules")};
	Property<Table, DisplayNameAnnotation, LinkEndAnnotation> inputs_{{}, DisplayNameAnnotation("Inputs"), {}};
	Property<Table, DisplayNameAnnotation> outputs_{{}, DisplayNameAnnotation("Outputs")};

private:
	void syncLuaScript(BaseContext& context, bool syncModules);

	OutdatedPropertiesStore cachedLuaInputValues_;

	// Map module name -> object id
	std::map<std::string, std::string> cachedModuleRefs_;
};

using SLuaScript = std::shared_ptr<LuaScript>;

}  // namespace raco::user_types
