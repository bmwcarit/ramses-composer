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

	
class CameraViewport : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = {"CameraViewport", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	CameraViewport() : ClassWithReflectedMembers(getProperties()) {}

	CameraViewport(const CameraViewport& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: ClassWithReflectedMembers(getProperties()),
		  offsetX_(other.offsetX_),
		  offsetY_(other.offsetY_),
		  width_(other.width_),
		  height_(other.height_) {
	}

	CameraViewport& operator=(const CameraViewport& other) {
		offsetX_ = other.offsetX_;
		offsetY_ = other.offsetY_;
		width_ = other.width_;
		height_ = other.height_;
		return *this;
	}

	void copyAnnotationData(const CameraViewport& other) {
		offsetX_.copyAnnotationData(other.offsetX_);
		offsetY_.copyAnnotationData(other.offsetY_);
		width_.copyAnnotationData(other.width_);
		height_.copyAnnotationData(other.height_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"offsetX", &offsetX_},
			{"offsetY", &offsetY_},
			{"width", &width_},
			{"height", &height_}};
	}
	
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> offsetX_{0, {-7680, 7680}, {"Offset X"}, {}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> offsetY_{0, {-7680, 7680}, {"Offset Y"}, {}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> width_{1440, {0, 7680}, {"Width"}, {}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation> height_{720, {0, 7680}, {"Height"}, {}};
};

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
		properties_.emplace_back("viewport", &viewport_);
	}

	Property<CameraViewport, DisplayNameAnnotation, LinkEndAnnotation> viewport_{{}, {"Viewport"}, {}};
};

using SBaseCamera = std::shared_ptr<BaseCamera>;

}  // namespace raco::user_types
