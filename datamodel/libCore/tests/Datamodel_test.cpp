/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "testing/TestEnvironmentCore.h"

#include "core/CoreAnnotations.h"

using namespace raco::core;
using namespace raco::user_types;

class DataModelTest : public TestEnvironmentCore {
public:
};

TEST_F(DataModelTest, check_user_type_property_annotations) {
	// Check on the annotations of the top-level properties in the user_type objects

	auto& userFactory{UserObjectFactory::getInstance()};

	for (auto& item : userFactory.getTypes()) {
		auto name = item.first;
		auto object = objectFactory()->createObject(name);
		ASSERT_TRUE(object != nullptr);
		for (size_t index = 0; index < object->size(); index++) {
			auto value = object->get(index);
			EXPECT_FALSE(value->query<ArraySemanticAnnotation>() && !value->query<HiddenProperty>()) << fmt::format("{}:{} has ArraySemanticAnnotation but not HiddenProperty", name, object->name(index));

			if (value->query<EnumerationAnnotation>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::Int) << fmt::format("{}:{} has EnumerationAnnotation but is not of type int", name, object->name(index));
			}

			if (value->query<URIAnnotation>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::String) << fmt::format("{}:{} has URIAnnotation but is not of type string", name, object->name(index));
			}

			if (value->query<ArraySemanticAnnotation>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::Table) << fmt::format("{}:{} has ArraySemanticAnnotation but is not of type Table", name, object->name(index));
			}

			if (value->query<TagContainerAnnotation>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::Table) << fmt::format("{}:{} has TagContainerAnnotation but is not of type Table", name, object->name(index));
			}

			if (value->query<RenderableTagContainerAnnotation>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::Table) << fmt::format("{}:{} has RenderableTagContainerAnnotation but is not of type Table", name, object->name(index));
			}

			if (value->query<ExpectEmptyReference>()) {
				EXPECT_TRUE(value->type() == PrimitiveType::Ref) << fmt::format("{}:{} has ExpectEmptyReference but is not of type Ref", name, object->name(index));
			}
		}
	}
}
