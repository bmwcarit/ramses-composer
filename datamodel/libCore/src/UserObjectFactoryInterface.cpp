/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/UserObjectFactoryInterface.h"
#include "core/Link.h"

#include "serialization/Serialization.h"

namespace raco::core {

raco::serialization::DeserializationFactory UserObjectFactoryInterface::deserializationFactory(UserObjectFactoryInterface* objectFactory) {
	return raco::serialization::DeserializationFactory{
		[objectFactory](const std::string& type) -> raco::serialization::SReflectionInterface {
			if (type == Link::typeDescription.typeName) {
				return std::make_shared<Link>();
			}
			return objectFactory->createObject(type);
		},
		[objectFactory](const std::string& type) {
			return objectFactory->createAnnotation(type);
		},
		[objectFactory](const std::string& type) {
			return objectFactory->createValue(type);
		}};
}

}  // namespace raco::core
