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
#include "user_types/Texture.h"
#include <memory>
#include <ramses/client/Texture2D.h>
#include <ramses/client/TextureSampler.h>

namespace raco::ramses_adaptor {

class TextureSamplerAdaptor : public TypedObjectAdaptor<user_types::Texture, ramses::TextureSampler> {
public:
	explicit TextureSamplerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::Texture> editorObject);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;
	static void flipDecodedPicture(std::vector<unsigned char>& rawPictureData, unsigned int availableChannels, unsigned int width, unsigned int height, unsigned int bitdepth);

private:
	std::array<components::Subscription, 10> subscriptions_;
	ramses_base::RamsesTexture2D textureData_;

	ramses_base::RamsesTexture2D createTexture(core::Errors* errors, ramses_base::PngDecodingInfo &decodingInfo);
	std::string createDefaultTextureDataName();

};

};  // namespace raco::ramses_adaptor