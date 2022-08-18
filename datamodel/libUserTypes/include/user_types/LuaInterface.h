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

#include <map>

namespace raco::user_types {

class LuaInterface : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = { "LuaInterface", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	LuaInterface(LuaInterface const& other) : BaseObject(other), uri_(other.uri_), inputs_(other.inputs_) {
		fillPropertyDescription();
	}

	LuaInterface(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("inputs", &inputs_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	
	void updateFromExternalFile(BaseContext& context) override;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Lua interface files(*.lua);; All files (*.*)"}, DisplayNameAnnotation("URI")};

	Property<Table, DisplayNameAnnotation, LinkStartAnnotation, LinkEndAnnotation> inputs_{{}, DisplayNameAnnotation("Inputs"), {}, {}};

private:
	OutdatedPropertiesStore cachedLuaInputValues_;
};

using SLuaInterface = std::shared_ptr<LuaInterface>;

}
