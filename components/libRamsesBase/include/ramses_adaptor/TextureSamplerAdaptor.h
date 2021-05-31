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
#include "user_types/Texture.h"
#include <memory>
#include <ramses-client-api/Texture2D.h>
#include <ramses-client-api/TextureSampler.h>

namespace raco::ramses_adaptor {

class TextureSamplerAdaptor : public TypedObjectAdaptor<user_types::Texture, ramses::TextureSampler> {
public:
	explicit TextureSamplerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::Texture> editorObject);

	bool sync(core::Errors* errors) override;

	static std::vector<unsigned char>* getFallbackTextureData(int mode);

private:
	ramses_base::RamsesTexture2D createTexture();
	ramses_base::RamsesTexture2D getFallbackTexture();

	std::array<components::Subscription, 6> subscriptions_;
	ramses_base::RamsesTexture2D textureData_;

	static inline std::vector<unsigned char> fallbackTextureData_[2];
	std::string createDefaultTextureDataName();
};

};  // namespace raco::ramses_adaptor