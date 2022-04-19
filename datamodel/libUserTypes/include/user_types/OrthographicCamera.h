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

class OrthographicFrustum : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"OrthographicFrustum", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	OrthographicFrustum() : StructBase(getProperties()) {}

	OrthographicFrustum(const OrthographicFrustum& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  near_(other.near_),
		  far_(other.far_),
		  left_(other.left_),
		  right_(other.right_),
		  bottom_(other.bottom_),
		  top_(other.top_) {
	}

	OrthographicFrustum& operator=(const OrthographicFrustum& other) {
		near_ = other.near_;
		far_ = other.far_;
		left_ = other.left_;
		right_ = other.right_;
		bottom_ = other.bottom_;
		top_ = other.top_;
		return *this;
	}

	void copyAnnotationData(const OrthographicFrustum& other) {
		near_.copyAnnotationData(other.near_);
		far_.copyAnnotationData(other.far_);
		left_.copyAnnotationData(other.left_);
		right_.copyAnnotationData(other.right_);
		bottom_.copyAnnotationData(other.bottom_);
		top_.copyAnnotationData(other.top_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"nearPlane", &near_},
			{"farPlane", &far_},
			{"leftPlane", &left_},
			{"rightPlane", &right_},
			{"bottomPlane", &bottom_},
			{"topPlane", &top_}};
	}

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> near_{0.1, DisplayNameAnnotation("nearPlane"), RangeAnnotation<double>(0.1, 1.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> far_{1000.0, DisplayNameAnnotation("farPlane"), RangeAnnotation<double>(100.0, 10000.0), {}};

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> left_{-10.0, DisplayNameAnnotation("leftPlane"), RangeAnnotation<double>(-1000.0, 0.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> right_{10.0, DisplayNameAnnotation("rightPlane"), RangeAnnotation<double>(0.0, 1000.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> bottom_{-10.0, DisplayNameAnnotation("bottomPlane"), RangeAnnotation<double>(-1000.0, 0.0), {}};
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation> top_{10.0, DisplayNameAnnotation("topPlane"), RangeAnnotation<double>(0.0, 1000.0), {}};
};

class OrthographicCamera : public BaseCamera {
public:
	static inline const TypeDescriptor typeDescription = {"OrthographicCamera", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	OrthographicCamera(OrthographicCamera const& other)
		: BaseCamera(other), frustum_(other.frustum_) {
		fillPropertyDescription();
	}

	OrthographicCamera(const std::string& name, const std::string& id) : BaseCamera(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("frustum", &frustum_);
	}

	Property<OrthographicFrustum, DisplayNameAnnotation, LinkEndAnnotation> frustum_{{}, {"Frustum"}, {}};
};

}  // namespace raco::user_types