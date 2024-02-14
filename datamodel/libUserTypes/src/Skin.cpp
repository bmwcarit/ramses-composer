/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/Skin.h"

#include "core/Errors.h"

#include "Validation.h"

namespace raco::user_types {

void Skin::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&Skin::uri_) || value.isRefToProp(&Skin::skinIndex_)) {
		updateFromExternalFile(context);
	}
}

void Skin::updateFromExternalFile(BaseContext& context) {
	if (validateURI(context, {shared_from_this(), &Skin::uri_})) {
		std::string uriAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Skin::uri_});
		std::string loaderErrorMessage;
		skinData_ = context.meshCache()->loadSkin(uriAbsPath, *skinIndex_, loaderErrorMessage);
		if (!skinData_) {
			context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, {shared_from_this()}, loaderErrorMessage);
		}
	} else {
		context.errors().removeError(ValueHandle{shared_from_this()});
		skinData_.reset();
	}

	if (skinData_) {
		std::string info;

		info.append(fmt::format("Skin {} of {}\n", *skinIndex_ + 1, skinData_->numSkins));
		info.append(fmt::format("Number of Joints: {}", skinData_->inverseBindMatrices.size()));

		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {shared_from_this()}, info);
	}

	syncJointProperties(context);

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void Skin::syncJointProperties(BaseContext& context) {
	if (joints_->size() > numJoints()) {
		while (joints_->size() > numJoints()) {
			context.removeProperty(core::ValueHandle(shared_from_this(), &Skin::joints_), joints_->size() - 1);
		}
	} else if (joints_->size() < numJoints()) {
		for (auto index = joints_->size(); index < numJoints(); index++) {
			context.addProperty(core::ValueHandle(shared_from_this(), &Skin::joints_), {}, std::make_unique<Value<SNode>>(), -1);
		}
	}
}

void Skin::setupTargetProperties(size_t numTargets) {
	targets_->resize(numTargets);
}

size_t Skin::numJoints() const {
	if (skinData_) {
		return skinData_->inverseBindMatrices.size();
	}
	return 0;
}

SharedSkinData Skin::skinData() const {
	return skinData_;
}

}  // namespace raco::user_types
