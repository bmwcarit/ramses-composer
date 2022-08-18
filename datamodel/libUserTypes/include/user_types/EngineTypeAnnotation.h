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
#include "data_storage/Value.h"
#include "core/EngineInterface.h"

namespace raco::user_types {

using namespace raco::data_storage;

class EngineTypeAnnotation : public raco::data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"EngineTypeAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	EngineTypeAnnotation(EngineTypeAnnotation const& other) : AnnotationBase({{"engineType", &engineType_}}),
															  engineType_(other.engineType_) {}

	EngineTypeAnnotation(raco::core::EnginePrimitive engineType = raco::core::EnginePrimitive::Undefined) : AnnotationBase({{"engineType", &engineType_}}),
		engineType_(static_cast<int>(engineType)) {
	}

	EngineTypeAnnotation& operator=(const EngineTypeAnnotation& other) {
		engineType_ = other.engineType_;
		return *this;
	}

	raco::core::EnginePrimitive type() const {
		return static_cast<raco::core::EnginePrimitive>(*engineType_);
	}

	// This is a raco::core::EnginePrimitive
	Value<int> engineType_;
};

}  // namespace raco::user_types