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

#include "ramses_adaptor/NodeAdaptor.h"
#include "user_types/PerspectiveCamera.h"

#include <memory>
#include <vector>

namespace raco::ramses_adaptor {

class PerspectiveCameraAdaptor : public SpatialAdaptor<user_types::PerspectiveCamera, ramses_base::RamsesPerspectiveCameraHandle> {
public:
	
	explicit PerspectiveCameraAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::PerspectiveCamera> editorObject);
	~PerspectiveCameraAdaptor() override;

	bool sync(core::Errors* errors) override;

	void getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const override;
	ramses::Property* getProperty(const std::vector<std::string_view>& propertyNamesVector) override;

	ramses_base::RamsesCameraBinding cameraBinding();
	std::vector<ExportInformation> getExportInformation() const override;

private:
	components::Subscription viewportSubscription_;
	components::Subscription frustrumSubscription_;
	ramses_base::RamsesCameraBinding cameraBinding_;
};

};  // namespace raco::ramses_adaptor
