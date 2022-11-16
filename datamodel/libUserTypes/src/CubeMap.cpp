/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/CubeMap.h"

#include "Validation.h"
#include "core/Context.h"
#include "utils/FileUtils.h"
#include <algorithm>


namespace raco::user_types {

void CubeMap::onAfterContextActivated(BaseContext& context) {
	BaseObject::onAfterContextActivated(context);

	validateMipmapLevel(context);
}

void CubeMap::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&CubeMap::mipmapLevel_)) {
		validateURIs(context);
		validateMipmapLevel(context);
	} else if (value.isRefToProp(&CubeMap::generateMipmaps_)) {
		validateMipmapLevel(context);
	}
}

void CubeMap::updateFromExternalFile(BaseContext& context) {
	validateURIs(context);

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void CubeMap::validateURIs(BaseContext& context) {
	context.errors().removeError({shared_from_this(), &CubeMap::uriFront_});
	context.errors().removeError({shared_from_this(), &CubeMap::uriBack_});
	context.errors().removeError({shared_from_this(), &CubeMap::uriLeft_});
	context.errors().removeError({shared_from_this(), &CubeMap::uriRight_});
	context.errors().removeError({shared_from_this(), &CubeMap::uriTop_});
	context.errors().removeError({shared_from_this(), &CubeMap::uriBottom_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriFront_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriBack_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriLeft_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriRight_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriTop_});
	context.errors().removeError({shared_from_this(), &CubeMap::level2uriBottom_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriFront_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriBack_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriLeft_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriRight_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriTop_});
	context.errors().removeError({shared_from_this(), &CubeMap::level3uriBottom_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriFront_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriBack_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriLeft_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriRight_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriTop_});
	context.errors().removeError({shared_from_this(), &CubeMap::level4uriBottom_});

	validateURI(context, {shared_from_this(), &CubeMap::uriFront_});
	validateURI(context, {shared_from_this(), &CubeMap::uriBack_});
	validateURI(context, {shared_from_this(), &CubeMap::uriLeft_});
	validateURI(context, {shared_from_this(), &CubeMap::uriRight_});
	validateURI(context, {shared_from_this(), &CubeMap::uriTop_});
	validateURI(context, {shared_from_this(), &CubeMap::uriBottom_});

	if (*mipmapLevel_ > 1) {
		validateURI(context, {shared_from_this(), &CubeMap::level2uriFront_});
		validateURI(context, {shared_from_this(), &CubeMap::level2uriBack_});
		validateURI(context, {shared_from_this(), &CubeMap::level2uriLeft_});
		validateURI(context, {shared_from_this(), &CubeMap::level2uriRight_});
		validateURI(context, {shared_from_this(), &CubeMap::level2uriTop_});
		validateURI(context, {shared_from_this(), &CubeMap::level2uriBottom_});
	}

	if (*mipmapLevel_ > 2) {
		validateURI(context, {shared_from_this(), &CubeMap::level3uriFront_});
		validateURI(context, {shared_from_this(), &CubeMap::level3uriBack_});
		validateURI(context, {shared_from_this(), &CubeMap::level3uriLeft_});
		validateURI(context, {shared_from_this(), &CubeMap::level3uriRight_});
		validateURI(context, {shared_from_this(), &CubeMap::level3uriTop_});
		validateURI(context, {shared_from_this(), &CubeMap::level3uriBottom_});
	}

	if (*mipmapLevel_ > 3) {
		validateURI(context, {shared_from_this(), &CubeMap::level4uriFront_});
		validateURI(context, {shared_from_this(), &CubeMap::level4uriBack_});
		validateURI(context, {shared_from_this(), &CubeMap::level4uriLeft_});
		validateURI(context, {shared_from_this(), &CubeMap::level4uriRight_});
		validateURI(context, {shared_from_this(), &CubeMap::level4uriTop_});
		validateURI(context, {shared_from_this(), &CubeMap::level4uriBottom_});
	}
}

void CubeMap::validateMipmapLevel(BaseContext& context) {
	auto mipmapLevelValue = ValueHandle{shared_from_this(), &raco::user_types::CubeMap::mipmapLevel_};

	if (*mipmapLevel_ < 1 || *mipmapLevel_ > 4) {
		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, mipmapLevelValue,
			"Invalid mipmap level - only mipmap levels 1 to 4 are allowed.");
	} else if (*generateMipmaps_ && *mipmapLevel_ != 1) {
		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, mipmapLevelValue,
			"Mipmap level larger than 1 specified while auto-generation flag is on - Ramses will ignore the auto-generation flag and still try to use the manually specified URIs.");
	} else {
		context.errors().removeError(mipmapLevelValue);
	}
}

}  // namespace raco::user_types
