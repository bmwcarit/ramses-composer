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
#include "user_types/TextureExternal.h"

#include <ramses/client/TextureSamplerExternal.h>

#include <memory>

namespace raco::ramses_adaptor {

class TextureExternalAdaptor : public TypedObjectAdaptor<user_types::TextureExternal, ramses::TextureSamplerExternal> {
public:
	explicit TextureExternalAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::TextureExternal> editorObject);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;

private:
	std::array<components::Subscription, 2> subscriptions_;
};

};  // namespace raco::ramses_adaptor