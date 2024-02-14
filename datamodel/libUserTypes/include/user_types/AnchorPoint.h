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

#include "user_types/BaseCamera.h"

namespace raco::user_types {

class AnchorPointOutputs : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"AnchorPointOutputs", false};

	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	bool serializationRequired() const override {
		return true;
	}

	AnchorPointOutputs(const AnchorPointOutputs& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(),
		  viewportCoords_(other.viewportCoords_),
		  depth_(other.depth_) {
		fillPropertyDescription();
	}

	AnchorPointOutputs() : StructBase() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("viewportCoords", &viewportCoords_);
		properties_.emplace_back("depth", &depth_);
	}

	void copyAnnotationData(const AnchorPointOutputs& other) {
		viewportCoords_.copyAnnotationData(other.viewportCoords_);
		depth_.copyAnnotationData(other.depth_);
	}

	AnchorPointOutputs& operator=(const AnchorPointOutputs& other) {
		viewportCoords_ = other.viewportCoords_;
		depth_ = other.depth_;
		return *this;
	}

	Property<Vec2f, DisplayNameAnnotation, LinkStartAnnotation> viewportCoords_{{}, {"Viewport Coordinates"}, {}};
	Property<double, DisplayNameAnnotation, LinkStartAnnotation> depth_{{}, {"Depth"}, {}};
};

class AnchorPoint : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"AnchorPoint", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	AnchorPoint(const AnchorPoint& other) : BaseObject(other) {
		fillPropertyDescription();
	}

	AnchorPoint(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("node", &node_);
		properties_.emplace_back("camera", &camera_);
		properties_.emplace_back("outputs", &outputs_);
	}

	Property<SNode, DisplayNameAnnotation> node_{{}, {"Node"}};
	Property<SBaseCamera, DisplayNameAnnotation> camera_{{}, {"Camera"}};

	Property<AnchorPointOutputs, DisplayNameAnnotation> outputs_{{}, {"Outputs"}};
};

using SAnchorPoint = std::shared_ptr<AnchorPoint>;

}  // namespace raco::user_types