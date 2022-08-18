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

#include "core/Handles.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/CubeMap.h"
#include <memory>
#include <ramses-client-api/TextureCube.h>
#include <ramses-client-api/TextureSampler.h>

namespace raco::ramses_adaptor {

class CubeMapAdaptor : public TypedObjectAdaptor<user_types::CubeMap, ramses::TextureSampler> {
public:
	explicit CubeMapAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::CubeMap> editorObject);

	bool sync(core::Errors* errors) override;

private:
	raco::ramses_base::RamsesTextureCube createTexture(core::Errors* errors);
	raco::ramses_base::RamsesTextureCube fallbackCube();
	std::string createDefaultTextureDataName();

	std::array<components::Subscription, 9> subscriptions_;
	raco::ramses_base::RamsesTextureCube textureData_;

	std::map<std::string, std::vector<unsigned char>> generateMipmapData(core::Errors* errors, int level, raco::ramses_base::PngDecodingInfo& decodingInfo);
};

};  // namespace raco::ramses_adaptor