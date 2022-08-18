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

#include "ReflectionInterface.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace raco::data_storage {

class AnnotationBase : public ClassWithReflectedMembers {
public:
	AnnotationBase(std::vector<std::pair<std::string, ValueBase*>>&& properties) : ClassWithReflectedMembers(std::move(properties)) {}

	virtual ~AnnotationBase() {}
};

using SAnnotationBase = std::shared_ptr<AnnotationBase>;

}  // namespace raco::data_storage