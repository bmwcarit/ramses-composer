/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/TextureExternalAdaptor.h"

#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

TextureExternalAdaptor::TextureExternalAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::TextureExternal> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::TextureExternal::minSamplingMethod_}, [this]() {
	tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::TextureExternal::magSamplingMethod_}, [this]() {
	tagDirty();
		  })} {
}

bool TextureExternalAdaptor::sync(core::Errors* errors) {
	reset(ramsesTextureSamplerExternal(sceneAdaptor_->scene(),
		static_cast<ramses::ETextureSamplingMethod>(*editorObject()->minSamplingMethod_),
		static_cast<ramses::ETextureSamplingMethod>(*editorObject()->magSamplingMethod_)));

	tagDirty(false);
	return true;
}

std::vector<ExportInformation> TextureExternalAdaptor::getExportInformation() const {
	if (getRamsesObjectPointer()) {
		return {
			ExportInformation{ramsesObject().getType(), ramsesObject().getName()},
		};
	}
	return {};
}

}  // namespace raco::ramses_adaptor
