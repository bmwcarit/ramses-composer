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
#include "user_types/Enumerations.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include "core/EngineInterface.h"

namespace raco::user_types {

class PerspectiveCamera : public BaseCamera {
public:
	static inline const TypeDescriptor typeDescription = {"PerspectiveCamera", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	PerspectiveCamera(PerspectiveCamera const& other)
		: BaseCamera(other), frustum_(other.frustum_) {
		fillPropertyDescription();
	}

	PerspectiveCamera(const std::string& name, const std::string& id) : BaseCamera(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("frustumType", &frustumType_);
		properties_.emplace_back("frustum", &frustum_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;


	Property<int, DisplayNameAnnotation, EnumerationAnnotation, FeatureLevel> frustumType_{static_cast<int>(EFrustumType::Aspect_FieldOfView), {"Frustum Type"}, {EngineEnumeration::FrustumType}, {2}};
	Property<Table, DisplayNameAnnotation, LinkEndAnnotation> frustum_{{}, {"Frustum"}, {}};

private:
	void syncFrustum(BaseContext& context);

	OutdatedPropertiesStore cachedFrustumValues_;
};

using SPerspectiveCamera = std::shared_ptr<PerspectiveCamera>;

}  // namespace raco::user_types