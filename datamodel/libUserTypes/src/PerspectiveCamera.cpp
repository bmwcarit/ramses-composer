/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/PerspectiveCamera.h"

#include <memory>

namespace raco::user_types {

void PerspectiveCamera::onAfterContextActivated(BaseContext& context) {
	syncFrustum(context);
}

void PerspectiveCamera::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&PerspectiveCamera::frustumType_)) {
		syncFrustum(context);
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

void PerspectiveCamera::syncFrustum(BaseContext& context) {
	ValueHandle frustumHandle{shared_from_this(), &PerspectiveCamera::frustum_};
	if (!frustum_->hasProperty("nearPlane")) {
		std::unique_ptr<data_storage::ValueBase> near = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(0.1, DisplayNameAnnotation("nearPlane"), RangeAnnotation<double>(0.1, 1.0), LinkEndAnnotation());
		context.addProperty(frustumHandle, "nearPlane", std::move(near));
	}
	if (!frustum_->hasProperty("farPlane")) {
		std::unique_ptr<data_storage::ValueBase> far = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(1000.0, DisplayNameAnnotation("farPlane"), RangeAnnotation<double>(100.0, 10000.0), LinkEndAnnotation());
		context.addProperty(frustumHandle, "farPlane", std::move(far));
	}

	if (*frustumType_ == static_cast<int>(EFrustumType::Aspect_FieldOfView)) {
		// aspect & fov
		if (frustum_->hasProperty("leftPlane")) {
			cachedFrustumValues_[std::make_pair("leftPlane", EnginePrimitive::Double)] = frustum_->get("leftPlane")->clone({});
			context.removeProperty(frustumHandle, "leftPlane");
		}
		if (frustum_->hasProperty("rightPlane")) {
			cachedFrustumValues_[std::make_pair("rightPlane", EnginePrimitive::Double)] = frustum_->get("rightPlane")->clone({});
			context.removeProperty(frustumHandle, "rightPlane");
		}
		if (frustum_->hasProperty("topPlane")) {
			cachedFrustumValues_[std::make_pair("topPlane", EnginePrimitive::Double)] = frustum_->get("topPlane")->clone({});
			context.removeProperty(frustumHandle, "topPlane");
		}
		if (frustum_->hasProperty("bottomPlane")) {
			cachedFrustumValues_[std::make_pair("bottomPlane", EnginePrimitive::Double)] = frustum_->get("bottomPlane")->clone({});
			context.removeProperty(frustumHandle, "bottomPlane");
		}
		if (!frustum_->hasProperty("fieldOfView")) {
			std::unique_ptr<data_storage::ValueBase> fov = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(35.0, DisplayNameAnnotation("fieldOfView"), RangeAnnotation<double>(10.0, 120.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"fieldOfView", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*fov = *it->second;
			}
			context.addProperty(frustumHandle, "fieldOfView", std::move(fov));
		}
		if (!frustum_->hasProperty("aspectRatio")) {
			std::unique_ptr<data_storage::ValueBase> aspect = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(1440.0 / 720.0, DisplayNameAnnotation("aspectRatio"), RangeAnnotation<double>(0.5, 4.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"aspectRatio", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*aspect = *it->second;
			}
			context.addProperty(frustumHandle, "aspectRatio", std::move(aspect));
		}
	} else {
		// planes
		if (frustum_->hasProperty("fieldOfView")) {
			cachedFrustumValues_[std::make_pair("fieldOfView", EnginePrimitive::Double)] = frustum_->get("fieldOfView")->clone({});
			context.removeProperty(frustumHandle, "fieldOfView");
		}
		if (frustum_->hasProperty("aspectRatio")) {
			cachedFrustumValues_[std::make_pair("aspectRatio", EnginePrimitive::Double)] = frustum_->get("aspectRatio")->clone({});
			context.removeProperty(frustumHandle, "aspectRatio");
		}
		if (!frustum_->hasProperty("leftPlane")) {
			std::unique_ptr<data_storage::ValueBase> left = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(-10.0, DisplayNameAnnotation("leftPlane"), RangeAnnotation<double>(-1000.0, 0.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"leftPlane", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*left = *it->second;
			}
			context.addProperty(frustumHandle, "leftPlane", std::move(left));
		}
		if (!frustum_->hasProperty("rightPlane")) {
			std::unique_ptr<data_storage::ValueBase> right = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(10.0, DisplayNameAnnotation("rightPlane"), RangeAnnotation<double>(0.0, 1000.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"rightPlane", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*right = *it->second;
			}
			context.addProperty(frustumHandle, "rightPlane", std::move(right));
		}
		if (!frustum_->hasProperty("bottomPlane")) {
			std::unique_ptr<data_storage::ValueBase> bottom = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(-10.0, DisplayNameAnnotation("bottomPlane"), RangeAnnotation<double>(-1000.0, 0.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"bottomPlane", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*bottom = *it->second;
			}
			context.addProperty(frustumHandle, "bottomPlane", std::move(bottom));
		}
		if (!frustum_->hasProperty("topPlane")) {
			std::unique_ptr<data_storage::ValueBase> top = std::make_unique<Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>>(10.0, DisplayNameAnnotation("topPlane"), RangeAnnotation<double>(0.0, 1000.0), LinkEndAnnotation());
			auto it = cachedFrustumValues_.find({"topPlane", EnginePrimitive::Double});
			if (it != cachedFrustumValues_.end()) {
				*top = *it->second;
			}
			context.addProperty(frustumHandle, "topPlane", std::move(top));
		}
	}
}

}  // namespace raco::user_types