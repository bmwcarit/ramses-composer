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
        , uriFront_(other.uriFront_)
        , uriBack_(other.uriBack_)
        , uriLeft_(other.uriLeft_)
        , uriRight_(other.uriRight_)
        , uriTop_(other.uriTop_)
        , uriBottom_(other.uriBottom_)
    {
		fillPropertyDescription();
	}

	CubeMap(const std::string& name, const std::string& id) : BaseTexture(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uriFront", &uriFront_);
		properties_.emplace_back("uriBack", &uriBack_);
		properties_.emplace_back("uriLeft", &uriLeft_);
		properties_.emplace_back("uriRight", &uriRight_);
		properties_.emplace_back("uriTop", &uriTop_);
		properties_.emplace_back("uriBottom", &uriBottom_);
	}

	void updateFromExternalFile(BaseContext& context) override;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriFront_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI front"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriBack_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI back"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriLeft_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI left"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriRight_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI right"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriTop_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI top"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriBottom_{
		std::string{}, {"Image files(*.png)"}, DisplayNameAnnotation{"URI bottom"}};

};

using SCubeMap = std::shared_ptr<CubeMap>;

};  // namespace raco::user_types
