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

#include "core/Handles.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/LuaScript.h"

#include <ramses-logic/LuaScript.h>

#include <memory>
#include <vector>

namespace raco::ramses_adaptor {

class LuaScriptAdaptor : public ObjectAdaptor, public ILogicPropertyProvider {
public:
	explicit LuaScriptAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaScript> editorObject);
	SEditorObject baseEditorObject() noexcept override;
	const SEditorObject baseEditorObject() const noexcept override;
	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;

	bool sync(core::Errors* errors) override;
	void readDataFromEngine(core::DataChangeRecorder &recorder); 

private:
	void setupParentSubscription();
	void setupInputValuesSubscription();
	std::string generateRamsesObjectName() const;

	rlogic::LuaScript* rlogicLuaScript() const {
		return luaScript_.get();
	}

	std::shared_ptr<user_types::LuaScript> editorObject_;
	
	std::vector<raco::ramses_base::RamsesLuaModule> modules;
	std::unique_ptr<rlogic::LuaScript, std::function<void(rlogic::LuaScript*)>> luaScript_{nullptr, [](auto) {}};
	components::Subscription subscription_;
	components::Subscription nameSubscription_;
	components::Subscription inputSubscription_;
	components::Subscription childrenSubscription_;
	components::Subscription parentNameSubscription_;
	components::Subscription moduleSubscription_;


	// Flag to keep track if a change needs to recreate the lua script in the logicengine
	// or if it is sufficient to just update the input properties.
	bool recreateStatus_ = true;
	SEditorObject parent_;
};

};	// namespace raco::ramses_adaptor
