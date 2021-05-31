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

#include "user_types/BaseCamera.h"

namespace raco::user_types {

class OrthographicCamera : public BaseCamera {
public:
	static inline const TypeDescriptor typeDescription = {"OrthographicCamera", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

    OrthographicCamera(OrthographicCamera const& other)
        : BaseCamera(other)
        , near_(other.near_)
        , far_(other.far_)
        , left_(other.left_)
        , right_(other.right_)
        , bottom_(other.bottom_)
        , top_(other.top_)
    {
		fillPropertyDescription();
	}

	OrthographicCamera(const std::string& name, const std::string& id) : BaseCamera(name, id) {
		fillPropertyDescription();
    }

    void fillPropertyDescription() {
		properties_.emplace_back("near", &near_);
		properties_.emplace_back("far", &far_);

        properties_.emplace_back("left", &left_);
		properties_.emplace_back("right", &right_);

        properties_.emplace_back("bottom", &bottom_);
        properties_.emplace_back("top", &top_);
	}

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> near_{0.1, DisplayNameAnnotation("Near Plane"), RangeAnnotation<double>(0.1, 1.0), {} };
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> far_{1000.0, DisplayNameAnnotation("Far Plane"), RangeAnnotation<double>(100.0, 10000.0), {} };

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> left_{-10.0, DisplayNameAnnotation("Left Plane"), RangeAnnotation<double>(-1000.0, 0.0), {} };
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> right_{10.0, DisplayNameAnnotation("Right Plane"), RangeAnnotation<double>(0.0, 1000.0), {} };
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> bottom_{-10.0, DisplayNameAnnotation("Bottom Plane"), RangeAnnotation<double>(-1000.0, 0.0), {} };
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> top_{10.0, DisplayNameAnnotation("Top Plane"), RangeAnnotation<double>(0.0, 1000.0), {}};
};

}  // namespace raco::user_types