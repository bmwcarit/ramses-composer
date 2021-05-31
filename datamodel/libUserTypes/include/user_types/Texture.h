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

#include "user_types/BaseObject.h"
#include "core/FileChangeMonitor.h"
#include "user_types/BaseTexture.h"

namespace raco::user_types {

class Texture : public BaseTexture {
public:
	static inline const TypeDescriptor typeDescription = {"Texture", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Texture(Texture const& other)
		: BaseTexture(other), uri_(other.uri_), origin_(other.origin_) {
		fillPropertyDescription();
	}

	Texture(const std::string& name, const std::string& id) : BaseTexture(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uri", &uri_);
		properties_.emplace_back("origin", &origin_);
	}

    void onBeforeDeleteObject(Errors& errors) const override;

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;


	Property<std::string, URIAnnotation, DisplayNameAnnotation> uri_{std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation("URI")};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> origin_{DEFAULT_VALUE_TEXTURE_ORIGIN_BOTTOM, DisplayNameAnnotation("U/V Origin"), EnumerationAnnotation{EngineEnumeration::TextureOrigin}};

private:
	mutable FileChangeMonitor::UniqueListener uriListener_;
};

using STexture = std::shared_ptr<Texture>;

}  // namespace raco::user_types
