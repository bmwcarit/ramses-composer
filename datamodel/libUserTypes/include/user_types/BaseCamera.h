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

#include "user_types/Node.h"

namespace raco::user_types {

class BaseCamera : public Node {
	Property<double> step_;

public:
	static inline const TypeDescriptor typeDescription = {"BaseCamera", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	BaseCamera(const std::string& name, const std::string& id) : Node(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("viewPortOffsetX", &viewportOffsetX_);
		properties_.emplace_back("viewPortOffsetY", &viewportOffsetY_);
		properties_.emplace_back("viewPortWidth", &viewportWidth_);
		properties_.emplace_back("viewPortHeight", &viewportHeight_);
	}

	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> viewportOffsetX_{ 0, { -7680, 7680 }, { "Viewport Offset X" }, {} };
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> viewportOffsetY_{ 0, { -7680, 7680 }, { "Viewport Offset Y" }, {} };
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> viewportWidth_{ 1440, { 0, 7680 }, { "Viewport Width" }, {} };
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> viewportHeight_{ 720, { 0, 7680 }, { "Viewport Height" }, {} };
};

using SBaseCamera = std::shared_ptr<BaseCamera>;

}  // namespace raco::user_types
