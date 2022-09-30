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
#include "user_types/LuaScript.h"

#include <ramses-logic/LuaScript.h>

#include <memory>
#include <vector>

namespace raco::ramses_adaptor {

class LuaScriptAdaptor : public UserTypeObjectAdaptor<user_types::LuaScript>, public ILogicPropertyProvider {
public:
	explicit LuaScriptAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaScript> editorObject);
	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;

	bool sync(core::Errors* errors) override;
	void readDataFromEngine(core::DataChangeRecorder &recorder);
	std::vector<ExportInformation> getExportInformation() const override;

private:
	void setupParentSubscription();
	void setupInputValuesSubscription();
	std::string generateRamsesObjectName() const;

	rlogic::LuaScript* rlogicLuaScript() const {
		return luaScript_.get();
	}

	ramses_base::RamsesLuaScript luaScript_;

	components::Subscription subscription_;
	components::Subscription nameSubscription_;
	components::Subscription inputSubscription_;
	components::Subscription childrenSubscription_;
	components::Subscription parentNameSubscription_;
	components::Subscription stdModuleSubscription_;
	components::Subscription moduleSubscription_;


	// Flag to keep track if a change needs to recreate the lua script in the logicengine
	// or if it is sufficient to just update the input properties.
	bool recreateStatus_ = true;
	SEditorObject parent_;
};

};	// namespace raco::ramses_adaptor
