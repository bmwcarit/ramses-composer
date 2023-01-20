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

#include "user_types/Skin.h"


namespace raco::ramses_adaptor {

class SkinAdaptor : public UserTypeObjectAdaptor<user_types::Skin> {
public:
	SkinAdaptor(SceneAdaptor *sceneAdaptor, raco::user_types::SSkin skin);

	bool sync(core::Errors* errors) override;

	std::vector<ExportInformation> getExportInformation() const override;

private:
	std::vector<raco::ramses_base::RamsesSkinBinding> skinBindings_;

	raco::components::Subscription dirtySubscription_;
	std::array<raco::components::Subscription, 4> subscriptions_;
};

}  // namespace raco::ramses_adaptor
