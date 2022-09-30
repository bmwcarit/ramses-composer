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

#include "ramses_adaptor/ObjectAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/LuaInterface.h"

#include <ramses-logic/LuaInterface.h>

#include <memory>
#include <vector>

namespace raco::ramses_adaptor {

class LuaInterfaceAdaptor : public UserTypeObjectAdaptor<user_types::LuaInterface>, public ILogicPropertyProvider {
public:
	explicit LuaInterfaceAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::LuaInterface> editorObject);

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;
	std::vector<ExportInformation> getExportInformation() const override;

	bool sync(core::Errors* errors) override;

private:
	void setupParentSubscription();
	std::string generateRamsesObjectName() const;

	ramses_base::RamsesLuaInterface ramsesInterface_;

	components::Subscription subscription_;
	components::Subscription nameSubscription_;
	components::Subscription inputSubscription_;
	components::Subscription childrenSubscription_;
	components::Subscription parentNameSubscription_;
	components::Subscription linksLifecycleSubscription_;
	components::Subscription linkValidityChangeSubscription_;

	// Flag to keep track if a change needs to recreate the lua script in the logicengine
	// or if it is sufficient to just update the input properties.
	bool recreateStatus_ = true;
	SEditorObject parent_;
};

};	// namespace raco::ramses_adaptor
