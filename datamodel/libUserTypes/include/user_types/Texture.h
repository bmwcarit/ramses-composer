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

#include "user_types/BaseObject.h"
#include "user_types/BaseTexture.h"

namespace raco::user_types {

class Texture : public TextureSampler2DBase {
public:
	static inline const TypeDescriptor typeDescription = {"Texture", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Texture(Texture const& other)
		: TextureSampler2DBase(other), uri_(other.uri_), flipTexture_(other.flipTexture_), generateMipmaps_(other.generateMipmaps_), textureFormat_(other.textureFormat_) {
		fillPropertyDescription();
	}

	Texture(const std::string& name, const std::string& id) : TextureSampler2DBase(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("textureFormat", &textureFormat_);
		properties_.emplace_back("flipTexture", &flipTexture_);
		properties_.emplace_back("generateMipmaps", &generateMipmaps_);
		properties_.emplace_back("mipmapLevel", &mipmapLevel_);
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("level2uri", &level2uri_);
		properties_.emplace_back("level3uri", &level3uri_);
		properties_.emplace_back("level4uri", &level4uri_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void updateFromExternalFile(BaseContext& context) override;


	Property<int, DisplayNameAnnotation, EnumerationAnnotation> textureFormat_{DEFAULT_VALUE_TEXTURE_FORMAT_RGBA8, {"Format"}, {EUserTypeEnumerations::TextureFormat}};
	Property<bool, DisplayNameAnnotation> flipTexture_{false, DisplayNameAnnotation("Flip Vertically")};
	Property<bool, DisplayNameAnnotation> generateMipmaps_{false, DisplayNameAnnotation("Generate Mipmaps")};

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> mipmapLevel_{1, {"Mipmap Level"}, {1, 4}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation("Level 1 URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uri_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uri_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uri_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI"}};

private:
	void validateURIs(BaseContext& context);
	void validateMipmapLevel(BaseContext& context);
};

using STexture = std::shared_ptr<Texture>;

}  // namespace raco::user_types
