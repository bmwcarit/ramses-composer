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

#include "components/DataChangeDispatcher.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/BlitPass.h"

namespace raco::ramses_adaptor {

class SceneAdaptor;

class BlitPassAdaptor final : public UserTypeObjectAdaptor<user_types::BlitPass> {
public:
	BlitPassAdaptor(SceneAdaptor* sceneAdaptor, user_types::SBlitPass blitPass);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;

private:
	std::array<components::Subscription, 13> subscriptions_;

	raco::ramses_base::RamsesBlitPass blitPass_;
	bool recreate_;
	bool update_;
};

};	// namespace raco::ramses_adaptor
