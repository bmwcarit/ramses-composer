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
#include "user_types/RenderPass.h"
#include <ramses-client-api/RenderPass.h>

namespace raco::ramses_adaptor {

class RenderPassAdaptor : public TypedObjectAdaptor<user_types::RenderPass, ramses_base::RamsesRenderPassHandle>, public ILogicPropertyProvider {
public:
	explicit RenderPassAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderPass> editorObject);

	bool sync(core::Errors* errors) override;

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override;

private:
	raco::ramses_base::UniqueRamsesRenderPassBinding binding_;

	std::array<components::Subscription, 17> subscriptions_;
};

};	// namespace raco::ramses_adaptor
