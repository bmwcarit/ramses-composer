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

#include "data_storage/AnnotationBase.h"

namespace raco::user_types {

class TexturePreviewEditorAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"TexturePreviewEditorAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	TexturePreviewEditorAnnotation(const TexturePreviewEditorAnnotation& other) : AnnotationBase({}) {}
	TexturePreviewEditorAnnotation() : AnnotationBase({}) {}

	TexturePreviewEditorAnnotation& operator=(const TexturePreviewEditorAnnotation& other) {
		return *this;
	}
};

}  // namespace raco::user_types