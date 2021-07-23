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
#include "ramses_adaptor/NodeAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/OrthographicCamera.h"
#include <memory>
#include <ramses-client-api/OrthographicCamera.h>

namespace raco::ramses_adaptor {

class OrthographicCameraAdaptor : public SpatialAdaptor<user_types::OrthographicCamera, ramses::OrthographicCamera> {
public:
	explicit OrthographicCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::OrthographicCamera> editorObject);
	~OrthographicCameraAdaptor() override;

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override;
	bool sync(core::Errors* errors) override;

	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override;

private:
	components::Subscription viewportSubscription_;
	components::Subscription frustrumSubscription_;
	raco::ramses_base::UniqueRamsesCameraBinding cameraBinding_;
};

};	// namespace raco::ramses_adaptor
