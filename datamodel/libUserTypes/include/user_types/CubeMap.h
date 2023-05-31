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

#include "user_types/BaseTexture.h"

namespace raco::user_types {

class CubeMap : public BaseTexture {
public:
	static inline const TypeDescriptor typeDescription = {"CubeMap", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	CubeMap(CubeMap const& other)
		: BaseTexture(other)
		, textureFormat_(other.textureFormat_)
		, generateMipmaps_(other.generateMipmaps_)
		, mipmapLevel_(other.mipmapLevel_)
        , uriFront_(other.uriFront_)
        , uriBack_(other.uriBack_)
        , uriLeft_(other.uriLeft_)
        , uriRight_(other.uriRight_)
        , uriTop_(other.uriTop_)
        , uriBottom_(other.uriBottom_)
		, level2uriFront_(other.level2uriFront_)
        , level2uriBack_(other.level2uriBack_)
        , level2uriLeft_(other.level2uriLeft_)
        , level2uriRight_(other.level2uriRight_)
        , level2uriTop_(other.level2uriTop_)
        , level2uriBottom_(other.level2uriBottom_)
		, level3uriFront_(other.level3uriFront_)
        , level3uriBack_(other.level3uriBack_)
        , level3uriLeft_(other.level3uriLeft_)
        , level3uriRight_(other.level3uriRight_)
        , level3uriTop_(other.level3uriTop_)
        , level3uriBottom_(other.level3uriBottom_)
		, level4uriFront_(other.level4uriFront_)
        , level4uriBack_(other.level4uriBack_)
        , level4uriLeft_(other.level4uriLeft_)
        , level4uriRight_(other.level4uriRight_)
        , level4uriTop_(other.level4uriTop_)
        , level4uriBottom_(other.level4uriBottom_)
    {
		fillPropertyDescription();
	}

	CubeMap(const std::string& name, const std::string& id) : BaseTexture(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("textureFormat", &textureFormat_);
		properties_.emplace_back("generateMipmaps", &generateMipmaps_);
		properties_.emplace_back("mipmapLevel", &mipmapLevel_);
		properties_.emplace_back("uriRight", &uriRight_);
		properties_.emplace_back("uriLeft", &uriLeft_);
		properties_.emplace_back("uriTop", &uriTop_);
		properties_.emplace_back("uriBottom", &uriBottom_);
		properties_.emplace_back("uriFront", &uriFront_);
		properties_.emplace_back("uriBack", &uriBack_);
		properties_.emplace_back("level2uriRight", &level2uriRight_);
		properties_.emplace_back("level2uriLeft", &level2uriLeft_);
		properties_.emplace_back("level2uriTop", &level2uriTop_);
		properties_.emplace_back("level2uriBottom", &level2uriBottom_);
		properties_.emplace_back("level2uriFront", &level2uriFront_);
		properties_.emplace_back("level2uriBack", &level2uriBack_);
		properties_.emplace_back("level3uriRight", &level3uriRight_);
		properties_.emplace_back("level3uriLeft", &level3uriLeft_);
		properties_.emplace_back("level3uriTop", &level3uriTop_);
		properties_.emplace_back("level3uriBottom", &level3uriBottom_);
		properties_.emplace_back("level3uriFront", &level3uriFront_);
		properties_.emplace_back("level3uriBack", &level3uriBack_);
		properties_.emplace_back("level4uriRight", &level4uriRight_);
		properties_.emplace_back("level4uriLeft", &level4uriLeft_);
		properties_.emplace_back("level4uriTop", &level4uriTop_);
		properties_.emplace_back("level4uriBottom", &level4uriBottom_);
		properties_.emplace_back("level4uriFront", &level4uriFront_);
		properties_.emplace_back("level4uriBack", &level4uriBack_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void updateFromExternalFile(BaseContext& context) override;

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> textureFormat_{DEFAULT_VALUE_TEXTURE_FORMAT_RGBA8, {"Format"}, {EUserTypeEnumerations::TextureFormat}};

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> mipmapLevel_{1, {"Mipmap Level"}, {1, 4}};

	Property<bool, DisplayNameAnnotation> generateMipmaps_{false, DisplayNameAnnotation("Automatically Generate Mipmaps")};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriFront_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI +Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriBack_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI -Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriLeft_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI -X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriRight_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI +X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriTop_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI +Y"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriBottom_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 1 URI -Y"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriFront_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI +Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriBack_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI -Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriLeft_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI -X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriRight_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI +X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriTop_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI +Y"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level2uriBottom_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 2 URI -Y"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriFront_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI +Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriBack_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI -Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriLeft_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI -X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriRight_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI +X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriTop_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI +Y"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level3uriBottom_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 3 URI -Y"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriFront_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI +Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriBack_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI -Z"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriLeft_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI -X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriRight_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI +X"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriTop_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI +Y"}};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> level4uriBottom_{std::string{}, {"Image files(*.png);; All files (*.*)", core::PathManager::FolderTypeKeys::Image}, DisplayNameAnnotation{"Level 4 URI -Y"}};

private:
	void validateURIs(BaseContext& context);
	void validateMipmapLevel(BaseContext& context);
};

using SCubeMap = std::shared_ptr<CubeMap>;

};  // namespace raco::user_types
