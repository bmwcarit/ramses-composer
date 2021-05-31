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

#include "data_storage/AnnotationBase.h"
#include "data_storage/Value.h"

namespace raco::core {

using namespace raco::data_storage;

class ExternalReferenceAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"ExternalReferenceAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	ExternalReferenceAnnotation(const ExternalReferenceAnnotation& other) : AnnotationBase({{"projectID", &projectID_}}),
																			projectID_(other.projectID_) {
	}

	ExternalReferenceAnnotation(const std::string& projectID = std::string()) : AnnotationBase({{"projectID", &projectID_}}),
																				  projectID_(projectID) {
	}

	ExternalReferenceAnnotation& operator=(const ExternalReferenceAnnotation& other) {
		projectID_ = other.projectID_;
		return *this;
	}

	Value<std::string> projectID_;
};

}  // namespace raco::core
