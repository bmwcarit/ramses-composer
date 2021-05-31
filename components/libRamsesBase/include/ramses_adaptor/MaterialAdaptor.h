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

#include "components/DataChangeDispatcher.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "user_types/Material.h"
#include <array>

namespace raco::ramses_adaptor {

class SceneAdaptor;

class MaterialAdaptor final : public TypedObjectAdaptor<user_types::Material, ramses::Effect> {
private:
	static raco::ramses_base::RamsesEffect createEffect(SceneAdaptor* buildContext);

public:
	explicit MaterialAdaptor(SceneAdaptor* buildContext, user_types::SMaterial material);
	bool isValid();

	bool sync(core::Errors* errors) override;

private:
	components::Subscription subscription_;
};

};	// namespace raco::ramses_adaptor
