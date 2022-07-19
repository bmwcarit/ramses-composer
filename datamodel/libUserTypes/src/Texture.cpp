/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Texture.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"

namespace raco::user_types {

void Texture::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&Texture::mipmapLevel_)) {
		validateURIs(context);
		validateMipmapLevel(context);
	} else if (value.isRefToProp(&Texture::generateMipmaps_)) {
		validateMipmapLevel(context);
	}
}

void Texture::updateFromExternalFile(BaseContext& context) {
	context.errors().removeError({shared_from_this()});
	validateURIs(context);

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void Texture::validateURIs(BaseContext& context) {
	context.errors().removeError({shared_from_this(), &Texture::uri_});
	context.errors().removeError({shared_from_this(), &Texture::level2uri_});
	context.errors().removeError({shared_from_this(), &Texture::level3uri_});
	context.errors().removeError({shared_from_this(), &Texture::level4uri_});

	validateURI(context, {shared_from_this(), &Texture::uri_});

	if (*mipmapLevel_ > 1) {
		validateURI(context, {shared_from_this(), &Texture::level2uri_});
	}

	if (*mipmapLevel_ > 2) {
		validateURI(context, {shared_from_this(), &Texture::level3uri_});
	}

	if (*mipmapLevel_ > 3) {
		validateURI(context, {shared_from_this(), &Texture::level4uri_});
	}
}

void Texture::validateMipmapLevel(BaseContext& context) {
	auto mipmapLevelValue = ValueHandle{shared_from_this(), &raco::user_types::Texture::mipmapLevel_};
	context.errors().removeError(mipmapLevelValue);

	if (*mipmapLevel_ < 1 || *mipmapLevel_ > 4) {
		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, mipmapLevelValue,
			"Invalid mipmap level - only mipmap levels 1 to 4 are allowed.");
	} else if (*generateMipmaps_ && *mipmapLevel_ != 1) {
		context.errors().addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, mipmapLevelValue,
			"Mipmap level larger than 1 specified while auto-generation flag is on - Ramses will ignore the auto-generation flag and still try to use the manually specified URIs.");
	}
}

}  // namespace raco::user_types
